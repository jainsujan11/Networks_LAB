/*
    ===================================== 
    Assignment 4 Submission 
    Name: Sujan Jain
    Roll number: 22CS10075 
    ===================================== 
*/


#ifndef KTP_SOCKET_H
#define KTP_SOCKET_H

#include <sys/time.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* --- Custom Socket Type and Protocol Parameters --- */
#define SOCK_KTP 3
#define ENOSPACE 10
#define ENOTBOUND 11
#define ENOMESSAGE 12

/* as mentioned in assg */
#define T       5
#define p     0.05
#define N     25

#define P(s)   semop(s, &pop, 1)
#define V(s)   semop(s, &vop, 1)

/* --- Window Structure --- */
struct wndw {
    int slots[256];
    /* 
       In the send window, -1 means the slot is free or the message has been acknowledged.
       In the receive window, -1 indicates that the slot is not expected.
    */
    int window_size;
    int start_seq; // Starting sequence number for this window.
};

/* --- Shared Memory Entry for Each KTP Socket --- */
struct ktp_entry {
    int available;                // 1 if free; 0 if allocated.
    pid_t process_id;             // Process ID that created this KTP socket.
    int udp_fd;                   // UDP socket file descriptor.
    char dest_ip[16];             // dest ip address
    uint16_t dest_port;           // dest port number.
    char send_buf[10][512];       // send buffer.
    int send_buf_sz;              // available buffer slots 
    int send_msg_length[10];      // Lengths for each message in the send buffer.
    char recv_buf[10][512];       // receive buffer.
    int recv_read_index;          // Next index from which the application will read.
    int recv_valid[10];           // Flags to indicate if a message in the receive buffer is valid.
    int recv_msg_length[10];      // Message lengths stored in the receive buffer.
    struct wndw swnd;             // Sender’s sliding window.
    struct wndw rwnd;             // Receiver’s sliding window.
    int nospace;                  // Flag set if no free space is available.
    time_t last_sent[256];        // Timestamp for each message in the send window.
    int transmission_cnt;         // storing number of transmissions 
};

/* --- Global Socket Information Structure --- */
typedef struct {
    int ktp_fd;
    char local_ip[16];
    uint16_t local_port;
    int error;
} KTP_INFO;

/* --- Global Variables (to be attached via shared memory) --- */
extern struct ktp_entry* SM;
extern KTP_INFO* ktpInfo;
extern int sem1, sem2;
extern int sem_info, sem_SM;
extern int shm_id_info, shm_id_SM;
extern struct sembuf pop, vop;

extern int glob_error; // used to store error 

/* --- Function Prototypes --- */
int k_socket(int domain, int type, int protocol);
int k_bind(int k_sockfd, char local_ip[], uint16_t local_port,
             char remote_ip[], uint16_t remote_port);
ssize_t k_sendto(int k_sockfd, const void *buffer, size_t length, int flags,
                   const struct sockaddr *dest_addr, socklen_t addrlen);
ssize_t k_recvfrom(int k_sockfd, void *buffer, size_t length, int flags,
                     struct sockaddr *src_addr, socklen_t *addrlen);
int k_close(int k_sockfd);
int dropMessage(float prob);

#endif
