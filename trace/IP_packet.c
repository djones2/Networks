#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <pcap.h>
#include "IP_packet.h"
#include "checksum.h"

void IP_init(IP_packet **packet_init, int size, u_char *packet)
{

    /* Create personal packet for later use */
    IP_packet *result;

    /* Return error if packet header caplen does not match IP minimum */
    if (size < IP_MIN)
    {
        perror("Invalid size");
        exit(1);
    }

    /* Initialize parsable packet */
    result = calloc(1, sizeof(IP_packet));

    /* Initialize ethernet packet frame fields */
    result->frame_length = size;
    result->frame = calloc(result->frame_length, sizeof(u_char));

    /* Move data between buffers, initialize data fields */
    memcpy(result->frame, packet, result->frame_length);
    // From lecture
    result->tos = result->frame[1];
    result->ttl = result->frame[8];
    result->protocol_type = result->frame[9];
    result->ihl = WORD_LENGTH * (result->frame[0] & 0x0F);
    result->length = ntohs(((u_int16_t *)packet)[1]);

    /* Checksum */
    result->checksum = in_cksum((u_int16_t *)result->frame, result->ihl);

    /* Move source and destination info */
    memcpy(result->source, result->frame + 12, WORD_LENGTH);
    memcpy(result->destination, result->frame + 16, WORD_LENGTH);

    /* Get content from data field */
    result->data_content = result->frame_length - result->ihl;
    result->data = calloc(result->data_content, sizeof(u_char));
    memcpy(result->data, result->frame + result->ihl, result->data_content);

    /* Translate our result to the packet-to-be-initialized */
    *packet_init = result;
}

void print_IP_packet(IP_packet *packet)
{
    /* Header info */
    printf("\n\tIP Header\n");
    printf("\t\tHeader Len: %d (bytes)\n", packet->ihl);
    printf("\t\tTOS: 0x%x\n", packet->tos);
    printf("\t\tTTL: %d\n", (int)(packet->ttl));
    printf("\t\tIP PDU Len: %d (bytes)", packet->length);
    /* Protocol info */
    printf("\n\t\tProtocol: ");

    if (packet->protocol_type == TCP_PROTOCOL)
    {
        printf("TCP");
    }
    else if (packet->protocol_type == UDP_PROTOCOL)
    {
        printf("UDP");
    }
    else if (packet->protocol_type == ICMP_PROTOCOL)
    {
        printf("ICMP");
    }
    else
    {
        printf("Unknown");
    }

    u_int16_t checksum1;
    u_int16_t temp;
    /* Checksum */
    checksum1 = ntohs(((u_int16_t *)packet->frame)[5]);
    temp = checksum1 >> 8;
    if (packet->checksum)
    {
        printf("\n\t\tChecksum: Incorrect (0x%02x%02x)", ((u_int8_t)checksum1), temp);
    }
    else
    {
        printf("\n\t\tChecksum: Correct (0x%01x%02x)", ((u_int8_t)checksum1), temp);
    }

    printf("\n\t\tSender IP: ");
    int i;
    for (i = 0; i < IP_LENGTH; i++)
    {
        if (i > 0)
        {
            printf(".");
        }
        printf("%d", (int)packet->source[i]);
    }

    printf("\n\t\tDest IP: ");

    for (i = 0; i < IP_LENGTH; i++)
    {
        if (i > 0)
        {
            printf(".");
        }
        printf("%d", (int)packet->destination[i]);
    }
    printf("\n");
}

u_char *IP_packet_data(IP_packet *packet)
{
    /* Initialize data, copy contents, return packet data only */
    u_char *packet_data;
    packet_data = calloc(packet->data_content, sizeof(u_char));
    memcpy(packet_data, packet->data, packet->data_content);
    return packet_data;
}

int IP_protocol_type(IP_packet *packet)
{
    return packet->protocol_type;
}

int IP_data_length(IP_packet *packet)
{
    return packet->data_content;
}

/* Construct the psuedo header */
u_char *IP_psuedo_header(struct IP_packet *packet)
{
    u_char *ph;
    ph = calloc(PSUEDO_HEADER_SIZE, sizeof(u_char));
    memcpy(ph, packet->source, IP_LENGTH);
    memcpy(ph + IP_LENGTH, packet->destination, IP_LENGTH);
    ph[9] = packet->protocol_type;
    ph[10] = (packet->data_content & 0xFF00) >> 8;
    ph[11] = packet->data_content & 0x00FF;
    return ph;
}

void free_ip_data(IP_packet **packet)
{
    if (packet && *packet && (*packet)->frame)
    {
        free((*packet)->frame);
        free((*packet)->data);
        free((*packet));
        (*packet) = NULL;
    }
}
