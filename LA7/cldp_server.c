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
#include <sys/time.h>      
#include <sys/sysinfo.h>   
#include <time.h>          

#define SERVER_PROTOCOL 253  // Server->Client
#define CLIENT_PROTOCOL 254  // Client->Server
#define MAX_PAYLOAD 1024
#define SERVER_IP "127.0.0.1"
#define CLIENT_IP "127.0.0.1"

// 8 byte header 
struct cldp_header {
    uint8_t msg_type;      // 0x01: HELLO, 0x02: QUERY, 0x03: RESPONSE - 1 byte
    uint8_t payload_len;   // Length of payload - 1 byte
    uint16_t txn_id;       // 2 byte
    uint32_t reserved;      // used to send some extra info - 4 byte 
};
// Helper function to get hostname
void get_hostname(char* buffer, size_t len) {
    gethostname(buffer, len);
}
// Helper function to get time_of_day
void get_time_of_day(char* buffer, size_t len) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    strftime(buffer, len, "%Y-%m-%d %H:%M:%S", localtime(&tv.tv_sec));
}
// Helper function to get cpu_load
float get_cpu_load() {
    struct sysinfo si;
    if (sysinfo(&si) != 0) {
        return -1.0f;  // Error case
    }
    
    // Calculate 1-minute load average percentage
    // Note: This gives system load average, not instantaneous CPU usage
    float load = (float)si.loads[0] / (1 << SI_LOAD_SHIFT);
    return load * 100.0f;  // Convert to percentage
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

    // Filling CLDP header fields 
    cldp->msg_type = msg_type;
    cldp->payload_len = payload ? strlen(payload) : 0;
    cldp->txn_id = txn_id;
    cldp->reserved = 0;
    if (payload) strcpy(packet + sizeof(struct iphdr) + sizeof(struct cldp_header), payload);

    // Filling IP header fields 
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
    // manually fill the IP header fields 
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
                char metadata[256] = {0};
                char temp[128];
                
                switch(cldp->reserved) {
                    case 0: // Hostname only
                        get_hostname(temp, sizeof(temp));
                        snprintf(metadata, sizeof(metadata), "Hostname: %s", temp);
                        break;
                        
                    case 1: // Time only
                        get_time_of_day(temp, sizeof(temp));
                        snprintf(metadata, sizeof(metadata), "Time: %s", temp);
                        break;
                        
                    case 2: // CPU only
                        snprintf(metadata, sizeof(metadata), "CPU: %.1f%%", get_cpu_load());
                        break;
                        
                    default: // All fields
                        get_hostname(temp, sizeof(temp));
                        snprintf(metadata, sizeof(metadata), "Hostname: %s, ", temp);
                        
                        get_time_of_day(temp, sizeof(temp));
                        strncat(metadata, "Time: ", sizeof(metadata)-strlen(metadata)-1);
                        strncat(metadata, temp, sizeof(metadata)-strlen(metadata)-1);
                        
                        char cpu_str[32];
                        snprintf(cpu_str, sizeof(cpu_str), ", CPU: %.1f%%", get_cpu_load());
                        strncat(metadata, cpu_str, sizeof(metadata)-strlen(metadata)-1);
                        break;
                }
            
                printf("Server: Sending RESPONSE\n");
                send_packet(sock, 0x03, cldp->txn_id, metadata, &sender_addr);
            }
        }
        sleep(10);
    }
    close(sock);
    return 0;
}
