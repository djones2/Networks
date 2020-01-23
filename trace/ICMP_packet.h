#ifndef ICMP_PACKET_H
#define ICMP_PACKET_H

/* Recurring ICMP packet constants */
#define ECHO_REPLY 0
#define ECHO_REQUEST 8

/* ICMP helper functions */
int icmp_packet_type(u_char *packet);

#endif