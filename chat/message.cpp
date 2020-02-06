#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <arpa/inet.h>

#include "message.h"

#define MIN_MESSAGE_SIZE 7

/*
 * Structors
 */
Message::Message() {
	sequence_number = 0;
	flag = 0;
	destinationLength = 0;
	destination = NULL;
	sourceLength = 0;
	source = NULL;
	text_length = 0;
	text = NULL;
	bytes = NULL;
	total_length = 0;
}

Message::Message(const char *text) {
	sequence_number = 0;
	flag = 0;
	destinationLength = 0;
	destination = NULL;
	sourceLength = 0;
	source = NULL;
	text_length = strlen(text);
	this->text = new char[text_length];
	strcpy(this->text, text);
	bytes = NULL;
	total_length = 0;
}

Message::Message(uint8_t *recieved, int length) {
	bytes = (uint8_t *) calloc(length, sizeof(uint8_t));
	memcpy(bytes, recieved, length);
	total_length = length;
	destination = NULL;
	source = NULL;
	text = NULL;
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

int Message::packet() {
	uint32_t *pointer;
	
	total_length = MIN_MESSAGE_SIZE;
	
	total_length += destinationLength + sourceLength + text_length;
	
	if (bytes != NULL)
		free(bytes);
	
	bytes = (uint8_t *)calloc(total_length, sizeof(uint8_t));
	
	pointer = (uint32_t *)bytes;
	pointer[0] = htonl(sequence_number);
	bytes[4] = flag;
	bytes[5] = destinationLength;
	
	if (destinationLength)
		memcpy(bytes + 6, destination, destinationLength);
	
	bytes[6 + destinationLength] = sourceLength;
	
	if (sourceLength)
		memcpy(bytes + 6 + destinationLength + 1, source, sourceLength);
	
	if (text_length)
		memcpy(bytes + 6 + destinationLength + 1 + sourceLength, text, text_length);
	
	return total_length;
}

void Message::unpack() {
	if (bytes == NULL)
		throw UNPACK_EX;
	
	if (destination != NULL)
		free(destination);
	if (source != NULL)
		free(source);
	if (text != NULL)
		free(text);
		
	sequence_number = ntohl(((uint32_t *)bytes)[0]);
	flag = bytes[4];
	
	destinationLength = bytes[5];
	destination = (char *) calloc(destinationLength, sizeof(char));
	memcpy(destination, bytes + 6, destinationLength);
	
	sourceLength = bytes[6 + destinationLength];
	source = (char *) calloc(sourceLength, sizeof(char));
	memcpy(source, bytes + 6 + destinationLength + 1, sourceLength);
	
	text_length = total_length - MIN_MESSAGE_SIZE - destinationLength - sourceLength;
	//printf("total length %d\n", total_length);
	text = (char *) calloc(text_length, sizeof(char));
	memcpy(text, bytes + 7 + destinationLength + sourceLength, text_length);
	
}

uint8_t *Message::sendable() {
	return bytes;
}

/*
 * Setter functions
 */
void Message::set_sequence_number(uint32_t number) {
	sequence_number = number;
}

void Message::set_flag(uint8_t flag) {
	this->flag = flag;
}

void Message::set_to(const char *destination, int length) {
	if (this->destination != NULL)
		free(this->destination);
	
	destinationLength = length;
	this->destination = (char *)calloc(length, sizeof(char));
	memcpy(this->destination, destination, length);
}
	
void Message::set_from(const char *source, int length) {
	if (this->source != NULL)
		free(this->source);
	
	sourceLength = length;
	this->source = (char *)calloc(length, sizeof(char));
	memcpy(this->source, source, length );
}
		
void Message::set_text(const char *text, int length) {
	if (this->text != NULL)
		free(this->text);
	
	text_length = length;
	this->text = (char *)calloc(length, sizeof(char));
	memcpy(this->text, text, length);
}

void Message::set_int(int to_set) {
	uint32_t *pointer;
	
	if (this->text != NULL)
		free(this->text);
	
	text_length = 4;
	text = (char *)calloc(4, sizeof(char));
	pointer = (uint32_t *) text;
	pointer[0] = htonl(to_set);	
}

/*
 * Getter Functions
 */
uint32_t Message::get_sequence_number() {
	return sequence_number;
}

uint8_t Message::get_flag() {
	return flag;
}

char *Message::get_to() {
	return destination;
}

int Message::get_to_length() {
	return destinationLength;
}

char *Message::getSource() {
	return source;
}

int Message::get_from_length() {
	return sourceLength;
}

char *Message::get_text() {
	return text;
}

int Message::get_length() {
	return total_length;
}

int Message::get_text_length() {
	return text_length;
}

/*
 * Output Functions
 */
void Message::print_bytes(char *bytes, int length) {
	for (int i = 0; i < length; i++)
		printf("%c", bytes[i]);
}


void Message::print() {
	print_bytes(source, sourceLength);
	printf(": ");
	print_bytes(text, text_length);
}

void Message::print_full() {
	printf("Message\n");
	printf("   sequence_number %d\n", sequence_number);
	printf("              flag %d\n", flag);
	printf("         destinationLength %d\n", destinationLength);
	printf("                destination "); print_bytes(destination, destinationLength); printf("\n");
	printf("       sourceLength %d\n", sourceLength);
	printf("              source "); print_bytes(source, sourceLength); printf("\n");
	printf("       text_length %d\n", text_length);
	printf("              text "); print_bytes(text, text_length); printf("\n");
	printf("      total_length %d\n", total_length);
}