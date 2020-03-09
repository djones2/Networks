#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#include "networks.h"
#include "cpe464.h"
#include "gethostbyname.h"
#include "networks.h"

// Send a packet at a time. Adopted from origiginal but modified for Packet type.
int32_t send_buf(uint8_t *data, uint32_t len, Connection *connection, uint8_t flag,
				 uint32_t seq_num)
{
	Packet packet;
	int32_t sendingLen = 0;

	// Populate fields
	packet.seq_num = seq_num;
	packet.flag = flag;
	packet.size = len + HEADER_LENGTH;

	// Copy data, create packet
	memcpy(packet.data, data, len);
	create_packet(&packet);
	// Send packet through the buffer
	sendingLen = safeSendTo(&packet, connection);
	// Return length sent
	return sendingLen;
}

// Create a packet and populate it's fields, ready for sending.
// Adopted from createHeader
void create_packet(Packet *packet)
{
	int i;
	uint8_t *out_packet = packet->full_packet;
	// clear fields
	for (i = 0; i < HEADER_LENGTH + MAX_LEN; i++)
	{
		out_packet[i] = 0;
	}
	// sequence number in network order
	((uint32_t *)out_packet)[0] = htonl(packet->seq_num);
	// flag
	out_packet[6] = packet->flag;
	// translate the size to network order
	*((uint32_t *)(out_packet + 7)) = htonl(packet->size);
	// copy data
	memcpy(out_packet + HEADER_LENGTH, &packet->data, MAX_LEN);
	// populate checksum field
	packet->checksum = in_cksum((uint16_t *)out_packet, packet->size);
	((int16_t *)out_packet)[2] = packet->checksum;
}

// Send a packet that is already constructed or an RR
int32_t send_additional_packet(Packet *packet, Connection *connection)
{
	int32_t sendingLen = 0;
	// Send the packet constructed via the connection passed
	sendingLen = safeSendTo(packet, connection);
	return sendingLen;
}

// Receive a packet. Changed from retrieving a buffer to a packet
int32_t recv_buf(Packet *packet, int32_t recv_sk_num, Connection *connection)
{
	int32_t recv_len = 0;
	uint32_t remote_len = sizeof(struct sockaddr_in6);
	// If less than 0, received a corrupted packet
	recv_len = safeRecvFrom(recv_sk_num, packet, connection);
	// Read packet contents
	if (read_packet(packet) != 0)
	{
		return CRC_ERROR;
	}
	// Return received data amount minues header
	connection->len = remote_len;
	return recv_len - HEADER_LENGTH;
}

// Opposide of creating the packet
int read_packet(Packet *packet)
{
	uint8_t *data = packet->full_packet;
	int i;
	for (i = 0; i < MAX_LEN; i++)
	{
		packet->data[i] = 0;
	}
	// bring packet fields to host order for reading
	packet->seq_num = ntohl(((uint32_t *)data)[0]);
	// this checksum field
	packet->checksum = ntohs(*((int16_t *)(data + SIZE_OF_BUF_SIZE)));
	packet->flag = data[6];
	packet->size = ntohl(*((uint32_t *)(data + 7)));
	//crc check
	if (packet->size > MAX_LEN + HEADER_LENGTH)
	{
		return CRC_ERROR;
	}
	// copy packet data only
	memcpy(packet->data, data + HEADER_LENGTH, packet->size - HEADER_LENGTH);
	// return dirty_bit check
	return in_cksum((uint16_t *)data, packet->size);
}
