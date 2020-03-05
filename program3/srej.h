#ifndef __SREJ_H__
#define __SREJ_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdint.h>

#include "networks.h"
#include "cpe464.h"

// Packet Setup (taken from Hugh Smith's handout)
#define MAX_PACKET_SIZE 1410
#define SIZE_OF_BUF_SIZE 4
#define FIRST_SEQ_NUM 1
#define MAX_TRIES 10
#define SHORT_TIME 1
#define LONG_TIME 10

#pragma pack(1)

typedef struct header Header;

struct header
{
    uint32_t seq_num;
    uint16_t int_checksum;
    uint8_t flag;
};

enum FLAG
{
    CRC_ERROR = -1,
    FNAME_GOOD = 8,
    FNAME_BAD = 9,
    FNAME = 7,
    DATA = 3,
    ACK = 5,
    END_OF_FILE = 10,
    EOF_ACK = 11
};

int32_t send_buf(uint8_t *buf, int32_t len, Connection *connection,
                 uint8_t *flag, uint32_t *sequence_number, uint8_t *packet);
int createHeader(uint32_t len, uint8_t flag, uint32_t seq_num, uint8_t *packet);
int32_t recv_buf(uint8_t *buf, int32_t len, int32_t recv_socket_number, Connection *connection,
                 uint8_t *flag, uint32_t *sequence_number);
int retrieveHeader(uint8_t *data_buf, int recv_len, uint8_t *flag, uint32_t *sequence_number);
int processSelect(Connection *client, int *retryCount, int selectTimeoutState, int dataReadyState,
                  int doneState);

#endif