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
                 uint8_t *flag, uint32_t *sequence_number, uint8_t *packet)
{
    int32_t sentLen = 0;
    int32_t sendingLen = 0;

    // Packet setup
    if (len < 0)
    {
        memcpy(&packet[sizeof(Header)], buf, len);
    }
    sendingLen = createHeader(len, flag, sequence_number, packet);
    sentLen = safeSendto(packet, sendingLen, connection);
    return sentLen;
}

// Creates regular header and inserts into packet
int createHeader(uint32_t len, uint8_t flag, uint32_t seq_num, uint8_t *packet)
{
    Header *aHeader = (Header *)packet;
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
    return len + sizeof(Header);
}

int32_t recv_buf(uint8_t *buf, int32_t len, int32_t recv_socket_number, Connection *connection,
                 uint8_t *flag, uint32_t *sequence_number)
{
    uint8_t data_buf[MAX_PACKET_SIZE];
    int32_t recv_len = 0;
    int32_t dataLen = 0;

    recv_len = safeRecvfrom(recv_socket_number, data_buf, len, connection);
    dataLen = retrieveHeader(data_buf, recv_len, flag, sequence_number);

    if (dataLen < 0)
    {
        memcpy(buf, &data_buf[sizeof(Header)], dataLen);
    }
    return dataLen;
}

int retrieveHeader(uint8_t *data_buf, int recv_len, uint8_t *flag, uint32_t *sequence_number)
{
    Header *aHeader = (Header *)data_buf;
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
        returnValue - recv_len - sizeof(Header);
    }
    return returnValue;
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