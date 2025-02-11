#include <stdio.h>
#include <stdlib.h> 
#include <string.h>

#include <sys/types.h>          
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h> 
#include <netinet/in.h> 


#define PORT 5000
int main(int argc, char const *argv[])
{
    int sockfd;
    struct sockaddr_in serv_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    bzero(&serv_addr,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
    {
        perror("error in connecting\n");
        exit(1);
    }
    int c;
    do
    {
        char message[100];
        scanf(" %[^\n]s",message);
        send(sockfd,message,strlen(message),0);
        printf("Do you want to continue ?\n");
        scanf(" %d",&c);
    } while (c);

    close(sockfd);

    return 0;
}
