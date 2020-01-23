#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <pcap.h>
#include "ethernet_packet.h"

void ethernet_init(ethernet_packet **packet_init, struct pcap_pkthdr *packet_header, const u_char *packet)
{

    /* Create personal packet for later use */
    ethernet_packet *result;

    /* Return error if packet header caplen does not match ethernet minimum */
    if (packet_header->caplen < ETHERNET_FRAME)
    {
        exit(1);
    }

    /* Initialize parsable packet */
    result = calloc(1, sizeof(ethernet_packet));

    /* Initialize ethernet packet frame fields */
    result->frame_length = packet_header->caplen;
    result->frame = calloc(result->frame_length, sizeof(u_char));

    /* Move data between buffers, initialize data fields */
    memcpy(result->frame, packet, result->frame_length);

    memcpy(result->destination, result->frame, MAC_ADDR_LEN);

    memcpy(result->source, result->frame + MAC_ADDR_LEN, MAC_ADDR_LEN);
    /* Translate the packet type to host */
    result->packet_type = ntohs(((u_int16_t *)result->frame)[6]);

    result->data_content = result->frame_length - ETHERNET_FRAME;
    result->data = calloc(result->data_content, sizeof(u_char));
    memcpy(result->data, result->frame + ETHERNET_FRAME, result->data_content);

    /* Translate our result to the packet-to-be-initialized */
    *packet_init = result;
}

void print_ethernet_packet(ethernet_packet *packet)
{

    printf("\n\tEthernet Header\n");
    printf("\t\tDest MAC: ");

    int i;
    for (i = 0; i < MAC_ADDR_LEN; i++)
    {
        if (i > 0)
        {
            printf(":");
        }
        printf("%x", packet->destination[i]);
    }

    printf("\n\t\tSource MAC: ");

    for (i = 0; i < MAC_ADDR_LEN; i++)
    {
        // if (i)
        if (i > 0)
        {
            printf(":");
        }
        printf("%x", packet->source[i]);
    }

    printf("\n\t\tType: ");

    if (packet->packet_type == IP_TYPE)
    {
        printf("IP");
    }
    else if (packet->packet_type == ARP_TYPE)
    {
        printf("ARP");
    }
    else
    {
        printf("UNKNOWN");
    }
    printf("\n");
}

u_char *ethernet_packet_data(ethernet_packet *packet)
{
    /* Initialize data, copy contents, return packet data only */
    u_char *packet_data;
    packet_data = calloc(packet->data_content, sizeof(u_char));
    memcpy(packet_data, packet->data, packet->data_content);
    return packet_data;
}

int ethernet_packet_type(ethernet_packet *packet)
{
    return packet->packet_type;
}

int data_length(ethernet_packet *packet)
{
    return packet->data_content;
}

void free_ethernet_data(ethernet_packet **packet)
{
    if ((*packet)->frame && *packet && packet)
    {
        free((*packet)->frame);
        free(*packet);
        *packet = NULL;
    }
}
