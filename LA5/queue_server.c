/*
    ===================================== 
    Assignment 5 Submission 
    Name: Sujan Jain
    Roll number: 22CS10075 
    ===================================== 
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/sem.h> 
#include <sys/ipc.h>
#include <sys/wait.h> 
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/shm.h>

#define P(s) semop(s, &pop, 1)  
#define V(s) semop(s, &vop, 1)  
#define PORT 5000
#define MAX_TASKS 100
#define TASK_SIZE 100

// Structure for task queue
struct task_queue {
    int front;
    int rear;
    int count;
    char tasks[MAX_TASKS][TASK_SIZE];
};
int sem;
struct sembuf pop, vop;
/*  ASSUMPTIONS 
> There is fixed format for tasks which is "num1 op num2" as given in assg , please add tasks in this format only 
> All arithmatic operations are assumed to be valid, like division by zero is not handled
> After client has connected, there is a time limit within which client has to send GET_TASK or result of Task, otherwise client is treated as malicious 
*/


/* --- HELPER FUNCTION --- */
void handle_sigchld(int sig) {
    int status;
    while (waitpid(-1, &status, WNOHANG) > 0);
}

// load tasks from task.config into the queue once
void load_queue_tasks(struct task_queue *queue) {
    FILE *fp = fopen("task.config", "r");
    if(fp) {
        char line[TASK_SIZE];
        while(fgets(line, sizeof(line), fp)) {
            line[strcspn(line, "\n")] = '\0';
            if(strlen(line) > 0) {
                P(sem);
                if(queue->count < MAX_TASKS) {
                    strcpy(queue->tasks[queue->rear], line);
                    queue->rear = (queue->rear + 1) % MAX_TASKS;
                    queue->count++;
                }
                V(sem);
            }
        }
        fclose(fp);
    }
}

int main(int argc, char const *argv[])
{
    signal(SIGCHLD, handle_sigchld); // register the signal 
    // Semaphore Initialization
    sem = semget(IPC_PRIVATE, 1, 0777 | IPC_CREAT);
    semctl(sem, 0, SETVAL, 1);
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; 
    vop.sem_op = 1;

    // Shared Memory for task queue (
    int shmid = shmget(IPC_PRIVATE, sizeof(struct task_queue), 0777 | IPC_CREAT);
    struct task_queue *queue = (struct task_queue *) shmat(shmid, NULL, 0);
    // Initialize queue
    queue->front = queue->rear = queue->count = 0;

    // Load all tasks from task.config directly into the queue
    load_queue_tasks(queue);

    int sockfd, newsockfd;
    int clilen;
    struct sockaddr_in cli_addr, serv_addr;
    // add the socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);
    // bind the socket 
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        printf("Unable to bind local address\n");
        exit(0);
    }
    listen(sockfd, 5);
    while (1) {
        clilen = sizeof(cli_addr);
        // accepting a new client 
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (fork() == 0) {
            close(sockfd);
            // made it non blocking socket 
            int flags = fcntl(newsockfd, F_GETFL, 0);
            fcntl(newsockfd, F_SETFL, flags | O_NONBLOCK);

            int wait = 0; // to store inactivity period
            int result_calc = 0; // 0 means no task in progress
            char last_task[TASK_SIZE] = {0};  // store last assigned task for each client 
            while (1) {
                char buffer[100];
                memset(buffer, '\0', 100);
                int bytes_recv = recv(newsockfd, buffer, 100, 0);
                if (bytes_recv != -1 && bytes_recv != 0) {
                    wait = 0; // reset inactivity counter
                    if (strcmp(buffer, "GET_TASK") == 0) {
                        if (result_calc == 0) {
                            char task[TASK_SIZE];
                            memset(task,'\0',TASK_SIZE);
                            P(sem);
                            if(queue->count > 0) {
                                strcpy(task, queue->tasks[queue->front]);
                                queue->front = (queue->front + 1) % MAX_TASKS;
                                queue->count--;
                            }
                            V(sem);
                            if (strlen(task) == 0) {
                                char ret[100] = "No tasks available";
                                send(newsockfd, ret, strlen(ret) + 1, 0);
                                exit(0);
                            }
                            result_calc = 1; // mark task in progress
                            strcpy(last_task, task); // store assigned task
                            send(newsockfd, task, strlen(task) + 1, 0);
                        } else {
                            printf("Client has not completed previous task yet\n");
                        }
                    } else if (strcmp(buffer, "exit") == 0) {
                        printf("Client exited\n");
                        break;
                    } else {
                        printf("RESULT of %s\n", buffer);
                        result_calc = 0; // clear task in progress
                        last_task[0] = '\0'; // New: clear stored task
                    }
                } else if (bytes_recv == -1) {
                    if (errno == EWOULDBLOCK || errno == EAGAIN) {
                        sleep(5);
                        if (wait == 5) {
                            printf("Malicious client detected. No message received within Time Limit !!\n");
                            break;
                        } else wait++;
                    }
                }
                else if (bytes_recv == 0){
                    break; // client closed connection
                }
            }
            // if a task was assigned but not completed, requeue it
            // so that it is not lost 
            if (result_calc == 1 && strlen(last_task) > 0) {
                P(sem);
                if(queue->count < MAX_TASKS) {
                    strcpy(queue->tasks[queue->rear], last_task);
                    queue->rear = (queue->rear + 1) % MAX_TASKS;
                    queue->count++;
                }
                V(sem);
            }
            close(newsockfd);
            exit(0);
        }
        close(newsockfd);
    }

    shmdt(queue);  // Detach shared memory
    shmctl(shmid, IPC_RMID, NULL);  // Destroy shared memory
    return 0;
}
