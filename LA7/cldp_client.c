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

#define SERVER_PROTOCOL 253
#define CLIENT_PROTOCOL 254
#define MAX_PAYLOAD 1024

// 8 byte header 
struct cldp_header {
    uint8_t msg_type;      // 0x01: HELLO, 0x02: QUERY, 0x03: RESPONSE - 1 byte
    uint8_t payload_len;   // Length of payload - 1 byte
    uint16_t txn_id;       // 2 byte
    uint32_t reserved;      // used to send some extra info - 4 byte 
};
 
unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned int sum = 0;
    for (sum = 0; len > 1; len -= 2) sum += *buf++;
    if (len == 1) sum += *(unsigned char *)buf;
    return (unsigned short)(~sum);
}

void send_packet(int sock, uint8_t msg_type, uint16_t txn_id, uint32_t reserved, const char *payload, struct sockaddr_in *dest) {
    char packet[sizeof(struct iphdr) + sizeof(struct cldp_header) + MAX_PAYLOAD];
    struct iphdr *ip = (struct iphdr *)packet;
    struct cldp_header *cldp = (struct cldp_header *)(packet + sizeof(struct iphdr));

    // CLDP Header
    cldp->msg_type = msg_type;
    cldp->payload_len = payload ? strlen(payload) : 0;
    cldp->txn_id = txn_id;
    cldp->reserved = reserved;
    if (payload) strcpy(packet + sizeof(struct iphdr) + sizeof(struct cldp_header), payload);

    // IP Header
    ip->version = 4;
    ip->ihl = 5;
    ip->tos = 0;
    ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct cldp_header) + cldp->payload_len);
    ip->id = htons(54321);
    ip->frag_off = 0;
    ip->ttl = 64;
    ip->protocol = CLIENT_PROTOCOL;
    ip->check = 0;
    ip->saddr = inet_addr("127.0.0.1");
    ip->daddr = dest->sin_addr.s_addr;
    ip->check = checksum((unsigned short *)ip, sizeof(struct iphdr));

    sendto(sock, packet, ntohs(ip->tot_len), 0, (struct sockaddr *)dest, sizeof(*dest));
}

int main() {
    int sock = socket(AF_INET, SOCK_RAW, SERVER_PROTOCOL);
    // set broadcast field 
    int broadcast = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    int opt = 1;
    setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("255.255.255.255"); // to send broadcast
    while(1){
        // Step 1: Wait for HELLO from server
        printf("Client: Waiting for HELLO...\n");
        while (1) {
            // just to make sure correct message is received 
            char buffer[2048];
            struct sockaddr_in sender_addr;
            socklen_t addr_len = sizeof(sender_addr);
            int bytes = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&sender_addr, &addr_len);
            if (bytes < 0) {
                perror("recvfrom");
                continue;
            }
            
            struct iphdr *ip = (struct iphdr *)buffer;
            if (ip->protocol == SERVER_PROTOCOL) {
                struct cldp_header *cldp = (struct cldp_header *)(buffer + (ip->ihl * 4));
                if (cldp->msg_type == 0x01) {  // HELLO
                    printf("Client: Received HELLO from server with IP %s\n",inet_ntoa(sender_addr.sin_addr));
                    break;
                }
            }
        }

        // Step 2: Send QUERY
        printf("Client: Sending QUERY\n");
        // change the values here to ask for a particular meta data 
        send_packet(sock, 0x02, 5678, 4,"", &server_addr);

        // Step 3: Wait for RESPONSE
        printf("Client: Waiting for RESPONSE...\n");
        while (1) {
            char buffer[2048];
            struct sockaddr_in sender_addr;
            socklen_t addr_len = sizeof(sender_addr);

            int bytes = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&sender_addr, &addr_len);
            if (bytes < 0) {
                perror("recvfrom");
                continue;
            }

            struct iphdr *ip = (struct iphdr *)buffer;
            if (ip->protocol == SERVER_PROTOCOL) {
                struct cldp_header *cldp = (struct cldp_header *)(buffer + (ip->ihl * 4));
                char *payload = buffer + (ip->ihl * 4) + sizeof(struct cldp_header);

                if (cldp->msg_type == 0x03) {  // RESPONSE
                    printf("Client: Received RESPONSE: %.*s from IP %s\n", cldp->payload_len, payload, inet_ntoa(sender_addr.sin_addr));
                    break;
                }
            }
        }
    }
    close(sock);
    return 0;
}
