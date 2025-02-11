/*
    ===================================== 
    Assignment 3 Submission 
    Name: Sujan Jain
    Roll number: 22CS10075 
    Link of the pcap file: https://drive.google.com/drive/folders/1UGQQRvyApczh3OEZ0D-5gaQHkjQkTEY5?usp=sharing
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

#define BUFFER_SIZE 100 

int main()
{
    int sockfd;
    struct sockaddr_in serv_addr;
    // creating socket 
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Unable to create socket\n");
        exit(0);
    }
    serv_addr.sin_family = AF_INET;
    inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port	= htons(20000);
    // initialising a 3 way handshake 
    if((connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) < 0)
    {
        perror("Unable to connect to server\n");
        exit(0);
    }
    do
    {
        char filename[100];
        printf("Enter the filename to retrieve: ");
        scanf("%s", filename);
        FILE *fp = fopen(filename, "r");
        if(fp == NULL) 
        {
            printf("NOTFOUND %s\n",filename);
            continue;
        }
        // so the file exists and we can proceed further 
        char key[27];
        while(1)
        {
            printf("Enter the key: ");
            scanf("%s", key);
            if(strlen(key) != 26) 
            {
                printf("Invalid Key !!\n");   
                continue;
            }
            else break;
        } 
        // first we will send the key
        send(sockfd, key, strlen(key), 0);
        // then we will send the file contents
        char buffer[BUFFER_SIZE];
        while (fgets(buffer, BUFFER_SIZE, fp)) {
            send(sockfd, buffer, strlen(buffer), 0);
        }
        char eof_marker = '$'; // send this to denote end of file for server 
        send(sockfd,&eof_marker,1,0);
        fclose(fp);
        // now the server will send the encrypted file
        char encryptedfile[100];
        strcpy(encryptedfile, filename);
        strcat(encryptedfile, ".enc");
        FILE *encfile = fopen(encryptedfile, "w");
        while(1)
        {
            int n = recv(sockfd, buffer, BUFFER_SIZE, 0);
            if(buffer[n-1] == '$'){ // eof checking 
                buffer[n-1] = '\0';
                fprintf(encfile, "%.*s", (int)n, buffer);
                break;
            }
            fprintf(encfile, "%.*s", (int)n, buffer);
        }
        printf("File is Encrypted successfully !!\n");
        printf("Original file: %s\n", filename);
        printf("Encrypted file: %s\n", encryptedfile);
        fclose(encfile);
        for (int i = 0; i < 100; i++)
        {
            buffer[i] = '\0';
        }
        printf("Do you want to continue? Enter No to exit: ");
        scanf("%s", buffer);
        send(sockfd,buffer,strlen(buffer),0);
        if(strcmp(buffer, "No") == 0) break;
    } while (1);
    
    close(sockfd);

    return 0;
}