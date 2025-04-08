// flags in recv and send call 
// flags in setsockopt 
// revise syntax of threads, semaphores, SM 
// non blocking errno 
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h> 
#include <arpa/inet.h> 


int sendall(int s,char* buf, int * len)
{
    
}


int sendall(int s,char* buf, int* len)
{
    int total = 0;
    int bytesleft = *len;
    int n;
    while(total < *len) 
    {
        int n = send(s,buf + total,bytesleft,0);
        if(n == -1)  break;
        total += n;
        bytesleft -= n;
    }
    *len = total;
    return n==-1?-1:0;
}





int main(int argc, char const *argv[])
{
    int sockfd;
    struct sockaddr_in serv_addr,cli_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0 ) {

    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) {}
    char buffer[10] = "hello";
    // send(sockfd,buffer,sizeof(buffer),0);
    int x = strlen(buffer);
    int n = sendall(sockfd,buffer,&x);
    printf("%d\n",n);
    close(sockfd);
    return 0;
}
