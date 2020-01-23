#ifndef ARP_PACKET_H
#define ARP_PACKET_H

/* Recurring ARP packet constants */
#define REQUEST 1
#define REPLY 2
#define IP_LENGTH 4
#define MAC_ADDR_LENGTH 6
#define ARP_MIN 28

/* Ethernet Packet Prototype */
typedef struct ARP_packet
{
    u_char *frame;
    int frame_length;
    u_char destination_ip[4];
    u_char source_ip[4];
    u_char destination_mac[6];
    u_char source_mac[6];
    u_int16_t opcode;
} ARP_packet;

/* Ethernet helper functions */
void ARP_init(ARP_packet **packet_init, u_char *packet, int size);
void print_ARP_packet(ARP_packet *packet);
void free_arp_data(ARP_packet **packet);

#endif