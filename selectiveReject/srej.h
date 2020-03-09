#ifndef __SREJ_H__
#define __SREJ_H__

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

#include "networks.h"
#include "cpe464.h"

#define MAX_LEN 1400
#define MIN_LEN 400
#define FILENAME_LEN 100
#define HEADER_LENGTH 11
#define SIZE_OF_BUF_SIZE 4
#define START_SEQ_NUM 1
#define MAX_TRIES 10
#define LONG_TIME 10
#define SHORT_TIME 1

#pragma pack(1)

// Needed to include this type here, too, for compilation.
typedef struct connection Connection;
typedef struct packet Packet;

struct packet
{
	uint32_t seq_num;
	int16_t checksum;
	int8_t flag;
	uint32_t size;
	uint8_t data[MAX_LEN];
	uint8_t full_packet[MAX_LEN + HEADER_LENGTH];
};

enum FLAG
{
	DATA = 3,
	RR = 5,
	SREJ = 6,
	FNAME = 7,
	FNAME_OK = 8,
	FNAME_BAD = 9,
	END_OF_FILE = 10,
	CRC_ERROR = -1,
	RESEND
};

int32_t send_buf(uint8_t *data, uint32_t len, Connection *connection, uint8_t flag,
				 uint32_t seq_num);
void create_packet(Packet *packet);
int32_t send_additional_packet(Packet *packet, Connection *connection);
int32_t recv_buf(Packet *packet, int32_t recv_sk_num, Connection *connection);
int read_packet(Packet *packet);

#endif
