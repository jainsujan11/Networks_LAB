#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <sys/select.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <unistd.h>


#define PORT 5000
#define MAX_CLIENTS 5


int main(int argc, char const *argv[])
{
    int sockfd;
    struct sockaddr_in serv_addr,cli_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    printf("%d\n",sockfd);
    if(sockfd < 0 ){perror("error in socket creating\n");exit(0);}
    bzero(&serv_addr,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    bind(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
    listen(sockfd,5);

    int clients[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i] = -1;
    }
    
    fd_set readfds;
    int max_fd = 0;
    while(1)
    {
        FD_ZERO(&readfds);
        FD_SET(0,&readfds);
        FD_SET(sockfd,&readfds);
        max_fd = sockfd;
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if(clients[i] != -1)
            {
                FD_SET(clients[i],&readfds);
                if(clients[i] > max_fd) max_fd = clients[i];
            }
            else{
                break;
            }
        }
        int activity = select(max_fd+1,&readfds,NULL,NULL,NULL);
        if(activity < 0) {perror("no activity");exit(1);}
        if(FD_ISSET(sockfd,&readfds))
        {
            // new connection is incoming 
            socklen_t clilen = sizeof(cli_addr);
            int clifd = accept(sockfd,(struct sockaddr *)&cli_addr,&clilen);
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if(clients[i] == -1)
                {
                    clients[i] = clifd;
                    break;
                }
            }
        }
        if(FD_ISSET(0,&readfds))
        {
            char message[100];
            // fgets(message,100,stdin);
            scanf(" %[^\n]s",message);
            printf("User from terminal entered : %s\n",message);
        }
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if(FD_ISSET(clients[i],&readfds))
            {
                // has something 
                char message[100];
                int n = recv(clients[i],message,100,0);
                if(n == 0)
                {
                    // closed the connection 
                    close(clients[i]);
                    clients[i]=-1;
                    printf("Client %d disconnected\n",i+1);
                }
                else{
                    message[n] = '\0';
                    printf("Client %d sent the message %s\n",i+1,message);
                }
            }
        }
    }
    return 0;
}
