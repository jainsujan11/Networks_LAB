#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h> 
#include <arpa/inet.h> 

int main(int argc, char const *argv[])
{
    int sockfd;
    struct sockaddr_in serv_addr,cli_addr;
    sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(sockfd < 0) {}
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000);
    serv_addr.sin_addr.s_addr  = INADDR_ANY;
    if(bind(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) {}
    listen(sockfd,5);
    while(1)
    {
        int clilen = sizeof(cli_addr);
        int newsockfd = accept(sockfd,(struct sockaddr *)&cli_addr,&clilen);
        if(fork() == 0)
        {
            close(sockfd);
            char buf[100];
            while(1){
                int bytes = recv(newsockfd,buf,1,MSG_PEEK);
                bytes = recv(newsockfd,buf,2,0);
                if(bytes <= 0) break;
                printf("%d\n",bytes);
                printf("%s\n",buf);
            }
            close(newsockfd);
            exit(0);
        }
        close(newsockfd);
    }
    return 0;
}
