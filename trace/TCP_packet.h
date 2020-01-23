#ifndef TCP_PACKET_H
#define TCP_PACKET_H

/* Recurring TCP packet constants */
#define FTP_PORT 21
#define SSH_PORT 22
#define TELNET_PORT 23
#define SMTP_PORT 25
#define UDP_SIZE 28
#define HTTP_PORT 80
#define POP3_PORT 110
#define FIN 0x1
#define SYN 0x2
#define RST 0x4
#define ACK 0x10

/* TCP Packet Prototype */
typedef struct TCP_packet
{
    u_char *frame;
    u_char psuedo_header[12];
    u_char flags;
    int frame_length;
    u_int16_t destination;
    u_int16_t source;
    u_int16_t window;
    u_int16_t chksm;
    u_int32_t ack;
    u_int32_t seq;
} TCP_packet;

/* TCP helper functions */
void tcp_hand_checksum(TCP_packet *packet);
void TCP_init(TCP_packet **packet_init, u_char *packet, int size, u_char *ph);
void print_TCP_packet(TCP_packet *packet);
void free_TCP_data(TCP_packet **packet);

#endif