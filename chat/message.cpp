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

#include "message.h"

#define MIN_MESSAGE_SIZE 0

/*
 * Structors
 */
Message::Message() {
	seqNum = 0;
	flag = 0;
	destinationHandleLen = 0;
	dest = NULL;
	sourceHandleLen = 0;
	source = NULL;
	payloadLen = 0;
	payload = NULL;
	bytes = NULL;
	total_length = 0;
}

Message::Message(const char *payload) {
	seqNum = 0;
	flag = 0;
	destinationHandleLen = 0;
	dest = NULL;
	sourceHandleLen = 0;
	source = NULL;
	payloadLen = strlen(payload);
	this->payload = new char[payloadLen];
	strcpy(this->payload, payload);
	bytes = NULL;
	total_length = 0;
}

Message::Message(uint8_t *received, int length) {
	bytes = (uint8_t *) calloc(length, sizeof(uint8_t));
	memcpy(bytes, received, length);
	total_length = length;
	dest = NULL;
	source = NULL;
	payload = NULL;
	unpack();
}

Message::~Message() {
	if (bytes != NULL)
		free(bytes);
	bytes = NULL;
}

/*
 * Serialization functions
 */

int Message::messagePacket() {
	uint32_t *pointer;
	
	total_length = MIN_MESSAGE_SIZE;
	
	total_length += destinationHandleLen + sourceHandleLen + payloadLen;
	
	if (bytes != NULL)
		free(bytes);
	
	bytes = (uint8_t *)calloc(total_length, sizeof(uint8_t));
	
	pointer = (uint32_t *)bytes;
	pointer[0] = htonl(seqNum);
	bytes[4] = flag;
	bytes[5] = destinationHandleLen;
	
	if (destinationHandleLen)
		memcpy(bytes + 6, dest, destinationHandleLen);
	
	bytes[6 + destinationHandleLen] = sourceHandleLen;
	
	if (sourceHandleLen)
		memcpy(bytes + 6 + destinationHandleLen + 1, source, sourceHandleLen);
	
	if (payloadLen)
		memcpy(bytes + 6 + destinationHandleLen + 1 + sourceHandleLen, payload, payloadLen);
	
	return total_length;
}

void Message::unpack() {
	if (bytes == NULL)
		throw INVALID_PACKET;
	if (payload != NULL)
		free(payload);
	if (source != NULL)
		free(source);	
	if (dest != NULL)
		free(dest);

	seqNum = ntohl(((uint32_t *)bytes)[0]);
	flag = bytes[4];
	
	destinationHandleLen = bytes[5];
	dest = (char *) calloc(destinationHandleLen, sizeof(char));
	memcpy(dest, bytes + 6, destinationHandleLen);
	
	sourceHandleLen = bytes[6 + destinationHandleLen];
	source = (char *) calloc(sourceHandleLen, sizeof(char));
	memcpy(source, bytes + 6 + destinationHandleLen + 1, sourceHandleLen);
	
	payloadLen = total_length - MIN_MESSAGE_SIZE - destinationHandleLen - sourceHandleLen;
	payload = (char *) calloc(payloadLen, sizeof(char));
	memcpy(payload, bytes + 7 + destinationHandleLen + sourceHandleLen, payloadLen);
	
}

uint8_t *Message::sendPacket() {
	return bytes;
}

/*
 * Setter functions
 */
void Message::sequenceNumber(uint32_t number) {
	seqNum = number;
}

void Message::setFlag(uint8_t flag) {
	this->flag = flag;
}

void Message::destinationHandle(const char *dest, int length) {
	if (this->dest != NULL)
		free(this->dest);
	
	destinationHandleLen = length;
	this->dest = (char *)calloc(length, sizeof(char));
	memcpy(this->dest, dest, length);
}
	
void Message::sourceHandle(const char *source, int length) {
	if (this->source != NULL)
		free(this->source);
	
	sourceHandleLen = length;
	this->source = (char *)calloc(length, sizeof(char));
	memcpy(this->source, source, length);
}
		
void Message::setPayload(const char *payload, int length) {
	if (this->payload != NULL)
		free(this->payload);
	
	payloadLen = length;
	this->payload = (char *)calloc(length, sizeof(char));
	memcpy(this->payload, payload, length);
}

void Message::setIndex(int to_set) {
	uint32_t *pointer;
	
	if (this->payload != NULL)
		free(this->payload);
	
	payloadLen = 4;
	payload = (char *)calloc(4, sizeof(char));
	pointer = (uint32_t *) payload;
	pointer[0] = htonl(to_set);	
}

/*
 * Getter Functions
 */
uint32_t Message::getSequenceNum() {
	return seqNum;
}

uint8_t Message::getFlag() {
	return flag;
}

char *Message::getDestination() {
	return dest;
}

int Message::getDestinationLen() {
	return destinationHandleLen;
}

char *Message::getSource() {
	return source;
}

int Message::getSourceLen() {
	return sourceHandleLen;
}

char *Message::getPayload() {
	return payload;
}

int Message::getPacketLen() {
	return total_length;
}

int Message::getPayloadLen() {
	return payloadLen;
}

/*
 * Output Functions
 */
void Message::print_bytes(char *bytes, int length) {
	for (int i = 0; i < length; i++)
		printf("%c", bytes[i]);
}


void Message::print() {
	print_bytes(source, sourceHandleLen);
	printf(": ");
	print_bytes(payload, payloadLen);
}

void Message::print_full() {
	printf("Message\n");
	printf("   sequence_number %d\n", seqNum);
	printf("              flag %d\n", flag);
	printf("         destinationHandleLen %d\n", destinationHandleLen);
	printf("                dest "); print_bytes(dest, destinationHandleLen); printf("\n");
	printf("       sourceHandleLen %d\n", sourceHandleLen);
	printf("              from "); print_bytes(source, sourceHandleLen); printf("\n");
	printf("       text_length %d\n", payloadLen);
	printf("              payload "); print_bytes(payload, payloadLen); printf("\n");
	printf("      total_length %d\n", total_length);
}
