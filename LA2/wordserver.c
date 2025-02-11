/*
    ==========================================
    Assignment 2 Submission 
    Name: SUJAN JAIN     
    Roll number: 22CS10075  
    Link of the pcap file: https://drive.google.com/file/d/12J9aC0c0nJoj8O2tLyK4F6kSz5KvHFtb/view?usp=sharing
    ========================================== 
*/

#include <stdio.h> 
#include <string.h> 
#include <stdlib.h>
#include <sys/types.h> 
#include <arpa/inet.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <unistd.h>


#define PORT 5000 
#define MAXLINE 1000 
  
int main() 
{    
    // create 3 files with name 22CS10075_File*.txt
    FILE *file1 = fopen("22CS10075_File1.txt", "w");
    // write HELLO to file1
    fprintf(file1, "HELLO\n");
    fprintf(file1, "CS31206\n");
    fprintf(file1, "CS39006\n");
    fprintf(file1, "CS31208\n");
    fprintf(file1, "FINISH\n");
    fclose(file1);

    FILE *file2 = fopen("22CS10075_File2.txt", "w");
    // write HELLO to file2
    fprintf(file2, "HELLO\n");
    fprintf(file2, "FOOD\n");
    fprintf(file2, "CHOCOLATE\n");
    fprintf(file2, "TOFFEE\n");
    fprintf(file2, "FINISH\n");
    fclose(file2);

    FILE *file3 = fopen("22CS10075_File3.txt", "w");
    fprintf(file3, "HELLO\n");
    fprintf(file3, "BMW\n");
    fprintf(file3, "ROLLS-ROYCE\n");
    fprintf(file3, "FINISH\n");
    fclose(file3);
    char buffer[100]; 
    int serverfd; 
    socklen_t len;
    struct sockaddr_in servaddr, cliaddr; 
    bzero(&servaddr, sizeof(servaddr)); 
    // Create a UDP Socket 
    serverfd = socket(AF_INET, SOCK_DGRAM, 0);         
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(PORT); 
    servaddr.sin_family = AF_INET;  
   
    // bind server address to socket descriptor 
    bind(serverfd, (struct sockaddr*)&servaddr, sizeof(servaddr)); 
    
    printf("\nServer Running .........\n");
    //receive the datagram 
    len = sizeof(cliaddr);
    int n = recvfrom(serverfd, buffer, sizeof(buffer), 
            0, (struct sockaddr*)&cliaddr, &len); //receive message from server 
    buffer[n] = '\0'; 
    printf("\nReceived from Client: %s\n",buffer); 
    FILE *file = fopen(buffer, "r");
    // if file is not found then send NOTFOUND message to client
    if(file == NULL)
    {
        char message[100];
        strcpy(message, "NOTFOUND ");
        strcat(message, buffer);
        // send the response 
        sendto(serverfd, message, strlen(message), 0, 
                (struct sockaddr*)&cliaddr, sizeof(cliaddr));
        printf("\nMessage sent to client"); 
    }
    else{
        // read the first line of the file
        char line[100];
        fgets(line, 100, file);
        // send the response 
        sendto(serverfd, line, strlen(line), 0, 
                (struct sockaddr*)&cliaddr, sizeof(cliaddr));
        printf("Message sent to client\n"); 
        // now read until FINISH
        while(1)
        {
            len = sizeof(cliaddr);
            n = recvfrom(serverfd, buffer, sizeof(buffer), 
                    0, (struct sockaddr*)&cliaddr, &len); //receive message from server 
            buffer[n] = '\0'; 
            printf("Received from Client: %s\n",buffer); 
            // we will receive message like WORD1, WORD2, WORD3 , so we have to send ith word from the file
            fgets(line, 100, file);
            // send the response 
            sendto(serverfd, line, strlen(line), 0, 
                    (struct sockaddr*)&cliaddr, sizeof(cliaddr));
            printf("Message sent to client\n"); 
            if(strcmp(line, "FINISH\n") == 0)
            {
                break;
            }
        }
    }

    close(serverfd);
    printf("\nServer Exit .........\n");
    return 0;
} 
