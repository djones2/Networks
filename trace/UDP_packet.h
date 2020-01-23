#ifndef UDP_PACKET_H
#define UDP_PACKET_H

/* Recurring UDP packet constants */
#define PORT_LENGTH 2
#define FTP_PORT 21
#define SSH_PORT 22
#define TELNET_PORT 23
#define SMTP_PORT 25
#define UDP_SIZE 28
#define HTTP_PORT 80
#define POP3_PORT 110

/* UDP Packet Prototype */
typedef struct UDP_packet
{
    u_char *frame;
    u_int16_t destination;
    u_int16_t source;
    u_int16_t opcode;
} UDP_packet;

/* UDP helper functions */
void UDP_init(UDP_packet **packet_init, u_char *packet, int size);
void print_UDP_packet(UDP_packet *packet);
void free_udp_data(UDP_packet **packet);

#endif