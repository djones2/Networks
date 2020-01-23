#include <stdio.h>
#include "trace.h"

int ethernet_header(struct pcap_pkthdr *packet_header, const u_char *packet, u_char **pdu, int *length)
{
    struct ethernet_packet *new_packet;
    int new_packet_type;

    new_packet_type = UNKNOWN;

    /* Initialize ethernet packet */
    ethernet_init(&new_packet, packet_header, packet);

    /* Print details of initialized packet */
    print_ethernet_packet(new_packet);

    /* Initialize PDU with data and data length*/
    *pdu = ethernet_packet_data(new_packet);
    *length = data_length(new_packet);

    /* Determine packet type */
    if (ethernet_packet_type(new_packet) == ARP_TYPE)
    {
        new_packet_type = ARP;
    }
    else if (ethernet_packet_type(new_packet) == IP_TYPE)
    {
        new_packet_type = IP;
    }

    /* Free allocated data */
    free_ethernet_data(&new_packet);

    /* Return packet type */
    return new_packet_type;
}

int ip_header(u_char **pdu, int *length, u_char **psuedo_header)
{
    struct IP_packet *new_packet;
    int new_packet_type = PAYLOAD;

    /* Initialize ethernet packet */
    IP_init(&new_packet, *length, *pdu);

    /* Print details of initialized packet */
    print_IP_packet(new_packet);

    free(*pdu);

    /* Initialize PDU with data and data length*/
    *pdu = IP_packet_data(new_packet);
    *length = IP_data_length(new_packet);
    *psuedo_header = IP_psuedo_header(new_packet);

    /* Determine packet type */
    if (IP_protocol_type(new_packet) == ICMP_PROTOCOL)
    {
        new_packet_type = ICMP;
    }
    else if (IP_protocol_type(new_packet) == UDP_PROTOCOL)
    {
        new_packet_type = UDP;
    }
    else if (IP_protocol_type(new_packet) == TCP_PROTOCOL)
    {
        new_packet_type = TCP;
    }

    /* Free allocated data */
    free_ip_data(&new_packet);

    /* Return packet type */
    return new_packet_type;
}

int arp_header(u_char **pdu, int *length)
{
    /* Create packet prototype */
    struct ARP_packet *new_packet;
    int new_packet_type = PAYLOAD;

    /* Initialize and detail packet data */
    ARP_init(&new_packet, *pdu, *length);
    print_ARP_packet(new_packet);
    /* Free occupied memory */
    free(*pdu);
    free_arp_data(&new_packet);

    /* Return PAYLOAD */
    return new_packet_type;
}

int udp_header(u_char **pdu, int *length)
{
    /* Create packet prototype */
    struct UDP_packet *new_packet;
    int new_packet_type = PAYLOAD;

    /* Initialize and detail packet data */
    UDP_init(&new_packet, *pdu, *length);
    print_UDP_packet(new_packet);
    /* Free occupied memory */
    free(*pdu);
    free_udp_data(&new_packet);

    /* Return PAYLOAD */
    return new_packet_type;
}

int tcp_header(u_char **pdu, int *length, u_char **psuedo_header)
{
    /* Create packet prototype */
    struct TCP_packet *new_packet;
    int new_packet_type = PAYLOAD;

    /* Initialize and detail packet data */
    TCP_init(&new_packet, *pdu, *length, *psuedo_header);
    print_TCP_packet(new_packet);
    /* Free occupied memory */
    free(*pdu);
    free_TCP_data(&new_packet);

    /* Return PAYLOAD */
    return new_packet_type;
}

int icmp_header(u_char **pdu)
{
    printf("\n\tICMP Header\n");
    /* Only need to find echo type */
    if (icmp_packet_type(*pdu) == ECHO_REPLY)
    {
        printf("\t\tType: Reply\n");
    }
    else if (icmp_packet_type(*pdu) == ECHO_REQUEST)
    {
        printf("\t\tType: Request\n");
    }
    else if (icmp_packet_type(*pdu) == 109)
    {
        printf("\t\tType: 109\n");
    }
    else
    {
        printf("\t\tType: Unknown\n");
    }
    free(*pdu);
    return PAYLOAD;
}

int main(int argc, char **argv)
{
    /* CLA check */
    if (argc < 2)
    {
        printf("Please provide a .pcap file\n");
        exit(1);
    }

    /* Fields to be generated across pcap files */
    int i = 1;
    int next_packet_type;
    int pdu_length;
    u_char *pdu;
    u_char *psuedo_header;
    const u_char *current_packet;
    struct pcap_pkthdr *header;
    pcap_t *pcap_file;

    /* Open provided .pcap file */
    char buff[PCAP_ERRBUF_SIZE];
    pcap_file = pcap_open_offline(argv[1], buff);
    if (!pcap_file)
    {
        printf("%s\n", buff);
        exit(1);
    }

    /* Open packets from .pcap */
    while (pcap_next_ex(pcap_file, &header, &current_packet) == 1)
    {
        printf("\nPacket number: %d", i);
        printf("  Frame Len: %d\n", header->caplen);
        next_packet_type = ethernet_header(header, current_packet, &pdu, &pdu_length);

        while (next_packet_type != PAYLOAD && next_packet_type != UNKNOWN)
        {
            if (next_packet_type == IP)
            {
                next_packet_type = ip_header(&pdu, &pdu_length, &psuedo_header);
            }
            else if (next_packet_type == ARP)
            {
                next_packet_type = arp_header(&pdu, &pdu_length);
            }
            else if (next_packet_type == UDP)
            {
                next_packet_type = udp_header(&pdu, &pdu_length);
            }
            else if (next_packet_type == TCP)
            {
                next_packet_type = tcp_header(&pdu, &pdu_length, &psuedo_header);
            }
            else if (next_packet_type == ICMP)
            {
                next_packet_type = icmp_header(&pdu);
            }
            else if (next_packet_type == UNKNOWN)
            {
                printf("Unknown PDU");
            }
        }
        i++;
    }
    /* Successful Trace */
    return 0;
}