/*
    ===================================== 
    Assignment 4 Submission 
    Name: Sujan Jain
    Roll number: 22CS10075 
    ===================================== 
*/


#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/select.h>
#include <signal.h>
#include <ksocket.h>
#include <assert.h>


/* --- Receiver Thread --- */
void *R(void *arg) {
    fd_set readSet;
    while (1) {
        FD_ZERO(&readSet);
        int maxFD = 0;
        /* Populate readSet with UDP sockets from active entries */
        for (int i = 0; i < N; i++) {
            if (SM[i].available == 0) {
                FD_SET(SM[i].udp_fd, &readSet);
                if (SM[i].udp_fd > maxFD)
                    maxFD = SM[i].udp_fd;
            }
        }
        struct timeval tv = {T, 0};
        // timeout after T secs 
        int ready = select(maxFD + 1, &readSet, NULL, NULL, &tv);
        if (ready < 0) {
            perror("select error");
        } else if (ready == 0) {
            printf("Receiver: select T\n");
            /* Refresh the master set and check for buffer space */
            FD_ZERO(&readSet);
            maxFD = 0;
            P(sem_SM);
            for (int i = 0; i < N; i++) {
                if (SM[i].available == 0) {
                    FD_SET(SM[i].udp_fd, &readSet);

                    if (SM[i].udp_fd > maxFD)
                        maxFD = SM[i].udp_fd;
                    
                    if (SM[i].nospace == 1 && SM[i].rwnd.window_size > 0)
                    {
                        SM[i].nospace = 0;
                    }
                    
                    /* Send a duplicate ACK */
                    int lastSeq = (SM[i].rwnd.start_seq - 1 + 256) % 256;
                    struct sockaddr_in clientAddr;
                    clientAddr.sin_family = AF_INET;
                    inet_aton(SM[i].dest_ip, &clientAddr.sin_addr);
                    clientAddr.sin_port = htons(SM[i].dest_port);
                    char ackMsg[13];
                    ackMsg[0] = '0'; // for ACK message
                    for (int bit = 0; bit < 8; bit++)
                        // next 8 bit sequence number
                        ackMsg[1 + bit] = ((lastSeq >> (7 - bit)) & 1) + '0';
                    for (int bit = 0; bit < 4; bit++)
                        // next 4 bit rwnd size
                        ackMsg[9 + bit] = ((SM[i].rwnd.window_size >> (3 - bit)) & 1) + '0';
                    // send the message
                    sendto(SM[i].udp_fd, ackMsg, 13, 0,
                           (struct sockaddr *)&clientAddr, sizeof(clientAddr));
                }
            }
            V(sem_SM);
        } else {
            P(sem_SM);
            for (int i = 0; i < N; i++) {
                if (SM[i].available == 0 && FD_ISSET(SM[i].udp_fd, &readSet)) {
                    char buffer[600];
                    // initialize buffer to NULL
                    memset(buffer, '\0', sizeof(buffer));
                    struct sockaddr_in srcAddr;
                    socklen_t addrLen = sizeof(srcAddr);
                    int nbytes = recvfrom(SM[i].udp_fd, buffer, sizeof(buffer) - 1, 0,
                                          (struct sockaddr *)&srcAddr, &addrLen);
                    if(strlen(buffer) == 18) continue;
                    if (dropMessage(p)) /* dropping message explicitly */
                        continue;
                    if (nbytes < 0) {
                        perror("recvfrom error");
                    } else {
                        buffer[nbytes] = '\0';
                        if (buffer[0] == '0') {
                            /* Process ACK message */
                            int ackSeq = 0;
                            for (int bit = 0; bit < 8; bit++)
                                ackSeq = ackSeq * 2 + (buffer[1 + bit] - '0');
                            int newWnd = 0;
                            for (int bit = 0; bit < 4; bit++)
                                newWnd = newWnd * 2 + (buffer[9 + bit] - '0');
                            if (SM[i].swnd.slots[ackSeq] >= 0) {
                                // no dup ACK
                                for (int j = SM[i].swnd.start_seq;
                                    j % 256 != (ackSeq + 1) % 256; j++) {
                                    j %= 256;
                                    SM[i].swnd.slots[j] = -1;
                                    SM[i].last_sent[j] = -1;
                                    SM[i].send_buf_sz++;
                                }
                                SM[i].swnd.start_seq = (ackSeq + 1) % 256;
                            }
                            // has to be done in both cases 
                            SM[i].swnd.window_size = newWnd;
                        } else {
                            /* Process DATA message */
                            int dataSeq = 0;
                            for (int bit = 0; bit < 8; bit++)
                                dataSeq = dataSeq * 2 + (buffer[1 + bit] - '0');
                           
                            int dataLen = 0;
                            for (int bit = 0; bit < 9; bit++)
                                dataLen = dataLen * 2 + (buffer[9 + bit] - '0');
                            if (dataSeq == SM[i].rwnd.start_seq) {
                                // In order message
                                int bufIndex = SM[i].rwnd.slots[dataSeq];
                                memcpy(SM[i].recv_buf[bufIndex], buffer + 18, dataLen);
                                SM[i].recv_valid[bufIndex] = 1;
                                SM[i].rwnd.window_size--;
                                SM[i].recv_msg_length[bufIndex] = dataLen;
                                SM[i].nospace = 0;
                                // Finding the next seq number
                                while (SM[i].rwnd.slots[SM[i].rwnd.start_seq] >= 0 && SM[i].recv_valid[SM[i].rwnd.slots[SM[i].rwnd.start_seq]] == 1){
                                    SM[i].rwnd.start_seq = (SM[i].rwnd.start_seq + 1) % 256;
                                }
                                // Send ACK
                                int lastSeq = (SM[i].rwnd.start_seq+256-1)%256;           // Last in-order message received
                                char ackMsg[13];
                                ackMsg[0] = '0'; // for ACK message
                                for (int bit = 0; bit < 8; bit++)
                                    // next 8 bit sequence number
                                    ackMsg[1 + bit] = ((lastSeq >> (7 - bit)) & 1) + '0';
                                for (int bit = 0; bit < 4; bit++)
                                    // next 4 bit rwnd size
                                    ackMsg[9 + bit] = ((SM[i].rwnd.window_size >> (3 - bit)) & 1) + '0';
                                // send the message
                                ackMsg[13] = '\0';
                                sendto(SM[i].udp_fd, ackMsg, 13, 0,
                                    (struct sockaddr *)&srcAddr, sizeof(srcAddr));
                            } else {
                                // Out of order message
                                if (SM[i].rwnd.slots[dataSeq] >= 0 && SM[i].recv_valid[SM[i].rwnd.slots[dataSeq]] == 0) {
                                    int bufIndex = SM[i].rwnd.slots[dataSeq];
                                    memcpy(SM[i].recv_buf[bufIndex], buffer + 18, dataLen);
                                    SM[i].recv_valid[bufIndex] = 1;
                                    SM[i].rwnd.window_size--;
                                    SM[i].recv_msg_length[bufIndex] = dataLen;
                                }
                                 // No need to send ACK as it is out of order
                            }
                            // else drop the message 
                            if (SM[i].rwnd.window_size == 0)
                                SM[i].nospace = 1;
                        }
                    }
                }
            }
            V(sem_SM);
        }
    }
    return NULL;
}

/* --- Sender Thread --- */
void *S(void *arg) {
    while (1) {
        sleep(T / 2);
        P(sem_SM);
        for (int i = 0; i < N; i++) {
            if (SM[i].available == 0) {
                struct sockaddr_in remoteAddr;
                remoteAddr.sin_family = AF_INET;
                remoteAddr.sin_port = htons(SM[i].dest_port);
                inet_aton(SM[i].dest_ip, &remoteAddr.sin_addr);
                int shouldRetransmit = 0;
                int seqIndex = SM[i].swnd.start_seq;
                // if last sent is -1 then it is not sent yet
                while (seqIndex != (SM[i].swnd.start_seq + SM[i].swnd.window_size) % 256) {
                    if (SM[i].last_sent[seqIndex] != -1 &&
                        time(NULL) - SM[i].last_sent[seqIndex] > T) {
                        shouldRetransmit = 1;
                        break;
                    }
                    seqIndex = (seqIndex + 1) % 256;
                }
/*
    Each message will have a 9 bit header. 1 bit for type (0 = ACK, 1 = DATA), 8 bit for sequence number.
    Next bits are either 3-bit rwnd size (for ACK) or 9 bits of length and 512B data (for DATA).
*/
                if (shouldRetransmit) {
                    int j = SM[i].swnd.start_seq;
                    int startSeq = SM[i].swnd.start_seq;
                    while (j != (startSeq + SM[i].swnd.window_size) % 256) {
                        if(SM[i].swnd.slots[j] == -1) {
                            j = (j + 1) % 256;
                            continue;
                        }
                        char dataMsg[600];
                        memset(dataMsg, '\0', sizeof(dataMsg));
                        dataMsg[0] = '1'; // for DATA message 
                        for (int bit = 0; bit < 8; bit++)
                            // next 8 bit sequence number
                            dataMsg[1 + bit] = ((j >> (7 - bit)) & 1) + '0';
                        int msgLength = SM[i].send_msg_length[SM[i].swnd.slots[j]];
                        for (int bit = 0; bit < 9; bit++)
                            // next 9 bit length of message and max message length is 512 == (1<<9)
                            dataMsg[9 + bit] = ((msgLength >> (8 - bit)) & 1) + '0';
                        memcpy(dataMsg + 18, SM[i].send_buf[SM[i].swnd.slots[j]], msgLength);
                        dataMsg[18 + msgLength] = '\0';
                        // send the message
                        sendto(SM[i].udp_fd, dataMsg, 18 + msgLength, 0,
                               (struct sockaddr *)&remoteAddr, sizeof(remoteAddr));
                        SM[i].transmission_cnt++;
                        SM[i].last_sent[j] = time(NULL);
                        j = (j + 1) % 256;
                    }
                } else {
                    int j = SM[i].swnd.start_seq;
                    int startSeq = SM[i].swnd.start_seq;
                    while (j != (startSeq + SM[i].swnd.window_size) % 256) {
                        if (SM[i].swnd.slots[j] != -1 && SM[i].last_sent[j] == -1) {
                            char dataMsg[600];
                            memset(dataMsg, '\0', sizeof(dataMsg));
                            dataMsg[0] = '1';
                            for (int bit = 0; bit < 8; bit++)
                                dataMsg[1 + bit] = ((j >> (7 - bit)) & 1) + '0';
                            int msgLength = SM[i].send_msg_length[SM[i].swnd.slots[j]];
                            for (int bit = 0; bit < 9; bit++)
                                dataMsg[9 + bit] = ((msgLength >> (8 - bit)) & 1) + '0';
                            memcpy(dataMsg + 18, SM[i].send_buf[SM[i].swnd.slots[j]], msgLength);
                            dataMsg[18 + msgLength] = '\0';
                            sendto(SM[i].udp_fd, dataMsg, 18 + msgLength, 0,
                                   (struct sockaddr *)&remoteAddr, sizeof(remoteAddr));
                            SM[i].transmission_cnt++;
                            SM[i].last_sent[j] = time(NULL);
                        }
                        j = (j + 1) % 256;
                    }
                }
            }
        }
        V(sem_SM);
    }
    return NULL;
}

/* --- Garbage Thread --- */
void *G() {
    // Garbage collector thread to identify killed messages and close their sockets if not closed already in the SM table
    while (1) {
        sleep(T);
        P(sem_SM);
        for (int i = 0; i < N; i++) {
            if (SM[i].available) continue;                    // Free entry
            if (kill(SM[i].process_id, 0) == 0) continue;   // Process still running
            SM[i].available = 1;                              // Process killed, free the entry
        }
        V(sem_SM);
    }

}



/* --- Main Function (Init Process) --- */
int main() {
    srand(time(0));

    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1;
    vop.sem_op = 1;

    /* Generate keys for shared resources */
    int key_shm_info   = ftok("/", 65);
    int key_shm_SM  = ftok("/", 66);
    int key_sem1   = ftok("/", 67);
    int key_sem2    = ftok("/", 68);
    int key_sem_SM  = ftok("/", 69);
    int key_sem_info   = ftok("/", 70);

    shm_id_info  = shmget(key_shm_info, sizeof(KTP_INFO), 0666 | IPC_CREAT);
    shm_id_SM = shmget(key_shm_SM, sizeof(struct ktp_entry) * N, 0666 | IPC_CREAT);
    sem1     = semget(key_sem1, 1, 0666 | IPC_CREAT);
    sem2      = semget(key_sem2, 1, 0666 | IPC_CREAT);
    sem_SM    = semget(key_sem_SM, 1, 0666 | IPC_CREAT);
    sem_info     = semget(key_sem_info, 1, 0666 | IPC_CREAT);

    ktpInfo = (KTP_INFO *)shmat(shm_id_info, NULL, 0);
    SM   = (struct ktp_entry *)shmat(shm_id_SM, NULL, 0);

    ktpInfo->ktp_fd = 0;
    ktpInfo->error = 0;
    ktpInfo->local_ip[0] = '\0';
    ktpInfo->local_port = 0;

    for (int i = 0; i < N; i++)
        SM[i].available = 1;

    semctl(sem1, 0, SETVAL, 0);
    semctl(sem2, 0, SETVAL, 0);
    semctl(sem_SM, 0, SETVAL, 1);
    semctl(sem_info, 0, SETVAL, 1);

    pthread_t tid_sender, tid_receiver, tid_garbage;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&tid_sender, &attr, S, NULL);
    pthread_create(&tid_receiver, &attr, R, NULL);
    pthread_create(&tid_garbage, &attr, G, NULL);
    while (1) {
        P(sem1);
        P(sem_info);
        if (ktpInfo->ktp_fd == 0 && ktpInfo->local_ip[0] == '\0' && ktpInfo->local_port == 0 && glob_error != ENOSPACE) {
            int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
            if (udp_fd == -1) {
                ktpInfo->ktp_fd = -1;
                ktpInfo->error = errno; // set by udp socket creation
            } else {
                ktpInfo->ktp_fd = udp_fd;
            }
        } else {
            struct sockaddr_in bindAddr;
            bindAddr.sin_family = AF_INET;
            bindAddr.sin_port = htons(ktpInfo->local_port);
            inet_aton(ktpInfo->local_ip, &bindAddr.sin_addr);
            if (bind(ktpInfo->ktp_fd, (struct sockaddr *)&bindAddr, sizeof(bindAddr)) < 0) {
                ktpInfo->ktp_fd = -1;
                ktpInfo->error = errno;
            }
        }
        V(sem_info);
        V(sem2);
    }

    return 0;
}
