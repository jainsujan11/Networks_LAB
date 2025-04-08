/*
    ===================================== 
    Assignment 6 Submission 
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
#include <errno.h>


int main(int argc, char const *argv[])
{
    if(argc != 3)
    {
        printf("Run with IP address and PORT number\n");
        exit(0);
    }
    char dest_ip[16];
    strcpy(dest_ip, argv[1]);
    uint16_t PORT = atoi(argv[2]);
    int sockfd;
    struct sockaddr_in serv_addr;
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0))<0){
        perror("Error in socket creation\n");
        return 1;
    }
    serv_addr.sin_family	= AF_INET;
	inet_aton(dest_ip, &serv_addr.sin_addr);
	serv_addr.sin_port	= htons(PORT);
    if((connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) < 0)
    {
        perror("Unable to connect to server\n");
        exit(0);
    }
    printf("Connected to My_SMTP server.\n");
    // Connection made 
    while(1)
    {
        printf("> ");
        fflush(stdout);
        char cmd[100];
        scanf(" %[^\n]s",cmd);
        send(sockfd, cmd, strlen(cmd)+1, 0);
        char resp[100];
        recv(sockfd, resp, 100, 0); // prints the response code from server for every command 
        printf("%s\n", resp);

        if(strcmp(resp, "400 ERR") == 0 || strcmp(resp, "403 FORBIDDEN") == 0 ||
           strcmp(resp, "401 NOT FOUND") == 0 || strcmp(resp, "500 SERVER ERROR") == 0)
        {
            continue;
        }
        if(cmd[0] == 'D')
        {
            // DATA: keep on sending data until you enter a single dot (.)
            printf("Enter your message (end with a single dot '.'):\n");
            char line[256];
            while(1) {
                fgets(line, sizeof(line), stdin);
                line[strcspn(line, "\n")] = '\0';
                send(sockfd, line, strlen(line)+1, 0);
                if(strcmp(line, ".") == 0)
                    break;
            }
            char ack[100];
            recv(sockfd, ack, 100, 0);
            printf("%s\n", ack);
        }
        else if(cmd[0] == 'L')
        {
            // LIST: receive list lines until termination marker "##END##" is detected.
            char buffer[1024];
            while (1) {
                int n = recv(sockfd, buffer, 1024, 0);
                if(n <= 0) break;
                if(strstr(buffer, "##END##") != NULL) {
                    char *end_ptr = strstr(buffer, "##END##");
                    *end_ptr = '\0';
                    printf("%s", buffer);
                    break;
                }
                printf("%s", buffer);
            }
            printf("\n");
        }
        else if(cmd[0] == 'G')
        {
            // GET_MAIL: receive email content until termination marker "##END##" is detected.
            char buffer[1024];
            while (1) {
                int n = recv(sockfd, buffer, 1024, 0);
                if(n <= 0) break;
                if(strstr(buffer, "##END##") != NULL) {
                    char *end_ptr = strstr(buffer, "##END##");
                    *end_ptr = '\0';
                    printf("%s", buffer);
                    break;
                }
                printf("%s", buffer);
            }
            printf("\n");
        }
        else if(cmd[0] == 'Q')
        {
            char temp[100];
            recv(sockfd, temp, 100, 0);
            printf("%s\n", temp);
            break;
        }
    }
    close(sockfd);
    return 0;
}