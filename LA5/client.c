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
#include <fcntl.h>
#include <errno.h> 

#define PORT 5000


int do_task(const char *expression) {
    int num1, num2;
    char op;

    // Parsing the input string
    if (sscanf(expression, "%d %c %d", &num1, &op, &num2) != 3) {
        fprintf(stderr, "Invalid expression format\n");
        return 0;
    }

    // Performing the operation
    switch (op) {
        case '+': return num1 + num2;
        case '-': return num1 - num2;
        case '*': return num1 * num2;
        case '/':
            if (num2 == 0) {
                fprintf(stderr, "Division by zero error\n");
                return 0;
            }
            return num1 / num2;
        default:
            fprintf(stderr, "Invalid operator\n");
            return 0;
    }
}


int main(int argc, char const *argv[])
{
    int sockfd;
	struct sockaddr_in serv_addr;
	int i;
	char buf[100];
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to create socket\n");
		exit(0);
	}
    serv_addr.sin_family	= AF_INET;
	inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port	= htons(PORT);
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    // Set non-blocking mode
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Client socket set to non-blocking mode.\n");
    int ret;
    while ((ret = connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) != 0) {
        if (errno == EINPROGRESS) {
            sleep(1);
            continue;
        } else {
            perror("connect failed");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
    }
    printf("Connected successfully!\n");
    // Normal client code 
    while(1)
    {
        int choice;
        printf("Enter 1 to continue, 0 to exit\n");
        scanf("%d",&choice);
        if(choice == 0)
        {
            char buf[100];
            strcpy(buf,"exit");
            send(sockfd,buf,strlen(buf)+1,0);
            break;
        }
        else{
            char buf[100];
            strcpy(buf,"GET_TASK");
            send(sockfd,buf,strlen(buf)+1,0);
            int flag = 0;
            while(1)
            {
                char buffer[100];
                memset(buffer,'\0',100);
                int bytes_recv = recv(sockfd, buffer, 100, 0);
                if(bytes_recv!=-1)
                {
                    printf("Task received: %s\n",buffer);
                    if(strcmp(buffer,"No tasks available") == 0) {flag=1;}
                    else{
                        int num1, num2;
                        char op;
                        sscanf(buffer, "%d %c %d", &num1, &op, &num2);
                        
                        int ans = do_task(buffer);
                        char res[200];
                        memset(res, '\0', 200);
                        sprintf(res, "%s is %d", buffer, ans);
                        send(sockfd,res,strlen(res)+1,0);

                    }
                    sleep(5);
                    break;
                }
                else if (bytes_recv == -1){
                    if(errno == EAGAIN || errno == EWOULDBLOCK) sleep(1);
                }
            }
            if(flag == 1) break;
        }
    }
	
    
    // for a client who does nothing after connecting, do nothing after connecting
    
    // // for a client who send multiple GET_TASK request
    // strcpy(buf,"GET_TASK");
    // send(sockfd,buf,strlen(buf)+1,0);
    // send(sockfd,buf,strlen(buf)+1,0);
    // send(sockfd,buf,strlen(buf)+1,0);
    // // in this server will respond with pending task 

    close(sockfd);
    return 0;
}
