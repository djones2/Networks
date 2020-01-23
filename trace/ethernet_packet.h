#ifndef ETHERNET_PACKET_H
#define ETHERNET_PACKET_H

/* Recurring ethernet packet constants */
#define UNKNOWN_TYPE 0
#define IP_TYPE 0x0800
#define ARP_TYPE 0x0806
#define MAC_ADDR_LEN 6
#define ETHERNET_TYPE_BYTE 12
#define ETHERNET_FRAME 14

/* Ethernet Packet Prototype */
typedef struct ethernet_packet
{
    u_char *frame;
    int frame_length;
    u_char destination[6];
    u_char source[6];
    u_int16_t packet_type;
    u_char *data;
    int data_content;
} ethernet_packet;

/* Ethernet helper functions */
void ethernet_init(ethernet_packet **packet_init, struct pcap_pkthdr *packet_header, const u_char *packet);
void print_ethernet_packet(ethernet_packet *packet);
u_char *ethernet_packet_data(ethernet_packet *packet);
int ethernet_packet_type(ethernet_packet *packet);
int data_length(ethernet_packet *packet);
void free_ethernet_data(ethernet_packet **packet);

#endif