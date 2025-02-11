/*
    ===================================== 
    Assignment 2 Submission 
    Name: SUJAN JAIN     
    Roll number: 22CS10075  
    Link of the pcap file: https://drive.google.com/file/d/12J9aC0c0nJoj8O2tLyK4F6kSz5KvHFtb/view?usp=sharing
    ===================================== 
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
    char buffer[100]; 
    char *message = "22CS10075_File1.txt"; 
    int sockfd, n;
    struct sockaddr_in servaddr; 
      
    // clear servaddr 
    bzero(&servaddr, sizeof(servaddr)); 
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    servaddr.sin_port = htons(PORT); 
    servaddr.sin_family = AF_INET; 
      
    // create datagram socket 
    sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
      
    // request to send datagram 
    sendto(sockfd, message, strlen(message), 0, (struct sockaddr*)&servaddr, sizeof(servaddr)); 
      
    // waiting for response 
    recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL); 
    printf("Received from Server:%s",buffer); 
    if(buffer[0] == 'N')
    {
        // it means no such file is there
        printf("\nERROR: FILE NOT FOUND \n");
    }
    else{
        // create a new file to write the content named 22CS10075_Client.txt
        FILE *file = fopen("22CS10075_Client.txt", "w");
        int cur_word = 1;
        while(1)
        {
            // create a string WORD + "cur_word" to send to server
            char word[100];
            sprintf(word, "WORD %d", cur_word);
            sendto(sockfd, word, strlen(word), 0, (struct sockaddr*)&servaddr, sizeof(servaddr)); 
            int n = recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL);
            buffer[n] = '\0'; 
            printf("Received from server:%s",buffer); 
            if(strcmp(buffer, "FINISH\n") == 0)
            {
                // breaking the loop upon receiving FINISH
                break;
            }
            // write buffer to the file
            fprintf(file, "%s", buffer);
            cur_word++;
        }
    }
    // close the descriptor 
    close(sockfd); 
    return 0;
} 
