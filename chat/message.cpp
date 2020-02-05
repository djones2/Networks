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
	to_length = 0;
	to = NULL;
	from_length = 0;
	from = NULL;
	text_length = 0;
	text = NULL;
	bytes = NULL;
	total_length = 0;
}

Message::Message(const char *text) {
	sequence_number = 0;
	flag = 0;
	to_length = 0;
	to = NULL;
	from_length = 0;
	from = NULL;
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
	to = NULL;
	from = NULL;
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

int Message::pack() {
	uint32_t *pointer;
	
	total_length = MIN_MESSAGE_SIZE;
	
	total_length += to_length + from_length + text_length;
	
	if (bytes != NULL)
		free(bytes);
	
	bytes = (uint8_t *)calloc(total_length, sizeof(uint8_t));
	
	pointer = (uint32_t *)bytes;
	pointer[0] = htonl(sequence_number);
	bytes[4] = flag;
	bytes[5] = to_length;
	
	if (to_length)
		memcpy(bytes + 6, to, to_length);
	
	bytes[6 + to_length] = from_length;
	
	if (from_length)
		memcpy(bytes + 6 + to_length + 1, from, from_length);
	
	if (text_length)
		memcpy(bytes + 6 + to_length + 1 + from_length, text, text_length);
	
	return total_length;
}

void Message::unpack() {
	if (bytes == NULL)
		throw UNPACK_EX;
	
	if (to != NULL)
		free(to);
	if (from != NULL)
		free(from);
	if (text != NULL)
		free(text);
		
	sequence_number = ntohl(((uint32_t *)bytes)[0]);
	flag = bytes[4];
	
	to_length = bytes[5];
	to = (char *) calloc(to_length, sizeof(char));
	memcpy(to, bytes + 6, to_length);
	
	from_length = bytes[6 + to_length];
	from = (char *) calloc(from_length, sizeof(char));
	memcpy(from, bytes + 6 + to_length + 1, from_length);
	
	text_length = total_length - MIN_MESSAGE_SIZE - to_length - from_length;
	//printf("total length %d\n", total_length);
	text = (char *) calloc(text_length, sizeof(char));
	memcpy(text, bytes + 7 + to_length + from_length, text_length);
	
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

void Message::set_to(const char *to, int length) {
	if (this->to != NULL)
		free(this->to);
	
	to_length = length;
	this->to = (char *)calloc(length, sizeof(char));
	memcpy(this->to, to, length);
}
	
void Message::set_from(const char *from, int length) {
	if (this->from != NULL)
		free(this->from);
	
	from_length = length;
	this->from = (char *)calloc(length, sizeof(char));
	memcpy(this->from, from, length );
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
	return to;
}

int Message::get_to_length() {
	return to_length;
}

char *Message::get_from() {
	return from;
}

int Message::get_from_length() {
	return from_length;
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
	print_bytes(from, from_length);
	printf(": ");
	print_bytes(text, text_length);
}

void Message::print_full() {
	printf("Message\n");
	printf("   sequence_number %d\n", sequence_number);
	printf("              flag %d\n", flag);
	printf("         to_length %d\n", to_length);
	printf("                to "); print_bytes(to, to_length); printf("\n");
	printf("       from_length %d\n", from_length);
	printf("              from "); print_bytes(from, from_length); printf("\n");
	printf("       text_length %d\n", text_length);
	printf("              text "); print_bytes(text, text_length); printf("\n");
	printf("      total_length %d\n", total_length);
}