#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>

/* Flags */
#define INITIAL_PCKT 1
#define GOOD_HANDLE 2
#define ERR_INITIAL_PCKT 3
#define BROADAST_PCKT 4
#define MESSAGE_PCKT 5
#define ERR_MESSAGE_PCKT 7
#define EXIT_REQUEST 8
#define EXIT_REPLY 9
#define HANDLE_LIST_REQUEST 10
#define HANDLE_LIST_LEN 11
#define HANDLE_INFO 12
#define END_OF_LIST 13

/* Exception handling */
#define GEN_ERROR -1
#define ARGUMENT_ERROR 14
#define IP_EX 15
#define CLIENT_NAME_ERROR 16
#define SOCKET_FAIL 17
#define BIND_EX 18
#define PACK_EX 19
#define UNPACK_EX 20
#define SEND_FAIL 21
#define SELECT_EX 22
#define LISTEN_EX 23
#define ACCEPT_EX 24
#define HANDLE_EX 25
#define MYSTERY_EX 26
#define EXIT_EX 27
#define LIST_EX 28
#define PORT_EX 29
#define HANDLE_LENGTH_EX 30
#define MESSAGE_SIZE_EX 31

/* Max sizing */
#define MAXBUF 1024
#define MAX_HANDLE_LENGTH 100
#define MAX_PAYLOAD 200

class Message
{
	uint32_t sequence_number;
	uint8_t flag;
	uint8_t destinationLength;
	char *destination;
	uint8_t sourceLength;
	char *source;
	int text_length;
	char *text;
	uint8_t *bytes;
	int total_length;

public:
	/* Create message class */
	Message();
	Message(const char *text);
	Message(uint8_t *recieved, int length);
	~Message();
	/* Assemble to message packet */
	int packet();
	void unpack();
	uint8_t *sendable();
	/* Set message fields */
	void set_sequence_number(uint32_t number);
	void set_flag(uint8_t flag);
	void set_to(const char *destination, int length);
	void set_from(const char *source, int length);
	void set_text(const char *text, int length);
	void set_int(int to_set);
	/* Get message fields */
	uint32_t get_sequence_number();
	uint8_t get_flag();
	char *get_to();
	int get_to_length();
	char *getSource();
	int get_from_length();
	char *get_text();
	int get_length();
	int get_text_length();
	/* Print message fields */
	void print_full();
	void print();
	void print_bytes(char *bytes, int length);
};

#endif