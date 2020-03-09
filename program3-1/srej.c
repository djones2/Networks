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

#include "slidingWindow.h"
#include "srej.h"
#include "cpe464.h"
#include "networks.h"

int32_t send_buf(uint8_t *buf, int32_t len, Connection *connection,
                 uint8_t flag, uint32_t sequence_number, uint8_t *packet)
{
/*     int32_t sentLen = 0;
    int32_t sendingLen = 0;

    // Packet setup
    if (len > 0)
    {
        memcpy(&packet[sizeof(Header)], buf, len);
    }
    sendingLen = createHeader(len, flag, sequence_number, packet);
    sentLen = safeSendto(packet, sendingLen, connection);
    return sentLen; */
    return 0;
}

// Translated from send_buf but tailored for all packets.
int32_t send_packet(uint8_t *data, uint32_t size, Connection *connection, uint8_t flag, 
	uint32_t seq_num) {
	Packet packet;
	int32_t sentLen = 0;
	
	packet.seq_num = seq_num;
	packet.flag = flag;
	packet.packet_size = size + HEADER_LENGTH;
	
	memcpy(packet.payload, data, size);
	
	createPacket(&packet);
	
	if ((sentLen = sendtoErr(connection->socket_number, packet.packet, packet.packet_size, 0,
		(struct sockaddr *) &(connection->remote), connection->length)) < 0) {
		perror("sendto err: ");
		exit(-1);
	}
	
	return sentLen;
}

// Send an additional packet if data buffer is not large enough
// to send all data in one packet
int32_t sendAdditionalPacket(Packet *packet, Connection *connection) {
	int32_t sentLen = 0;
	
	if ((sentLen = sendtoErr(connection->socket_number, packet->packet, packet->packet_size, 0,
		(struct sockaddr *) &(connection->remote), connection->length)) < 0) {
		perror("sendto err: ");
		exit(-1);
	}
	
	return sentLen;
}

// Creates regular header and inserts into packet
int createHeader(uint32_t len, uint8_t flag, uint32_t seq_num, uint8_t *packet)
{
/*     Header *aHeader = (Header *)packet;
    uint16_t checksum = 0;
    // Allocate sequence number
    seq_num = htonl(seq_num);
    memcpy(&(aHeader->seq_num), &seq_num, sizeof(seq_num));
    // Set flag
    aHeader->flag = flag;
    // Clear checksum
    memset(&(aHeader->int_checksum), 0, sizeof(checksum));
    // Evaluate checksum
    checksum = in_cksum((unsigned short *)packet, len + sizeof(Header));
    memcpy(&(aHeader->int_checksum), &checksum, sizeof(checksum));
    // Return total size
    return len + sizeof(Header); */
    return 0;
}

int32_t recv_buf(uint8_t *buf, int32_t len, int32_t recv_socket_number, Connection *connection,
                 uint8_t *flag, uint32_t *sequence_number)
{
/*     uint8_t data_buf[MAX_LEN];
    int32_t recv_len = 0;
    int32_t dataLen = 0;

    recv_len = safeRecvfrom(recv_socket_number, data_buf, len, connection);
    dataLen = retrieveHeader(data_buf, recv_len, flag, sequence_number);

    if (dataLen > 0)
    {
        memcpy(buf, &data_buf[sizeof(Header)], dataLen);
    }
    return dataLen; */
    return 0;
} 

// Translated recv_buffer to receive an entire packet. Calls parsePacket to 
// get different parts of the packet
int32_t recv_packet(Packet *packet, int32_t recv_sk_num, Connection *connection) {
	int32_t recv_len = 0;
	uint32_t remote_len = sizeof(struct sockaddr_in);
	
	if ((recv_len = recvfromErr(recv_sk_num, packet->packet, MAX_LEN + HEADER_LENGTH, 0, 
		(struct sockaddr *) &(connection->remote), &remote_len)) < 0) {
		perror("recv_buf, recvfrom");
		exit(-1);
	}
	
	if (parsePacket(packet) != 0)
		return CRC_ERROR;
	
	connection->length = remote_len;
	
	return recv_len - HEADER_LENGTH;
}

int retrieveHeader(uint8_t *data_buf, int recv_len, uint8_t *flag, uint32_t *sequence_number)
{
/*     Header *aHeader = (Header *)data_buf;
    int returnValue = 0;

    if (in_cksum((unsigned short *)data_buf, recv_len) != 0)
    {
        returnValue = CRC_ERROR;
    }
    else
    {
        *flag = aHeader->flag;
        memcpy(sequence_number, &(aHeader->seq_num), sizeof(aHeader->seq_num));
        *sequence_number = ntohl(*sequence_number);
        returnValue = recv_len - sizeof(Header);
    }
    return returnValue; */
    return 0;
}

int processSelect(Connection *client, int *retryCount, int selectTimeoutState, int dataReadyState,
                  int doneState)
{
    int returnValue = dataReadyState;
    (*retryCount)++;

    if (*retryCount > MAX_TRIES)
    {
        printf("No response for 10 seconds, terminating connection.\n");
        returnValue = doneState;
    }
    else
    {
        if (select_call(client->socket_number, SHORT_TIME, 0, NOT_NULL) == 1)
        {
            *retryCount = 0;
            returnValue = dataReadyState;
        }
        else
        {
            returnValue = selectTimeoutState;
        }
    }
    return returnValue;
}

void createPacket(Packet *packet) {
	uint8_t *result = packet->packet;
	int i;
	
	for (i = 0; i < MAX_LEN + HEADER_LENGTH; i++) {
		result[i] = 0;
    }	
	((uint32_t *)result)[0] = htonl(packet->seq_num);
	result[6] = packet->flag;
	*((uint32_t *)(result + 7)) = htonl(packet->packet_size);
	memcpy(result + 11, &packet->payload, MAX_LEN); 
	packet->int_checksum = in_cksum((uint16_t *) result, packet->packet_size);
	((int16_t *)result)[2] = packet->int_checksum;
}

int parsePacket(Packet *packet) {
	uint8_t *result = packet->packet;
	int i;
	
    // Construct empty packet
	for (i = 0; i < MAX_LEN; i++)  {
		packet->payload[i] = 0;
    }

    // Copy packet information
	packet->seq_num = ntohl(((uint32_t *)result)[0]);
	packet->int_checksum = ntohs(*((int16_t *)(result + 4)));
	packet->flag = result[6];
	packet->packet_size = ntohl(*((uint32_t *)(result + 7)));
	
    // Size check
	if (packet->packet_size > MAX_LEN + HEADER_LENGTH) {
		return CRC_ERROR;
	}
    // Copy data to packet payload
	memcpy(packet->payload, result + HEADER_LENGTH, packet->packet_size - HEADER_LENGTH);
	// Return checksum for validity
    return in_cksum((uint16_t *) result, packet->packet_size);
}