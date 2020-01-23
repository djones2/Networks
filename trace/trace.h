/* Standard libraries */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcap.h>

/* Programs for parsing specific 
    packet types */
#include "ARP_packet.h"
#include "ethernet_packet.h"
#include "ICMP_packet.h"
#include "IP_packet.h"
#include "TCP_packet.h"
#include "UDP_packet.h"

/* Define each header type */
#define ETHERNET 0
#define IP 1
#define ARP 2
#define UDP 3
#define TCP 4
#define ICMP 5
#define UNKNOWN 6
#define PAYLOAD 7

/* Functions for reading packet headers
    based on type */
int ethernet_header(struct pcap_pkthdr *packet_header, const u_char *packet, u_char **pdu, int *length);
int ip_header(u_char **pdu, int *length, u_char **psuedo_header);
int arp_header(u_char **pdu, int *length);
int udp_header(u_char **pdu, int *length);
int tcp_header(u_char **pdu, int *length, u_char **psuedo_header);
int icmp_header(u_char **pdu);
