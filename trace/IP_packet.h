#ifndef IP_PACKET_H
#define IP_PACKET_H

/* Recurring ethernet packet constants */
#define ICMP_PROTOCOL 1
#define WORD_LENGTH 4
#define IP_LENGTH 4
#define TCP_PROTOCOL 6
#define PSUEDO_HEADER_SIZE 12
#define UDP_PROTOCOL 17
#define IP_MIN 20

/* IP Packet Prototype */
typedef struct IP_packet
{
    u_char *frame;
    int frame_length;
    u_char tos;
    u_char ttl;
    u_char ihl;
    u_char protocol_type;
    u_char destination[4];
    u_char source[4];
    u_int16_t length;
    u_int16_t checksum;
    u_char *data;
    int data_content;
} IP_packet;

/* IP helper functions */
void IP_init(IP_packet **packet_init, int size, u_char *packet);
void print_IP_packet(IP_packet *packet);
u_char *IP_packet_data(IP_packet *packet);
int IP_protocol_type(IP_packet *packet);
int IP_data_length(IP_packet *packet);
u_char *IP_psuedo_header(struct IP_packet *packet);
void free_ip_data(IP_packet **packet);

#endif