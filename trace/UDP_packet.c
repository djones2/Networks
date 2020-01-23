#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <pcap.h>
#include "UDP_packet.h"

void UDP_init(UDP_packet **packet_init, u_char *packet, int size)
{

    /* Create personal packet for later use */
    UDP_packet *result;

    /* Return error if packet header caplen does not match ethernet minimum */
    if (size < UDP_SIZE)
    {
        exit(1);
    }

    /* Initialize parsable packet */
    result = calloc(1, sizeof(UDP_packet));

    /* Initialize ethernet packet frame fields */
    result->frame = calloc(size, sizeof(u_char));
    memcpy(result->frame, packet, size);

    /* Translate UDP header to host */
    result->source = ntohs(((u_int16_t *)packet)[0]);
    result->destination = ntohs(((u_int16_t *)packet)[1]);
    result->opcode = ntohs(((u_int16_t *)packet)[2]);

    /* Translate our result to the packet-to-be-initialized */
    *packet_init = result;
}

void print_UDP_packet(UDP_packet *packet)
{

    printf("\n\tUDP Header\n");
    printf("\t\tSource Port: : ");

    if (packet->source == FTP_PORT)
    {
        printf("FTP");
    }
    else if (packet->source == TELNET_PORT)
    {
        printf("Telnet");
    }
    else if (packet->source == SMTP_PORT)
    {
        printf("SMTP");
    }
    else if (packet->source == HTTP_PORT)
    {
        printf("HTTP");
    }
    else if (packet->source == POP3_PORT)
    {
        printf("POP3");
    }
    else
    {
        printf("%u", packet->source);
    }
    printf("\n\t\tDest Port: : ");
    if (packet->destination == FTP_PORT)
    {
        printf("FTP");
    }
    else if (packet->destination == TELNET_PORT)
    {
        printf("Telnet");
    }
    else if (packet->destination == SMTP_PORT)
    {
        printf("SMTP");
    }
    else if (packet->destination == HTTP_PORT)
    {
        printf("HTTP");
    }
    else if (packet->destination == POP3_PORT)
    {
        printf("POP3");
    }
    else
    {
        printf("%u", packet->destination);
    }
    printf("\n");
}

void free_udp_data(UDP_packet **packet)
{
    if ((*packet)->frame && *packet && packet)
    {
        free((*packet)->frame);
        free(*packet);
        *packet = NULL;
    }
}
