#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

int main(int argc, char const *argv[])
{
    int sockfd;
    struct sockaddr * serv_addr,cli_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&serv_addr,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    bind(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
    listen(sockfd,5);
    while(1)
    {
        socklen_t clilen = sizeof(cli_addr);
        int clifd = accept(sockfd,(struct sockaddr *)&cli_addr,&clilen);
        if(fork() == 0)
        {
            close(sockfd);
            // communicate with child through clifd

            close(clifd);
            exit(0);
        }
        close(clifd);
    }
    return 0;
}

