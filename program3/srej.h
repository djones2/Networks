#ifndef __SREJ_H__
#define __SREJ_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdint.h>

#include "networks.h"
#include "cpe464.h"

// Packet Setup (taken from Hugh Smith's handout)
#define HEADER_LENGTH 11
#define MAX_PACKET_SIZE 1410
#define SIZE_OF_BUF_SIZE 4
#define FIRST_SEQ_NUM 1
#define MAX_TRIES 10
#define SHORT_TIME 1
#define LONG_TIME 10

#pragma pack(1)

typedef struct header Header;

struct header { 
    uint32_t seq_num;
    uint16_t int_checksum;
    uint8_t flag;
};

enum FLAG {
    CRC_ERROR = -1, FILENAME = 1, FILENAME_ACK = 2, DATA_PACKET = 3, RR = 5, SREJ = 6, 
    RCOPY_INFO = 7, SERVER_INFO = 8, END_OF_FILE = 9, EOF_ACK = 10
};

int retrieveHeader(uint8_t * data_buf, int recv_len, uint8_t * flag, uint32_t * sequence_number);
int32_t recv_buf(uint8_t * buf, int32_t len, int32_t recv_socket_number, Connection * connection,
                    uint8_t * flag, uint32_t * sequence_number);


#endif