/*
    ===================================== 
    Assignment 7 Submission 
    Name: Sujan Jain
    Roll number: 22CS10075 
    ===================================== 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SERVER_PROTOCOL 253  // Server->Client
#define CLIENT_PROTOCOL 254  // Client->Server
#define MAX_PAYLOAD 1024
#define SERVER_IP "127.0.0.1"
#define CLIENT_IP "127.0.0.1"

struct cldp_header {
    uint8_t msg_type;      // 0x01: HELLO, 0x02: QUERY, 0x03: RESPONSE
    uint8_t payload_len;   // Length of payload
    uint16_t txn_id;       // Transaction ID
    uint32_t reserved;
};

float get_cpu_load() {
    FILE* fp = fopen("/proc/stat", "r");
    if (!fp) return -1.0f;

    char buffer[256];
    fgets(buffer, sizeof(buffer), fp);
    fclose(fp);

    unsigned long user, nice, system, idle;
    sscanf(buffer, "cpu %lu %lu %lu %lu", &user, &nice, &system, &idle);

    unsigned long total = user + nice + system + idle;
    unsigned long idle_percent = (idle * 100) / total;
    return 100.0f - idle_percent;
}

unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned int sum = 0;
    for (sum = 0; len > 1; len -= 2) sum += *buf++;
    if (len == 1) sum += *(unsigned char *)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    return (unsigned short)(~sum);
}

void send_packet(int sock, uint8_t msg_type, uint16_t txn_id, const char *payload, struct sockaddr_in *dest) {
    char packet[sizeof(struct iphdr) + sizeof(struct cldp_header) + MAX_PAYLOAD];
    struct iphdr *ip = (struct iphdr *)packet;
    struct cldp_header *cldp = (struct cldp_header *)(packet + sizeof(struct iphdr));

    // CLDP Header
    cldp->msg_type = msg_type;
    cldp->payload_len = payload ? strlen(payload) : 0;
    cldp->txn_id = txn_id;
    cldp->reserved = 0;
    if (payload) strcpy(packet + sizeof(struct iphdr) + sizeof(struct cldp_header), payload);

    // IP Header
    ip->version = 4;
    ip->ihl = 5;
    ip->tos = 0;
    ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct cldp_header) + cldp->payload_len);
    ip->id = htons(54321);
    ip->frag_off = 0;
    ip->ttl = 64;
    ip->protocol = SERVER_PROTOCOL;
    ip->check = 0;
    ip->saddr = inet_addr(SERVER_IP);
    ip->daddr = dest->sin_addr.s_addr;
    ip->check = checksum((unsigned short *)ip, sizeof(struct iphdr));
    sendto(sock, packet, ntohs(ip->tot_len), 0, (struct sockaddr *)dest, sizeof(*dest));
}



int main(int argc, char const *argv[])
{
    int sock = socket(AF_INET, SOCK_RAW, CLIENT_PROTOCOL);
    if (sock < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &opt, sizeof(opt));

    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = inet_addr(CLIENT_IP);
    while(1){
        // Step 1: Server announces itself (HELLO)
        printf("Server: Sending HELLO\n");
        send_packet(sock, 0x01, 1234, "SERVER_ANNOUNCE", &client_addr);

        char buffer[2048];
        struct sockaddr_in sender_addr;
        socklen_t addr_len = sizeof(sender_addr);

        int bytes = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&sender_addr, &addr_len);
        if (bytes < 0) {
            perror("recvfrom");
        }

        struct iphdr *ip = (struct iphdr *)buffer;
        if (ip->protocol == CLIENT_PROTOCOL) {
            struct cldp_header *cldp = (struct cldp_header *)(buffer + (ip->ihl * 4));
            char *payload = buffer + (ip->ihl * 4) + sizeof(struct cldp_header);

            printf("Server: Received packet (Type=0x%02X, TxnID=%d)\n", cldp->msg_type, cldp->txn_id);

            if (cldp->msg_type == 0x02) {  // QUERY
                struct utsname sys_info;
                uname(&sys_info);
                char metadata[256];
                float cpu_load = get_cpu_load();
                snprintf(metadata, sizeof(metadata), 
                    "Hostname:%s, OS:%s, CPU:%.1f%%", 
                    sys_info.nodename, 
                    sys_info.sysname,
                    cpu_load);
                printf("Server: Sending RESPONSE\n");
                send_packet(sock, 0x03, cldp->txn_id, metadata, &sender_addr);
            }
        }
        sleep(10);
    }
        close(sock);
    return 0;
}
