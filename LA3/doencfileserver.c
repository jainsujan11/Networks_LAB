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


// code for encrypting a file 
void encrypt_file(const char *input, const char *output, const char *key) {
    FILE *fin = fopen(input, "r");
    FILE *fout = fopen(output, "w");
    char buffer[BUFFER_SIZE];

    if (!fin || !fout) {
        perror("File open error");
        exit(EXIT_FAILURE);
    }

    while (fgets(buffer, BUFFER_SIZE, fin)) {
        for (int i = 0; buffer[i] != '\0'; i++) {
            if (buffer[i] >= 'A' && buffer[i] <= 'Z') {
                buffer[i] = key[buffer[i] - 'A'];
            } else if (buffer[i] >= 'a' && buffer[i] <= 'z') {
                buffer[i] = key[buffer[i] - 'a'] + 32;
            }
        }
        fputs(buffer, fout);
    }

    fclose(fin);
    fclose(fout);
}
int main()
{
    int sockfd, newsockfd;
    int clilen;
    struct sockaddr_in cli_addr, serv_addr;
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Unable to create socket\n");
        exit(0);
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(20000);
    if(bind(sockfd,  &serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Unable to bind local address\n");
        exit(0);
    }    
    listen(sockfd, 1);
    while(1)
    {
        clilen = sizeof(cli_addr);
        // accepting a three way handshake 
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen); 
        if(newsockfd < 0){
            perror("Accept error\n");
            exit(0);
        }
        // for a particular client handshake is done only once 
        while(1){
            // store the first 26 bytes of the message as the key
            char key[27];
            char filename[100];
            // create a file of form <client_ip>.<client_port>.txt
            sprintf(filename, "%s.%d.txt", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
            FILE *fp = fopen(filename, "w");
            if(fp == NULL) printf("true\n");
            char buffer[100];
            int total_received = 0;
            int bytes_received = 0;
            // code for retreiving the key, extracting first 26 bytes of the message for the key 
            while(total_received < 26)
            {
                bytes_received = recv(newsockfd, buffer, BUFFER_SIZE, 0);
                if(bytes_received <= 0){perror("Error receiving key");exit(EXIT_FAILURE);}

                if (total_received + bytes_received <= 26) {
                // If we still need more data to complete the key
                memcpy(key + total_received, buffer, bytes_received);
                total_received += bytes_received;
                }else{
                    int extra_data_start = 26 - total_received;
                    memcpy(key + total_received, buffer, extra_data_start);
                    key[26] = '\0';  
                    // Write the remaining bytes to the file
                    fprintf(fp, "%.*s", bytes_received - extra_data_start, buffer + extra_data_start);
                    // Exit loop since the key is completely received
                    break;
                }
            }
            // Continue receiving and writing the remaining file content
            int n;
            while (n = read(newsockfd, buffer, BUFFER_SIZE)) {
                if(buffer[n-1] == '$'){
                    buffer[n-1] = '\0';
                    fprintf(fp, "%.*s", (int)n, buffer);
                    break;
                }
                fprintf(fp, "%.*s", (int)n, buffer);
            }
            fclose(fp);
            // now we will do the encryption 
            char encryptedfile[100];
            strcpy(encryptedfile, filename);
            strcat(encryptedfile, ".enc");
            
            encrypt_file(filename, encryptedfile, key);
            // now send the encryptedfile
            FILE* en = fopen(encryptedfile,"r");
            while (fgets(buffer, BUFFER_SIZE, en)) {
                send(newsockfd, buffer, strlen(buffer), 0);
            }
            char eof_marker = '$';
            send(newsockfd,&eof_marker,1,0);
            fclose(en);
            for (int i = 0; i < 100; i++)
            {
                buffer[i] = '\0';
            }
            read(newsockfd, buffer, BUFFER_SIZE);
            if(strcmp(buffer,"No") == 0){
                printf("Client %s:%d has disconnected.\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
                break;
            }

        }
        close(newsockfd);
    }

    return 0;
}