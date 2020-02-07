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

#include "server.h"
#include "message.h"

#define MAXBUF 1024

using namespace std;

/*
 *	 Lifecycle Functions
 */

Server::Server(int argc, char **argv)
{
	port = 0;

	if (argc == 2)
		stringstream(argv[1]) >> port;
	else
		port = 0;
}

void Server::setup()
{
	socklen_t address_length;

	socketNum = socket(AF_INET, SOCK_STREAM, 0);
	seqNum = 0;

	if (socketNum == GEN_ERROR)
		throw SOCKET_ERROR;

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(port);

	if (bind(socketNum, (const sockaddr *)&address, sizeof(address)) == GEN_ERROR)
		throw BIND_FAIL;

	if (listen(socketNum, 10) == GEN_ERROR)
		throw LISTEN_CALL_FAIL;

	address_length = sizeof(address);

	if (getsockname(socketNum, (struct sockaddr *)&address, &address_length) == GEN_ERROR)
		throw INVALID_PORT;

	if (port == 0)
		printf("Server is using port %d\n", ntohs(address.sin_port));

	lastFD = socketNum;

	openSocketCount = 0;
	clientCount = 0;

	capacity = MAXBUF;
	openConnections = (int *)calloc(capacity, sizeof(int));
	activeClients = (char **)calloc(capacity, sizeof(char *));
}

void Server::serve()
{
	while (true)
	{
		FD_ZERO(&read_set);
		select_timer.tv_sec = 1;
		select_timer.tv_usec = 1000;

		FD_SET(socketNum, &read_set);
		for (int i = 0; i < openSocketCount; i++)
		{
			currentSocket = openConnections[i];
			FD_SET(currentSocket, &read_set);
		}

		if (select(lastFD + 1, &read_set, NULL, NULL, &select_timer) == GEN_ERROR)
			throw SELECT_CALL_FAIL;

		if (FD_ISSET(socketNum, &read_set))
			initPacketResponse();

		for (int i = 0; i < openSocketCount; i++)
		{
			currentSocket = openConnections[i];
			if (FD_ISSET(currentSocket, &read_set))
			{
				processMessage(currentSocket);
			}
		}
	}
}

void Server::processMessage(int fd)
{
	uint8_t buffer[MAXBUF];
	int message_size, flag;
	Message *messagePacket;

	message_size = recv(fd, buffer, MAXBUF, 0);

	if (message_size)
	{

		messagePacket = new Message(buffer, message_size);
		messagePacket->print();
		flag = messagePacket->getFlag();

		if (flag == 1)
			initPacket(fd, messagePacket);
		else if (flag == MESSAGE_PCKT)
		{
			forward(fd, messagePacket, message_size);
		}
		else if (flag == ERR_MESSAGE_PCKT)
		{
			clientExit(fd);
		}
		else if (flag == HANDLE_LIST_REQUEST)
		{
			listReply(fd);
		}
		else if (flag == HANDLE_INFO)
		{
			try
			{
				handleRequestResponse(fd, messagePacket);
			}
			catch (int ex)
			{
				if (ex == LIST_COMMAND_FAIL)
					invalidHandleRequest(fd, messagePacket);
				else
					throw ex;
			}
		}

		delete messagePacket;
	}
	else
	{
		closeClient(fd);
	}
}

void Server::initPacket(int fd, Message *messagePacket)
{
	try
	{
		addHandle(fd, messagePacket->getSource(), messagePacket->getSourceLen());
		validHandle(messagePacket->getSource(), messagePacket->getSourceLen());
	}
	catch (int ex)
	{
		if (ex == INVALID_HANDLE)
		{
			invalidHandle(fd, messagePacket->getSource(), messagePacket->getSourceLen());
		}
		else{
			printf("Caught exception %d while learning/confirming handle\n", ex);
		}
	}
}

void Server::forward(int fd, Message *messagePacket, int msg_size)
{
	int route;
	try
	{
		if (messagePacket->getDestinationLen() == 0)
		{
			broadcastMessage(messagePacket, msg_size);
		}
		else
		{
			route = fdByHandle(messagePacket->getDestination(), messagePacket->getDestinationLen());
			if (send(route, messagePacket->sendPacket(), msg_size, 0) == GEN_ERROR){
				throw SEND_CALL_FAIL;
			}
		}
	}
	catch (int ex)
	{
		if (ex == INVALID_HANDLE)
		{
			invalidDestination(fd, messagePacket->getDestination(), messagePacket->getDestinationLen());
		}
		else{
			throw ex;
		}
	}
}

void Server::initPacketResponse()
{
	tableSize();
	openConnections[openSocketCount] = accept(socketNum, NULL, 0);
	activeClients[openSocketCount] = (char *)calloc(1, sizeof(char));

	if (openConnections[openSocketCount] == GEN_ERROR){
		throw ACCEPT_CALL_FAIL;
	}
	if (openConnections[openSocketCount] > lastFD){
		lastFD = openConnections[openSocketCount];
	}
	openSocketCount++;
	clientCount++;
}

void Server::addHandle(int fd, char *handle, int length)
{
	int index = 0;
	for (int i = 0; i < openSocketCount; i++)
		if (openConnections[i] == fd)
			index = i;

	if (handle[0] == '\0' || findHandle(handle, length))
	{
		throw INVALID_HANDLE;
	}

	if (activeClients[index] != NULL)
	{
		free(activeClients[index]);
		activeClients[index] = NULL;
	}
	activeClients[index] = (char *)calloc(length + 1, sizeof(char));
	memcpy(activeClients[index], handle, length);
	activeClients[index][length] = '\0';
}

void Server::broadcastMessage(Message *messagePacket, int msg_size)
{
	char *sender;
	int length;

	length = messagePacket->getSourceLen();
	sender = (char *)calloc(length + 1, sizeof(char));
	memcpy(sender, messagePacket->getSource(), length);
	sender[length] = '\0';

	for (int i = 0; i < openSocketCount; i++){
		if (strcmp(sender, activeClients[i]) != 0){
			if (send(openConnections[i], messagePacket->sendPacket(), msg_size, 0) == GEN_ERROR){
				throw SEND_CALL_FAIL;
			}
		}
	}
}

void Server::closeClient(int fd)
{
	int index = 0;
	close(fd);
	for (int i = 0; i < openSocketCount; i++)
	{
		if (openConnections[i] == fd)
		{
			index = i;
			break;
		}
	}
	openSocketCount--;
	clientCount--;
	if (index != openSocketCount){
		openConnections[index] = openConnections[openSocketCount];
	} else {
		openConnections[index] = 0;
	}
	if (index != openSocketCount)
	{
		free(activeClients[index]);
		activeClients[index] = (char *)calloc(strlen(activeClients[openSocketCount]), sizeof(char));
		strcpy(activeClients[index], activeClients[openSocketCount]);
	}
	free(activeClients[openSocketCount]);
	activeClients[openSocketCount] = NULL;
}

void Server::validHandle(char *dest, int destinationHandleLen)
{
	Message *messagePacket;
	int client_fd, error, msg_size;

	messagePacket = new Message();
	messagePacket->destinationHandle(dest, destinationHandleLen);
	messagePacket->setFlag(GOOD_HANDLE);
	messagePacket->sequenceNumber(seqNum++);
	msg_size = messagePacket->messagePacket();
	client_fd = fdByHandle(dest, destinationHandleLen);
	error = send(client_fd, messagePacket->sendPacket(), msg_size, 0);
	if (error == GEN_ERROR)
	{
		throw SEND_CALL_FAIL;
	}
	getTables();
	delete messagePacket;
}

void Server::invalidHandle(int fd, char *handle, int length)
{
	Message *messagePacket;
	int error, msg_size;
	printf("Bad handle ");
	for (int i = 0; i < length; i++){
		printf("%c", handle[i]);
	}
	printf("\n");

	messagePacket = new Message();
	messagePacket->destinationHandle(handle, length);
	messagePacket->setFlag(ERR_INITIAL_PCKT);
	messagePacket->sequenceNumber(seqNum++);
	msg_size = messagePacket->messagePacket();
	error = send(fd, messagePacket->sendPacket(), msg_size, 0);

	if (error == GEN_ERROR)
	{
		throw SEND_CALL_FAIL;
	}
	delete messagePacket;
	closeClient(fd);
}

void Server::listReply(int fd)
{
	Message *messagePacket;
	int error, msg_size;

	printf("Sending list length %d", openSocketCount);
	messagePacket = new Message();
	messagePacket->setFlag(HANDLE_LIST_LEN);
	messagePacket->setIndex(openSocketCount);
	messagePacket->sequenceNumber(seqNum++);
	msg_size = messagePacket->messagePacket();
	error = send(fd, messagePacket->sendPacket(), msg_size, 0);

	if (error == GEN_ERROR)
	{
		throw SEND_CALL_FAIL;
	}
	delete messagePacket;
}

void Server::handleRequestResponse(int fd, Message *client_msg)
{
	Message *messagePacket;
	int error, msg_size, index;
	char handle[MAX_HANDLE_LEN + 2];
	int handle_length;

	index = ntohl(((uint32_t *)client_msg->getPayload())[0]);

	// if (index < 0 || index > openSocketCount){
	// 	throw LIST_COMMAND_FAIL;
	// }

	strcpy(handle + 1, activeClients[index]);
	handle_length = strlen(handle + 1);
	handle[0] = handle_length;
	messagePacket = new Message();
	messagePacket->sourceHandle("Server", 6);
	messagePacket->setFlag(END_OF_LIST);
	messagePacket->setPayload(handle, handle_length + 1);
	messagePacket->sequenceNumber(seqNum++);
	msg_size = messagePacket->messagePacket();
	error = send(fd, messagePacket->sendPacket(), msg_size, 0);

	if (error == GEN_ERROR)
	{
		throw SEND_CALL_FAIL;
	}
	delete messagePacket;
}

void Server::invalidHandleRequest(int fd, Message *client_msg)
{
	Message *messagePacket;
	int error, msg_size;

	messagePacket = new Message();
	messagePacket->setFlag(BAD_HANDLE_REQUEST);
	messagePacket->sequenceNumber(seqNum++);
	msg_size = messagePacket->messagePacket();
	error = send(fd, messagePacket->sendPacket(), msg_size, 0);

	if (error == GEN_ERROR)
	{
		throw SEND_CALL_FAIL;
	}
	delete messagePacket;
}

void Server::invalidDestination(int fd, char *handle, int length)
{
	Message *messagePacket;
	int error, msg_size;

	printf("Unknown destination ");
	messagePacket = new Message();
	messagePacket->destinationHandle(handle, length);
	messagePacket->setFlag(ERR_MESSAGE_PCKT);
	messagePacket->sequenceNumber(seqNum++);
	msg_size = messagePacket->messagePacket();
	error = send(fd, messagePacket->sendPacket(), msg_size, 0);

	if (error == GEN_ERROR)
	{
		throw SEND_CALL_FAIL;
	}
	delete messagePacket;
}

void Server::clientExit(int fd)
{
	Message *messagePacket;
	int msg_size, error;

	messagePacket = new Message();
	messagePacket->setFlag(EXIT_REQUEST);
	messagePacket->sequenceNumber(seqNum++);
	msg_size = messagePacket->messagePacket();
	error = send(fd, messagePacket->sendPacket(), msg_size, 0);

	if (error == GEN_ERROR){
		throw SEND_CALL_FAIL;
	}
	closeClient(fd);
}

int Server::fdByHandle(char *handle, int length)
{
	char *handle_str;

	handle_str = (char *)calloc(length + 1, sizeof(char));
	memcpy(handle_str, handle, length);
	for (int i = 0; i < clientCount; i++)
	{
		if (strcmp(handle_str, activeClients[i]) == 0)
		{
			free(handle_str);
			return openConnections[i];
		}
	}
	free(handle_str);
	throw INVALID_HANDLE;
}

int Server::findHandle(char *handle, int length)
{
	char *handle_str;

	handle_str = (char *)calloc(length + 1, sizeof(char));
	memcpy(handle_str, handle, length);
	handle_str[length] = '\0';

	for (int i = 0; i < clientCount; i++)
	{
		if (strcmp(handle_str, activeClients[i]) == 0)
		{
			free(handle_str);
			return 1;
		}
	}

	free(handle_str);
	return 0;
}

void Server::tableSize()
{
	int *new_sockets;
	char **new_clients;

	if (openSocketCount == capacity - 2)
	{
		new_sockets = (int *)calloc(2 * capacity, sizeof(int));
		new_clients = (char **)calloc(2 * capacity, sizeof(char *));
		memcpy(new_sockets, openConnections, capacity);
		memcpy(new_clients, activeClients, capacity);
		free(openConnections);
		free(activeClients);
		openConnections = new_sockets;
		activeClients = new_clients;
		capacity = 2 * capacity;
	}
}

void Server::getTables()
{
	printf("\nCurrent handles:\n");
	printf("----------------\n");
	printf("fd        handle\n");
	printf("----------------\n");
	for (int i = 0; i < openSocketCount; i++){
		printf("%d         %s\n", openConnections[i], activeClients[i]);
	}
	printf("\n\n");
}

void serverException(int ex) {
	if (ex == INVALID_ARGUMENT) {
		cout << "server <port #>";
	} else if (ex == SOCKET_ERROR) {
		cout << "Socket failed, errno " << errno << "\n";
	} else if (ex == BIND_FAIL) {
		cout << "Failed dest bind, errno " << errno << "\n";
	} else if (ex == LISTEN_CALL_FAIL) {
		cout << "listen(), errno " << errno << "\n";
	} else if (ex == SELECT_CALL_FAIL) {
		cout << "select(), errno " << errno << "\n";
	} else if (ex == ACCEPT_CALL_FAIL) {
		cout << "accept(), errno " << errno << "\n";
	} else if (ex == SEND_CALL_FAIL) {
		cout << "send(), errno " << errno << "\n";
	} else {
		cout << "Unknown exception.\n";
	}
}

int main(int argc, char **argv)
{
	Server *server;
	try {
		if (argc > 2){
			throw INVALID_ARGUMENT;
		}
		server = new Server(argc, argv);
		server->setup();
		server->serve();
	} catch (int ex) {
		serverException(ex);
	}
}
