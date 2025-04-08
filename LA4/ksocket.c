/*
    ===================================== 
    Assignment 4 Submission 
    Name: Sujan Jain
    Roll number: 22CS10075 
    ===================================== 
*/


#include "ksocket.h"
#include <assert.h>

/* --- Global Variables --- */
struct ktp_entry* SM;
KTP_INFO* ktpInfo;
int sem1, sem2;
int sem_info, sem_SM;
int shm_id_info, shm_id_SM;
struct sembuf pop, vop;
int glob_error = 0;


void initialize() {
    /* Generate keys (using "/" as the pathname) */
    int key_shm_info   = ftok("/", 65);
    int key_shm_SM  = ftok("/", 66);
    int key_sem1   = ftok("/", 67);
    int key_sem2    = ftok("/", 68);
    int key_sem_SM  = ftok("/", 69);
    int key_sem_info   = ftok("/", 70);

    /* Get shared memory and semaphore IDs (assumed to be created by an init process) */
    shm_id_info  = shmget(key_shm_info, sizeof(KTP_INFO), 0666);
    shm_id_SM = shmget(key_shm_SM, sizeof(struct ktp_entry) * N, 0666);
    sem1     = semget(key_sem1, 1, 0666);
    sem2      = semget(key_sem2, 1, 0666);
    sem_info     = semget(key_sem_info, 1, 0666);
    sem_SM    = semget(key_sem_SM, 1, 0666);

    ktpInfo = (KTP_INFO *)shmat(shm_id_info, NULL, 0);
    SM   = (struct ktp_entry *)shmat(shm_id_SM, NULL, 0);

    /* Initialize semaphore operation structures */
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1;
    vop.sem_op = 1;
}

int find_free_slot() {
    P(sem_SM);
    for (int i = 0; i < N; i++) {
        if (SM[i].available == 1) {
            V(sem_SM);
            return i;
        }
    }
    V(sem_SM);
    return -1;
}

int k_socket(int domain, int type, int protocol) {
    initialize();

    if (type != SOCK_KTP) {
        errno = EINVAL;
        return -1;
    }

    P(sem_info);
    int index = find_free_slot();
    if (index == -1) {
        glob_error = ENOSPACE; // No free index available
        errno = ENOBUFS;
        ktpInfo->error = errno;
        ktpInfo->ktp_fd = 0;
        ktpInfo->local_ip[0] = '\0';
        ktpInfo->local_port = 0;
        V(sem_info);
        return -1;
    }
    V(sem_info);

    /* signal the main function in initksocket to create the UDP socket */
    V(sem1);

    /*wait on this to ensure mutual exclusion*/
    P(sem2);

    P(sem_info);
    if (ktpInfo->ktp_fd == -1) {
        errno = ktpInfo->error;
        ktpInfo->ktp_fd = 0;
        ktpInfo->error = 0;
        ktpInfo->local_ip[0] = '\0';
        ktpInfo->local_port = 0;
        V(sem_info);
        return -1;
    }
    V(sem_info);

    /* Initialize the KTP entry */
    P(sem_SM);
    SM[index].available = 0;
    SM[index].process_id = getpid();
    SM[index].udp_fd = ktpInfo->ktp_fd;
    for (int i = 0; i < 10; i++)
    {
        memset(SM[index].send_buf[i], '\0', sizeof(SM[index].send_buf[i]));
        memset(SM[index].recv_buf[i], '\0', sizeof(SM[index].recv_buf[i]));
    }
    
    for (int j = 0; j < 256; j++) {
        SM[index].swnd.slots[j] = -1;
        SM[index].last_sent[j] = -1;
        if (j > 0 && j < 11)
            SM[index].rwnd.slots[j] = j - 1; // [0-9]
        else
            SM[index].rwnd.slots[j] = -1;
    }
    SM[index].swnd.window_size = SM[index].rwnd.window_size = 10;
    SM[index].swnd.start_seq = SM[index].rwnd.start_seq = 1;
    SM[index].send_buf_sz = 10;
    for (int j = 0; j < 10; j++)
        SM[index].recv_valid[j] = 0;
    SM[index].recv_read_index = 0;
    SM[index].nospace = 0;
    SM[index].transmission_cnt = 0;
    V(sem_SM);

    /* Reset the global socket info */
    P(sem_info);
    ktpInfo->ktp_fd = 0;
    ktpInfo->error = 0;
    ktpInfo->local_ip[0] = '\0';
    ktpInfo->local_port = 0;
    V(sem_info);

    return index;
}

int k_bind(int k_sockfd, char local_ip[], uint16_t local_port,
             char dest_ip[], uint16_t dest_port) {
    initialize();
    P(sem_SM);

    P(sem_info);
    /* this will not happen anyhow */
    if (k_sockfd < 0 || k_sockfd >= N) {
        printf("Invalid socket index\n");
        errno = ENOBUFS;
        ktpInfo->ktp_fd = 0;
        ktpInfo->error = 0;
        ktpInfo->local_ip[0] = '\0';
        ktpInfo->local_port = 0;
        V(sem_SM);
        V(sem_info);
        return -1;
    }

    /* Set local binding info */
    ktpInfo->ktp_fd = SM[k_sockfd].udp_fd;
    strncpy(ktpInfo->local_ip, local_ip, sizeof(ktpInfo->local_ip));
    ktpInfo->local_port = local_port;
    V(sem_info);

    /* signal the main function in initksocket to create the UDP socket */
    V(sem1);
    P(sem2);

    P(sem_info);
    if (ktpInfo->ktp_fd == -1) {
        errno = ktpInfo->error;
        ktpInfo->ktp_fd = 0;
        ktpInfo->error = 0;
        ktpInfo->local_ip[0] = '\0';
        ktpInfo->local_port = 0;
        V(sem_info);
        V(sem_SM);
        return -1;
    }
    V(sem_info);

    /* Save remote address information */
    strncpy(SM[k_sockfd].dest_ip, dest_ip, sizeof(SM[k_sockfd].dest_ip));
    SM[k_sockfd].dest_port = dest_port;
    V(sem_SM);

    P(sem_info);
    ktpInfo->ktp_fd = 0;
    ktpInfo->error = 0;
    ktpInfo->local_ip[0] = '\0';
    ktpInfo->local_port = 0;
    V(sem_info);

    return 0;
}

ssize_t k_sendto(int k_sockfd, const void *buffer, size_t length, int flags,
                   const struct sockaddr *dest_addr, socklen_t addrlen) {
    initialize();
    P(sem_SM);
    char *target_ip = inet_ntoa(((struct sockaddr_in *)dest_addr)->sin_addr);
    uint16_t target_port = ntohs(((struct sockaddr_in *)dest_addr)->sin_port);

    if (strcmp(SM[k_sockfd].dest_ip, target_ip) != 0 ||
        SM[k_sockfd].dest_port != target_port) {
        glob_error = ENOTBOUND;
        errno = ENOTCONN;
        V(sem_SM);
        return -1;
    }
    if (SM[k_sockfd].send_buf_sz == 0) {
        glob_error = ENOSPACE;
        errno = ENOBUFS;
        V(sem_SM);
        return -1;
    }

    int seq = SM[k_sockfd].swnd.start_seq;
    while (SM[k_sockfd].swnd.slots[seq] != -1)
        seq = (seq + 1) % 256;

    int buf_idx = 0;
    int freeFound = 0;
    for (buf_idx = 0; buf_idx < 10; buf_idx++) {
        freeFound = 1;
        for (int i = 0; i < 256; i++) {
            if (SM[k_sockfd].swnd.slots[i] == buf_idx) {
                freeFound = 0;
                break;
            }
        }
        if (freeFound)
            break;
    }
    if (!freeFound) {
        errno = ENOBUFS;
        V(sem_SM);
        return -1;
    }
    // stores the buffer index in the send window
    SM[k_sockfd].swnd.slots[seq] = buf_idx;
    memcpy(SM[k_sockfd].send_buf[buf_idx], buffer, length);
    SM[k_sockfd].send_buf[buf_idx][length] = '\0';
    SM[k_sockfd].last_sent[seq] = -1;
    SM[k_sockfd].send_buf_sz--;
    SM[k_sockfd].send_msg_length[buf_idx] = length;
    V(sem_SM);
    return length;
}

ssize_t k_recvfrom(int k_sockfd, void *buffer, size_t length, int flags,
                     struct sockaddr *src_addr, socklen_t *addrlen) {
    initialize();
    P(sem_SM);
    if (SM == NULL) {
        errno = ENOMEM;
        printf("run initksocket file.\n");
        V(sem_SM);
        return -1;
    }
    // accessing shared memory 
    struct ktp_entry *entry = &SM[k_sockfd];
    if (entry->recv_valid[entry->recv_read_index]) {
        entry->recv_valid[entry->recv_read_index] = 0;
        entry->rwnd.window_size++;
        int seq = -1;
        for (int i = 0; i < 256; i++) {
            if (entry->rwnd.slots[i] == entry->recv_read_index) {
                seq = i;
                break;
            }
        }
        entry->rwnd.slots[seq] = -1;
        entry->rwnd.slots[(seq + 10) % 256] = entry->recv_read_index;
        int msg_len = entry->recv_msg_length[entry->recv_read_index];
        memcpy(buffer, entry->recv_buf[entry->recv_read_index],
               (length < msg_len) ? length : msg_len);
        entry->recv_read_index = (entry->recv_read_index + 1) % 10;
        V(sem_SM);
        return (length < msg_len) ? length : msg_len;
    }
    glob_error = ENOMESSAGE;
    errno = ENOMSG;
    V(sem_SM);
    return -1;
}

int k_close(int k_sockfd) {
    initialize();
    P(sem_SM);
    SM[k_sockfd].available = 1;
    printf("Total number of transmissions = %d\n", SM[k_sockfd].transmission_cnt);
    V(sem_SM);
    return 0;
}

/* --- Simulate an Unreliable Link by Dropping a Message --- */
int dropMessage(float prob) {
    float rnd = (float)rand() / (float)RAND_MAX;
    return (rnd < prob) ? 1 : 0;
}