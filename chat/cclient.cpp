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
#include "cclient.h"

using namespace std;

/* Client creation */
Cclient::Cclient(char *handle, struct hostent *server, int port)
{
	this->handle = handle;
	this->server = server;
	this->port = port;
}

/* Initialize client fields and create connection */
void Cclient::createClient()
{
	Message *messagePacket;
	int msg_size, error;

	exit = false;
	seqNum = 0;
	/* Create connection with server */
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons((short)port);

	memcpy(&(server_address.sin_addr.s_addr), server->h_addr, server->h_length);
	/* Retrieve socket file descriptor, check for availability */
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd == GEN_ERROR)
		throw SOCKET_ERROR;
	/* connect() call to server */
	if (connect(socket_fd, (struct sockaddr *)(&server_address), 16) == GEN_ERROR)
		throw CONNET_CALL_FAIL;
	/* Populate message fields */
	messagePacket = new Message();
	messagePacket->setFlag(INITIAL_PCKT);
	messagePacket->sourceHandle(handle, strlen(handle));
	messagePacket->sequenceNumber(seqNum++);
	msg_size = messagePacket->messagePacket();
	/* Send initial packet to server */
	error = send(socket_fd, messagePacket->sendPacket(), msg_size, 0);
	if (error == GEN_ERROR)
		throw SEND_CALL_FAIL;

	delete messagePacket;
	validHandle();
}

/* Main client loop */
void Cclient::clientMain()
{
	printf("$: ");
	fflush(stdout);
	struct timeval select_timer;
	fd_set read_set;
	/* Loop while active client */
	while (!exit)
	{
		FD_ZERO(&read_set);
		select_timer.tv_sec = 1;
		select_timer.tv_usec = 0;
		/* Set socket file descriptors */
		FD_SET(socket_fd, &read_set);
		FD_SET(0, &read_set);
		/* select() call */
		if (select(socket_fd + 1, &read_set, NULL, NULL, &select_timer) == GEN_ERROR) {
			throw SELECT_CALL_FAIL;
		}
		/* Check if there is a message from server */
		if (FD_ISSET(socket_fd, &read_set))
			unpackMessage();
		/* Get input from user */
		if (FD_ISSET(0, &read_set))
			getInput();
	}
}

/* Called upon exit call */
void Cclient::clientExit()
{
	Message *messagePacket;
	uint8_t *buffer;
	int msg_size, error, flag;

	/* Create exit message to send to server */
	messagePacket = new Message();
	messagePacket->setFlag(EXIT_REQUEST);
	messagePacket->sourceHandle(handle, strlen(handle));
	msg_size = messagePacket->messagePacket();
	messagePacket->sequenceNumber(seqNum++);
	error = send(socket_fd, messagePacket->sendPacket(), msg_size, 0);

	if (error == GEN_ERROR)
		throw SEND_CALL_FAIL;

	delete messagePacket;

	/* Buffer received data until ack */
	buffer = (uint8_t *)calloc(MAXBUF, sizeof(uint8_t));

	msg_size = recv(socket_fd, buffer, MAXBUF, 0);

	/* If no ACK is received */
	if (msg_size == 0)
		throw CONNET_CALL_FAIL;
	/* Successful exit */
	messagePacket = new Message(buffer, msg_size);
	flag = messagePacket->getFlag();
	free(buffer);
	delete messagePacket;
	/* See if exit had been noticed */
	if (flag == EXIT_REQUEST)
	{
		printf("Connection to server closed.\n");
	}
	else {
		throw GEN_ERROR;
	}
	close(socket_fd);
}

/* Check for valid handle */
void Cclient::validHandle()
{
	uint8_t *buffer;
	int message_size, flag;
	Message *messagePacket;
	
	buffer = (uint8_t *)calloc(MAXBUF, sizeof(uint8_t));
	message_size = recv(socket_fd, buffer, MAXBUF, 0);

	if (message_size == 0){
		throw CONNET_CALL_FAIL;
	}
	messagePacket = new Message(buffer, message_size);
	flag = messagePacket->getFlag();
	free(buffer);
	delete messagePacket;

	if (flag == GOOD_HANDLE){
		return;
	} else if (flag == ERR_INITIAL_PCKT) {
		throw INVALID_HANDLE;
	} else {
		throw GEN_ERROR;
	}
}

void Cclient::getInput()
{
	char input_buffer[MAXBUF];
	char cmd;
	int selfMessage;
	selfMessage = 0;

	fgets(input_buffer, MAXBUF, stdin);
	try
	{
		if (strlen(input_buffer) > 1)
		{
			input_buffer[0] == '%' ? cmd = input_buffer[1] : cmd = input_buffer[0];

			if (cmd == 'm' || cmd == 'M')
			{
				selfMessage = sendClientMessage(input_buffer);
			}
			else if (cmd == 'b' || cmd == 'B')
			{
				broadcast(input_buffer);
			}
			else if (cmd == 'l' || cmd == 'L')
			{
				listCommand();
			}
			else if (cmd == 'e' || cmd == 'E')
			{
				exit = true;
			}
		}
		else
			printf("%%<option> <handle> <message>\n");
	}
	catch (int ex)
	{
		if (ex == INVALID_MESSAGE_ENTRY){
			printf("Messages must be under 100 characters.\n");
		}
		else {
			throw ex;
		}
	}

	if (!selfMessage)
	{
		printf("$: ");
		fflush(stdout);
	}
}

void Cclient::unpackMessage()
{
	uint8_t *buffer;
	int message_size, flag;
	char handle[MAX_HANDLE_LEN];
	Message *messagePacket;

	buffer = (uint8_t *)calloc(MAXBUF, sizeof(uint8_t));
	message_size = recv(socket_fd, buffer, MAXBUF, 0);
	/* Connection failure */
	if (message_size == 0) {
		throw CONNET_CALL_FAIL;
	}
	messagePacket = new Message(buffer, message_size);
	flag = messagePacket->getFlag();
	printf("\n");
	if (flag == MESSAGE_PCKT)
		messagePacket->print();
	else if (flag == 7)
	{
		memcpy(handle, messagePacket->getDestination(), messagePacket->getDestinationLen());
		handle[messagePacket->getDestinationLen()] = '\0';
		printf("Client with handle %s does not exist.\n", handle);
	}
	delete messagePacket;
	free(buffer);
	printf("$: ");
	fflush(stdout);
}

int Cclient::sendClientMessage(char *input)
{
	char dest[101];
	int destinationHandleLen;
	char temp[100];
	char *message_start;
	int message_size;
	Message *messagePacket;
	int error, selfMessage;

	sscanf(input, "%s %100s", temp, dest);
	destinationHandleLen = strlen(dest);
	message_start = strstr(input, dest) + strlen(dest) + 1;
	selfMessage = (strcmp(dest, handle) == 0);

	if (strlen(message_start) > MAX_MESSAGE_SIZE) {
		throw INVALID_MESSAGE_ENTRY;
	}

	for (message_size = 0; message_start[message_size] != '\0'; message_size++);

	message_size++;

	messagePacket = new Message();
	messagePacket->destinationHandle(dest, destinationHandleLen);
	messagePacket->sourceHandle(handle, strlen(handle));

	if (message_size == 0) {
		messagePacket->setPayload("\n", 1);
	} else {
		messagePacket->setPayload(message_start, message_size);
	}
	messagePacket->setFlag(MESSAGE_PCKT);
	messagePacket->sequenceNumber(seqNum++);
	message_size = messagePacket->messagePacket();
	error = send(socket_fd, messagePacket->sendPacket(), message_size, 0);

	if (error == GEN_ERROR)
		throw SEND_CALL_FAIL;

	return selfMessage;
}

void Cclient::broadcast(char *input)
{
	char temp[100];
	char *message_start;
	int message_size;
	Message *messagePacket;
	int error;

	sscanf(input, "%s", temp);
	message_start = input + strlen(temp) + 1;
	if (strlen(message_start) > MAX_MESSAGE_SIZE)
		throw INVALID_MESSAGE_ENTRY;
	for (message_size = 0; message_start[message_size] != '\0'; message_size++);

	message_size++;
	messagePacket = new Message();
	messagePacket->sourceHandle(handle, strlen(handle));
	messagePacket->setPayload(message_start, message_size);
	if (message_size == 0)
		messagePacket->setPayload("\n", 1);
	else
		messagePacket->setPayload(message_start, message_size);

	messagePacket->setFlag(MESSAGE_PCKT);
	messagePacket->sequenceNumber(seqNum++);
	message_size = messagePacket->messagePacket();

	error = send(socket_fd, messagePacket->sendPacket(), message_size, 0);

	if (error == GEN_ERROR)
		throw SEND_CALL_FAIL;
}

void Cclient::listCommand()
{
	int list_length;
	list_length = listLengthRequest();
	for (int i = 0; i < list_length; i++)
		requestHandle(i);
}

int Cclient::listLengthRequest()
{
	Message *messagePacket;
	uint8_t buffer[MAXBUF];
	int msg_size, error, flag, length;

	//Request list length from server, flag HANDLE_LIST_REQUEST
	messagePacket = new Message();
	messagePacket->setFlag(HANDLE_LIST_REQUEST);
	messagePacket->sourceHandle(handle, strlen(handle));
	messagePacket->sequenceNumber(seqNum++);
	msg_size = messagePacket->messagePacket();

	error = send(socket_fd, messagePacket->sendPacket(), msg_size, 0);

	if (error == GEN_ERROR)
		throw SEND_CALL_FAIL;

	delete messagePacket;

	msg_size = recv(socket_fd, buffer, MAXBUF, 0);

	if (msg_size == 0)
		throw CONNET_CALL_FAIL;

	messagePacket = new Message(buffer, msg_size);
	flag = messagePacket->getFlag();
	length = ntohl(((uint32_t *)messagePacket->getPayload())[0]);
	delete messagePacket;

	// Check Flag for HANDLE_LIST_LEN
	if (flag != HANDLE_LIST_LEN)
		throw LIST_COMMAND_FAIL;

	return length;
}

void Cclient::requestHandle(int index)
{
	Message *messagePacket;
	uint8_t buffer[MAXBUF];
	char *incoming_handle;
	uint8_t handle_length;
	int msg_size, error, flag;

/* 	Request handle with index */	
	messagePacket = new Message();
	messagePacket->setFlag(HANDLE_INFO);
	messagePacket->sourceHandle(handle, strlen(handle));
	messagePacket->setIndex(index);
	messagePacket->sequenceNumber(seqNum++);
	msg_size = messagePacket->messagePacket();

	error = send(socket_fd, messagePacket->sendPacket(), msg_size, 0);
	if (error == GEN_ERROR)
		throw SEND_CALL_FAIL;
	delete messagePacket;
	msg_size = recv(socket_fd, buffer, MAXBUF, 0);
	if (msg_size == 0)
		throw CONNET_CALL_FAIL;

	messagePacket = new Message(buffer, msg_size);
	flag = messagePacket->getFlag();
	incoming_handle = messagePacket->getPayload();
	delete messagePacket;
	if (flag == END_OF_LIST)
	{
		handle_length = incoming_handle[0];
		for (int i = 1; i <= handle_length; i++)
			printf("%c", incoming_handle[i]);
		printf("\n");
	}
	else {
		throw LIST_COMMAND_FAIL;
	}
}

void clientException(int ex) {
	if (ex == INVALID_ARGUMENT){
		cout << "cclient <handle> <server-IP> <server-port>\n";
	} else if (ex == INVALID_IP) {
		cout << "Could not resolve hostname.\n";
	} else if (ex == SOCKET_ERROR){
		cout << "socket() failed. errno " << errno << "\n";
	} else if (ex == CONNET_CALL_FAIL){
		cout << "connect() failed. errno " << errno << "\n";
	} else if (ex == PACKET_ASSEMBLY_FAILURE){
		cout << "Message packing failed. errno " << errno << "\n";
	} else if (ex == INVALID_PACKET){
		cout << "Message unpacking failed. errno " << errno << "\n";
	} else if (ex == SEND_CALL_FAIL) {
		cout << "Message sending failed. errno " << errno << "\n";
	} else if (ex == INVALID_HANDLE){
		cout << "Handle is already taken.\n";
	} else if (ex == INVALID_HANDLE_LEN){
		cout << "Handles must be under 100 characters.\n";
	} else {
		cout << "Unknown exception " << ex << "\n";
	}
}

void clientWrap(char* handle, struct hostent *server, int port, Cclient *client) {
	if (strlen(handle) > MAX_HANDLE_LEN)
		throw INVALID_HANDLE_LEN;

	if (server == NULL)
		throw INVALID_IP;

	client = new Cclient(handle, server, port);
	client->createClient();
	client->clientMain();
	client->clientExit();
}

int main(int argc, char **argv)
{
	if (argc != 4) {throw INVALID_ARGUMENT;}
	char *handle;
	struct hostent *server;
	int port;
	Cclient *client = NULL;
	try
	{	handle = argv[1];
		server = gethostbyname(argv[2]);
		stringstream(argv[3]) >> port;
		clientWrap(handle, server, port, client);
	}
	catch (int ex)
	{
		clientException(ex);
	}
}
