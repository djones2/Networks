#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <pcap.h>
#include "TCP_packet.h"
#include "checksum.h"

int checksum_flag = 0;

void TCP_init(TCP_packet **packet_init, u_char *packet, int size, u_char *ph)
{

    /* Create personal packet for later use */
    TCP_packet *result;

    /* Initialize parsable packet */
    result = calloc(1, sizeof(TCP_packet));
    u_char *checksum;

    /* Initialize ethernet packet frame fields */
    result->frame_length = size;
    result->frame = calloc(result->frame_length, sizeof(u_char));
    memcpy(result->frame, packet, result->frame_length);
    memcpy(result->psuedo_header, ph, 12);
    result->psuedo_header[11] = size & 0x00FF;

    /* Translate remaining data to frame fields */
    result->source = ntohs(*((u_int16_t *)result->frame));
    result->destination = ntohs(((u_int16_t *)result->frame)[1]);
    result->seq = ntohl(((u_int32_t *)result->frame)[1]);
    result->ack = ntohl(((u_int32_t *)result->frame)[2]);
    result->window = ntohs(((u_int16_t *)result->frame)[7]);
    result->flags = result->frame[13];
    checksum = calloc(result->frame_length + 12, sizeof(u_char));

    /* Call checksum on assembled packet */
    result->chksm = in_cksum((u_int16_t *)checksum, result->frame_length + 12);

    /* Translate our result to the packet-to-be-initialized */
    *packet_init = result;
}

void print_TCP_packet(TCP_packet *packet)
{

    printf("\n\tTCP Header\n");
    printf("\t\tSource Port:");

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
        printf("  HTTP");
    }
    else if (packet->source == POP3_PORT)
    {
        printf("POP3");
    }
    else
    {
        printf(" : %u", packet->source);
    }
    printf("\n\t\tDest Port: ");
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
        printf(" HTTP");
    }
    else if (packet->destination == POP3_PORT)
    {
        printf("POP3");
    }
    else
    {
        printf(": %u", packet->destination);
    }
    printf("\n\t\tSequence Number: %u", packet->seq);
    if (packet->flags & ACK)
    {
        printf("\n\t\tACK Number: %u", packet->ack);
        printf("\n\t\tACK Flag: Yes");
    }
    else
    {
        printf("\n\t\tACK Number: <not valid>");
        printf("\n\t\tACK Flag: No");
    }

    if (packet->flags & SYN)
    {
        printf("\n\t\tSYN Flag: Yes");
    }
    else
    {
        printf("\n\t\tSYN Flag: No");
    }

    if (packet->flags & RST)
    {
        printf("\n\t\tRST Flag: Yes");
    }
    else
    {
        printf("\n\t\tRST Flag: No");
    }

    if (packet->flags & FIN)
    {
        printf("\n\t\tFIN Flag: Yes");
    }
    else
    {
        printf("\n\t\tFIN Flag: No");
    }

    printf("\n\t\tWindow Size: %u", packet->window);
    u_int16_t checksum1 = ntohs(((u_int16_t *)packet->frame)[8]);
    if (packet->chksm == 0) {
        printf("\n\t\tChecksum: Incorrect (0x%x)\n", checksum1);
    } 
    else {
        printf("\n\t\tChecksum: Correct (0x%x)\n", checksum1);
    }
}

void free_TCP_data(TCP_packet **packet)
{
    if ((*packet)->frame && *packet && packet)
    {
        free((*packet)->frame);
        free(*packet);
        *packet = NULL;
    }
}
