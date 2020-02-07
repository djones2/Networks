#ifndef MESSAGE_H
#define MESSAGE_H

#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <cerrno>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <arpa/inet.h>

/* This class holds many of the many function
	calls that surround message handling by 
	both server and cclient. */

/* General error in a networking call */
#define GEN_ERROR -1

/* Flags */
#define INITIAL_PCKT 1
#define GOOD_HANDLE 2
#define ERR_INITIAL_PCKT 3
#define MESSAGE_PCKT 5
#define ERR_MESSAGE_PCKT 7
#define EXIT_REQUEST 8
#define EXIT_REPLY 9
#define HANDLE_LIST_REQUEST 10
#define HANDLE_LIST_LEN 11
#define HANDLE_INFO 12
#define END_OF_LIST 13
#define BAD_HANDLE_REQUEST 14

/* Defining different errors */
#define INVALID_ARGUMENT 20
#define INVALID_IP 21
#define SOCKET_ERROR 22
#define CONNET_CALL_FAIL 23
#define BIND_FAIL 24
#define PACKET_ASSEMBLY_FAILURE 25
#define INVALID_PACKET 26
#define SEND_CALL_FAIL 27
#define SELECT_CALL_FAIL 28
#define LISTEN_CALL_FAIL 29
#define ACCEPT_CALL_FAIL 30
#define INVALID_HANDLE 31
#define LIST_COMMAND_FAIL 32
#define INVALID_PORT 33
#define INVALID_HANDLE_LEN 36
#define INVALID_MESSAGE_ENTRY 37

/* Given or defined in spec*/
#define MAXBUF 1024
#define xstr(a) str(a)
#define str(a) #a
#define MAX_MESSAGE_SIZE 1000
#define MAX_HANDLE_LEN 100
#define PAYLOAD_LEN 200

class Message
{
	uint8_t flag;	
	uint8_t destinationHandleLen;
	uint8_t *bytes;
	uint32_t seqNum;
	uint8_t sourceHandleLen;
	char *source;
	char *dest;
	char *payload;
	int payloadLen;
	int total_length;

public:
	/* Message object */
	Message();
	Message(const char *text);
	Message(uint8_t *received, int length);
	/* No idea why this needs to be here,
		but it somehow solves a memcpy() call. */
	~Message();

	/* Set functions */
	void sequenceNumber(uint32_t number);
	void setFlag(uint8_t flag);
	void destinationHandle(const char *to, int length);
	void sourceHandle(const char *from, int length);
	void setPayload(const char *text, int length);
	void setIndex(int to_set);

	/* Get functions (didn't know you couldn't just
		return attributes) */
	uint32_t getSequenceNum();
	uint8_t getFlag();
	char *getDestination();
	int getDestinationLen();
	char *getSource();
	int getSourceLen();
	char *getPayload();
	int getPayloadLen();
	int getPacketLen();

	/* Helper functions to assemble message packet */
	int messagePacket();
	void unpack();
	uint8_t *sendPacket();

	/* Printing options */
	void print_bytes(char *bytes, int length);
	void print();
	void print_full();
};

#endif
