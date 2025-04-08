/*
    ===================================== 
    Assignment 6 Submission 
    Name: Sujan Jain
    Roll number: 22CS10075 
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
#include <sys/sem.h> 
#include <sys/ipc.h>
#include <sys/wait.h> 
#include <errno.h>
#include <time.h>


/*
    Explaination of states used in server code 
    
    - STATE -1: (Initial State)
       The server waits for the client to send a "HELO <client_id>" command.On receiving "HELO <client_id>", transition to STATE 0 (Session Established).
       QUIT command can also be sent which closes the client connection 

    - STATE 0: (Session Established)
       Different commands can be sent from this particularly MAIL FROM, GET MAIL, LIST, QUIT, HELO 
       each command takes it to specific state and perform a action 

    - STATE 1: (Sender Identified)
       The client must send "RCPT TO: <email>" to specify the recipient.
       On "RCPT TO: <email>", transition to STATE 2 .

    - STATE 2: (Recipient Identified)
       The client must send "DATA" to start writing the email body.
       On "DATA", transition to STATE 0 (Receiving Email Content).

    - ERROR Handling:
       If any wrong syntax command is sent in any state, the server sends "400 ERR".
       If a command is sent in the wrong state, the server sends "403 FORBIDDEN".

    NOTE: YOU CAN ALWAYS GO TO STATE 0 using HELO command 

    Summary:
    - The server follows a strict sequence: HELO → MAIL FROM → RCPT TO → DATA.
    - After an email is stored, the session resets to STATE 0.
    - LIST and GET_MAIL can be executed in STATE 0.
    - QUIT is always allowed in STATE -1 or STATE 0.

*/
/* --- FUNCTION TO CHECK IF COMMAND IS CORRECT --- */
int check_syntax(const char *message) {
    if (strncmp(message, "HELO ", 5) == 0) return 1;
    if (strncmp(message, "MAIL FROM: ", 11) == 0) return 1;
    if (strncmp(message, "RCPT TO: ", 9) == 0) return 1;
    if (strcmp(message, "DATA") == 0) return 1;
    if (strncmp(message, "LIST ", 5) == 0) return 1;
    if (strncmp(message, "GET_MAIL ", 9) == 0) return 1;
    if (strcmp(message, "QUIT") == 0) return 1;
    
    return 0;
}
int main(int argc, char const *argv[])
{
    if(argc == 1)
    {
        printf("Run with a PORT number\n");
        exit(0);
    }
    uint16_t PORT = atoi(argv[1]);
    int sockfd, newsockfd;
    int clilen;
    struct sockaddr_in cli_addr, serv_addr;
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0))<0){
        perror("Error in socket creation\n");
        return 1;
    }
    serv_addr.sin_family	= AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port	= htons(PORT);
    // binding socket 
    if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Unable to bind local address\n");
        exit(0);
    }    
    listen(sockfd, 1);
    printf("Listening on port %d...\n",PORT);
    // common error strings 
    char ok[10] = "200 OK";
    char err[10] = "400 ERR";
    char nt_fnd[20] = "401 NOT FOUND";
    char frb[20] = "403 FORBIDDEN";
    char ser_err[20] = "500 SERVER ERROR";
    while(1)
    {
        clilen = sizeof(cli_addr);
        // accepting a new client 
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if(fork() == 0)
        {
            // creating a child for a client 
            close(sockfd);
            printf("Client connected: %s\n",inet_ntoa(cli_addr.sin_addr));
            // state matters
            int state = -1; 
            char sender[100] = "";
            char recipient[100] = "";
            while(1){
                char message[100];
                memset(message,'\0',100);
                int bytes_recv = recv(newsockfd, message, 100, 0);
                // check message syntax here 
                if(check_syntax(message) == 0)
                {
                    // send 400 message to client 
                    send(newsockfd,err,strlen(err)+1,0);
                    continue;
                }
                /* 
                   I only checked the first letter as it is distinct for each command 
                   and the full command is already checked above so it is done to keep code simple 
                */
                if(message[0] == 'H') // denotes HELO, is receivable at any state 
                {
                    printf("HELO received from %s\n",message+5);
                    send(newsockfd,ok,strlen(ok)+1,0);
                    state = 0;
                }
                else if(message[0] == 'L' && state == 0) // denotes LIST 
                {
                    printf("%s\n",message);
                    char list_email[100];
                    strncpy(list_email, message + 5, sizeof(list_email)-1);
                    list_email[sizeof(list_email)-1] = '\0';
                    char list_filepath[200];
                    sprintf(list_filepath, "mailbox/%s.txt", list_email);
                    FILE *fp = fopen(list_filepath, "r");
                    if(fp == NULL) {
                        send(newsockfd, nt_fnd, strlen(nt_fnd)+1, 0);
                        continue;
                    }
                    send(newsockfd,ok,strlen(ok)+1,0);
                    char list_buf[5000];
                    list_buf[0] = '\0';
                    char line[256];
                    int id_counter = 0;
                    char sender_temp[100];
                    char date_temp[50];
                    // function for reading from the file and generating the emails 
                    while(fgets(line, sizeof(line), fp) != NULL) {
                        if(strncmp(line, "Email ID:", 9) == 0) {
                            id_counter++;
                            fgets(sender_temp, sizeof(sender_temp), fp); // From: line
                            fgets(date_temp, sizeof(date_temp), fp); // Date: line
                            // Skip lines until separator is reached
                            char dummy[256];
                            while(fgets(dummy, sizeof(dummy), fp) != NULL) {
                                if(strncmp(dummy, "-----", 5) == 0)
                                    break;
                            }
                            char *from_field = sender_temp + 6; 
                            char *newline = strchr(from_field, '\n');
                            if(newline) *newline = '\0';
                            char *date_field = date_temp + 6; 
                            newline = strchr(date_field, '\n');
                            if(newline) *newline = '\0';
                            char entry[256];
                            sprintf(entry, "%d: Email from %s (%s)\n", id_counter, from_field, date_field);
                            strcat(list_buf, entry);
                        }
                    }
                    fclose(fp);
                    strcat(list_buf, "##END##"); // termination marker
                    send(newsockfd, list_buf, strlen(list_buf)+1, 0);
                    printf("Emails retrieved; list sent.\n");
                }
                else if(message[0] == 'G' && state == 0) // denotes GET_MAIL 
                {
                    printf("%s\n",message);
                    char get_email[100];
                    int get_id;
                    sscanf(message, "GET_MAIL %s %d", get_email, &get_id);
                    char get_filepath[200];
                    sprintf(get_filepath, "mailbox/%s.txt", get_email);
                    FILE *fp = fopen(get_filepath, "r");
                    if(fp == NULL) {
                        send(newsockfd, nt_fnd, strlen(nt_fnd)+1, 0);
                        continue;
                    }
                    char mail_content[5000];
                    mail_content[0] = '\0';
                    int found = 0;
                    int current_id = 0;
                    char line[256];
                    // function for generating the message 
                    while(fgets(line, sizeof(line), fp) != NULL) {
                        if(strncmp(line, "Email ID:", 9) == 0) {
                            current_id++;
                            if(current_id == get_id) {
                                found = 1;
                                strcat(mail_content, line);
                                // Read and append subsequent lines until separator "-----"
                                while(fgets(line, sizeof(line), fp) != NULL) {
                                    if(strncmp(line, "-----", 5) == 0)
                                        break;
                                    strcat(mail_content, line);
                                }
                                break;
                            } else {
                                // skip the rest of this email
                                while(fgets(line, sizeof(line), fp) != NULL) {
                                    if(strncmp(line, "-----", 5) == 0)
                                        break;
                                }
                            }
                        }
                    }
                    fclose(fp);
                    if(!found) {
                        send(newsockfd, nt_fnd, strlen(nt_fnd)+1, 0);
                    } else {
                        send(newsockfd,ok,strlen(ok)+1,0);
                        sleep(1);
                        strcat(mail_content, "##END##");
                        send(newsockfd, mail_content, strlen(mail_content)+1, 0);
                        printf("Email with id %d sent\n", get_id);
                    }
                }
                else if(message[0] == 'M' && state == 0) // denotes MAIL_FROM 
                {
                    strncpy(sender, message + 11, sizeof(sender)-1);
                    sender[sizeof(sender)-1] = '\0';
                    printf("MAIL FROM: %s\n", sender);
                    send(newsockfd, ok, strlen(ok)+1, 0);
                    state = 1;
                }
                else if(message[0] == 'R' && state == 1) // denotes RCPT TO 
                {
                    strncpy(recipient, message + 9, sizeof(recipient)-1);
                    recipient[sizeof(recipient)-1] = '\0';
                    printf("RCPT TO: %s\n", recipient);
                    send(newsockfd, ok, strlen(ok)+1, 0);
                    state = 2;
                }
                else if(message[0] == 'D' && state == 2)
                {
                    send(newsockfd, ok, strlen(ok)+1, 0);
                    char email_body[5000];
                    email_body[0] = '\0';
                    char line[256];
                    while(1) {
                        // receiving message until \n followed by '.' is received 
                        // no assumption is made that #send == #recv
                        memset(line, 0, sizeof(line));
                        int n = recv(newsockfd, line, sizeof(line), 0);
                        if(n <= 0)
                            break;
                        line[strcspn(line, "\r\n")] = '\0';
                        if(strcmp(line, ".") == 0)
                            break;
                        strcat(email_body, line);
                        strcat(email_body, "\n");
                    }
                    // Get current date
                    time_t t = time(NULL);
                    struct tm tm = *localtime(&t);
                    char date_str[50];
                    strftime(date_str, sizeof(date_str), "%d-%m-%Y", &tm);
                    
                    // Create mailbox directory if it doesn't exist
                    system("mkdir -p mailbox");
                    
                    // Build file path for the recipient's mailbox file
                    char filepath[200];
                    sprintf(filepath, "mailbox/%s.txt", recipient);
                    
                    // Determine email id by counting existing emails in the file
                    int email_id = 1;
                    FILE *fp = fopen(filepath, "r");
                    if(fp != NULL) {
                        char temp[500];
                        while(fgets(temp, sizeof(temp), fp) != NULL) {
                            if(strncmp(temp, "Email ID:", 9) == 0)
                                email_id++;
                        }
                        fclose(fp);
                    }
                    
                    // Append the new email to the file
                    fp = fopen(filepath, "a");
                    if(fp == NULL) {
                        send(newsockfd, ser_err, strlen(ser_err)+1, 0);
                        state = 0;
                        continue;
                    }
                    fprintf(fp, "Email ID: %d\n", email_id);
                    fprintf(fp, "From: %s\n", sender);
                    fprintf(fp, "Date: %s\n", date_str);
                    fprintf(fp, "%s\n", email_body);
                    fprintf(fp, "-----\n");
                    fclose(fp);
                    
                    char temp_ack[50] = "200 Message stored successfully";
                    send(newsockfd, temp_ack, strlen(temp_ack)+1, 0);
                    printf("DATA received, message stored.\n");
                    state = 0;
                }
                else if(message[0] == 'Q' && (state == -1 || state == 0))
                {
                    send(newsockfd, ok, strlen(ok)+1, 0);
                    sleep(1);
                    printf("Client disconnected\n");
                    char temp[50] = "200 Goodbye";
                    send(newsockfd,temp,strlen(temp)+1,0);
                    break;
                }
                else{
                    // send the message for forbidden action 
                    send(newsockfd,frb,strlen(frb)+1,0);
                }
            }   
            close(newsockfd);
            exit(0);
        }

        close(newsockfd);
    }
    return 0;
}
