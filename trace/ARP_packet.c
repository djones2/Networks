#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <pcap.h>
#include "ARP_packet.h"

void ARP_init(ARP_packet **packet_init, u_char *packet, int size)
{

    /* Create personal packet for later use */
    ARP_packet *result;

    /* Return error if packet header caplen does not match ethernet minimum */
    if (size < ARP_MIN)
    {
        exit(1);
    }

    /* Initialize parsable packet */
    result = calloc(1, sizeof(ARP_packet));

    /* Initialize ethernet packet frame fields */
    result->frame_length = size;
    result->frame = calloc(result->frame_length, sizeof(u_char));
    memcpy(result->frame, packet, result->frame_length);

    /* Translate opcode */
    result->opcode = result->frame[6] << 8 | result->frame[7];
    /* Move data between buffers, initialize data fields */
    memcpy(result->source_mac, result->frame + 8, MAC_ADDR_LENGTH);
    memcpy(result->source_ip, result->frame + 14, IP_LENGTH);
    memcpy(result->destination_mac, result->frame + 18, MAC_ADDR_LENGTH);
    memcpy(result->destination_ip, result->frame + 24, IP_LENGTH);

    /* Translate our result to the packet-to-be-initialized */
    *packet_init = result;
}

void print_ARP_packet(ARP_packet *packet)
{

    printf("\n\tARP header\n");
    printf("\t\tOpcode: ");

    if (packet->opcode == REPLY)
    {
        printf("Reply");
    }
    else if (packet->opcode == REQUEST)
    {
        printf("Request");
    }

    printf("\n\t\tSender MAC: ");
    int i;
    for (i = 0; i < MAC_ADDR_LENGTH; i++)
    {
        if (i > 0)
        {
            printf(":");
        }
        printf("%x", (int)(packet->source_mac[i]));
    }

    printf("\n\t\tSender IP: ");

    for (i = 0; i < IP_LENGTH; i++)
    {
        if (i > 0)
        {
            printf(".");
        }
        printf("%d", (int)(packet->source_ip[i]));
    }

    printf("\n\t\tTarget MAC: ");

    for (i = 0; i < MAC_ADDR_LENGTH; i++)
    {
        if (i > 0)
        {
            printf(":");
        }
        printf("%x", (int)(packet->destination_mac[i]));
    }

    printf("\n\t\tTarget IP: ");

    for (i = 0; i < IP_LENGTH; i++)
    {
        if (i > 0)
        {
            printf(".");
        }
        printf("%d", (int)(packet->destination_ip[i]));
    }

    printf("\n\n");
}

void free_arp_data(ARP_packet **packet)
{
    if ((*packet)->frame && *packet && packet)
    {
        free((*packet)->frame);
        free(*packet);
        *packet = NULL;
    }
}
