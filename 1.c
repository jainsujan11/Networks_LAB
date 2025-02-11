#include <stdio.h>

// UDP server 
int main(int argc, char const *argv[])
{
    int sockfd;
    socklen_t len;
    struct sockaddr_in serv_addr,cli_addr;
    socket = socket(AF_INET, SOCK_DGRAM, 0);
    bzero(serv_addr,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    bind(sockfd,serv_addr,sizeof(serv_addr));
    len = sizeof(cli_addr); 



    return 0;
}


// UDP client 
int main(int argc, char const *argv[])
{
    int sockfd;
    struct sockaddr_in serv_addr;
    socket = socket(AF_INET, SOCK_DGRAM, 0);
    bzero(serv_addr,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    sendto(sockfd,message,strlen(message),0,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
    return 0;
}

// TCP concurrent server 
int main(int argc, char const *argv[])
{
    int sockfd;
    socklen_t len;
    struct sockaddr_in serv_addr,cli_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(serv_addr,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    bind(sockfd,serv_addr,sizeof(serv_addr));
    listen(sockfd,2);
    fd_set readfds;
    int max_fd = 0;
    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(0,&readfds);
        FD_SET(sockfd,&readfds);
        max_fd = sockfd;
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            // do same as above
        }
        
        if(FD_ISSET(0,readfds))
        {

        }        
    }
    
    return 0;
}
