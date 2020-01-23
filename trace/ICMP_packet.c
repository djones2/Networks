#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <pcap.h>
#include "ICMP_packet.h"
#include "checksum.h"

int icmp_packet_type(u_char *packet)
{
    return (int)(*packet);
}