#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <cerrno>
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include "server.h"
#include "message.h"

using namespace std;

/* Create server instance on port */
Server::Server(int argc, char **argv)
{
	port = 0;
	if (argc != 2)
	{
		port = 0;
	}
	else if (argc == 2)
	{
		stringstream(argv[1]) >> port;
	}
}

/* Populate server attributes */
void Server::setup()
{
	socklen_t address_length;
	socketNum = socket(AF_INET, SOCK_STREAM, 0);
	sequence_number = 0;

	if (socketNum == GEN_ERROR)
	{
		throw CLIENT_NAME_ERROR;
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(port);

	if (listen(socketNum, 10) == GEN_ERROR)
	{
		throw LISTEN_EX;
	}

	address_length = sizeof(address);

	if (getsockname(socketNum, (struct sockaddr *)&address, &address_length) == GEN_ERROR)
	{
		throw PORT_EX;
	}

	if (port == 0)
	{
		printf("Server on port %d\n", ntohs(address.sin_port));
	}

	capacity = MAXBUF;
	num_open = 0;
	num_clients = 0;
	highest_fd = socketNum;
	socketTable = (int *)calloc(capacity, sizeof(int));
	clientTable = (char **)calloc(capacity, sizeof(char *));
}

/* Live server call, includes (not) busy week */
void Server::liveServer()
{
	while (1)
	{
		int i;
		FD_ZERO(&read_set);
		/* Set time for select call */
		wait_time.tv_sec = 1;
		wait_time.tv_usec = 1000;
		FD_SET(socketNum, &read_set);
		for (i = 0; i < num_open; i++)
		{
			currentClientSocket = socketTable[i];
			FD_SET(currentClientSocket, &read_set);
		}

		if (select(highest_fd + 1, &read_set, NULL, NULL, &wait_time) == GEN_ERROR)
		{
			throw SELECT_EX;
		}

		/* Create new connection on open socket */
		if (FD_ISSET(socketNum, &read_set))
		{
			answer();
		}

		/* Handle client traffic across socket table */
		for (i = 0; i < num_open; i++)
		{
			currentClientSocket = socketTable[i];
			if (FD_ISSET(currentClientSocket, &read_set))
			{
				process_packet(currentClientSocket);
			}
		}
	}
}

/* Process incoming packets */
void Server::process_packet(int fd)
{
	uint8_t buffer[MAXBUF];
	int flag;
	Message *messagePacket;

	int message_size = recv(fd, buffer, MAXBUF, 0);

	if (message_size)
	{
		messagePacket = new Message(buffer, message_size);
		messagePacket->print();
		flag = messagePacket->get_flag();

		if (flag == INITIAL_PCKT)
		{
			initial_packet(fd, messagePacket);
		}
		else if (flag == MESSAGE_PCKT)
		{
			message_packet(fd, messagePacket, message_size);
		}
		else if (flag == EXIT_REPLY)
		{
			exitResponse(fd);
		}
		else if (flag == HANDLE_LIST_REQUEST)
		{
			listCommandLength(fd);
		}
		else if (flag == HANDLE_LIST_LEN)
		{
			try
			{
				requestResponse(fd, messagePacket);
			}
			catch (int ex)
			{
				if (ex == LIST_EX)
					invalidHandleRequest(fd, messagePacket);
				else
					throw ex;
			}
		}

		delete messagePacket;
	}
	/* Control-C on one side */
	else
	{
		close_connection(fd);
	}
}

/* Serving initial packet */
void Server::initial_packet(int fd, Message *messagePacket)
{
	try
	{
		learn_handle(fd, messagePacket->getSource(), messagePacket->get_from_length());
		handleCheck(messagePacket->getSource(), messagePacket->get_from_length());
	}
	catch (int ex)
	{
		if (ex == HANDLE_EX)
		{
			badHandle(fd, messagePacket->getSource(), messagePacket->get_from_length());
		}
	}
}

/* Serving message packet forwarding */
void Server::message_packet(int fd, Message *messagePacket, int payloadLen)
{
	try
	{
		if (messagePacket->get_to_length() == 0)
		{
			broadcast(messagePacket, payloadLen);
		}
		else
		{
			int route = fdByHandle(messagePacket->get_to(), messagePacket->get_to_length());
			/* send() call */
			if (send(route, messagePacket->sendable(), payloadLen, 0) == GEN_ERROR)
				throw SEND_FAIL;
		}
	}
	catch (int ex)
	{
		if (ex == HANDLE_EX)
		{
			invalidDestination(fd, messagePacket->get_to(), messagePacket->get_to_length());
		}
		else
			throw ex;
	}
}

/* Answer the initial packet of a client, add to socket and
	client tables */
void Server::answer()
{
	resize();
	socketTable[num_open] = accept(socketNum, NULL, 0);
	clientTable[num_open] = (char *)calloc(1, sizeof(char));
	if (socketTable[num_open] == GEN_ERROR)
	{
		throw ACCEPT_EX;
	}
	if (socketTable[num_open] > highest_fd)
	{
		highest_fd = socketTable[num_open];
	}
	num_clients++;
	num_open++;
}
/* Add handle to client table */
void Server::learn_handle(int fd, char *handle, int length)
{
	int index = 0;
	int i;
	/* Find file descriptor in socket table */
	for (i = 0; i < num_open; i++)
	{
		if (socketTable[i] == fd)
		{
			index = i;
		}
	}
	/* Check if client table is available.
		If not, overwrite */
	if (clientTable[index] != NULL)
	{
		free(clientTable[index]);
		clientTable[index] = NULL;
	}
	/* See if handle is valid */
	if (handle[0] == '\0' || findHandle(handle, length))
	{
		throw HANDLE_EX;
	}
	/* Arithmetic for client table */
	clientTable[index] = (char *)calloc(length + 1, sizeof(char));
	memcpy(clientTable[index], handle, length);
	clientTable[index][length] = '\0';
}

/* Broadcast command */
void Server::broadcast(Message *messagePacket, int payloadLen)
{
	/* Get lendth and handle for sender */
	int length = messagePacket->get_from_length();
	char *sender;
	sender = (char *)calloc(length + 1, sizeof(char));
	memcpy(sender, messagePacket->getSource(), length);
	sender[length] = '\0';

	/* Send message to all client in client table */
	for (int i = 0; i < num_open; i++)
	{
		if (strcmp(sender, clientTable[i]) != 0)
		{
			if (send(socketTable[i], messagePacket->sendable(), payloadLen, 0) == GEN_ERROR)
			{
				throw SEND_FAIL;
			}
		}
	}
}

/* Close server connection with client via file descriptor */
void Server::close_connection(int fd)
{
	close(fd);
	int index = 0;
	/* Fine socket value in socket table */
	for (int i = 0; i < num_open; i++)
	{
		if (socketTable[i] == fd)
		{
			index = i;
			break;
		}
	}
	/* Adjust global values */
	num_clients--;
	num_open--;

	if (index != num_open)
	{
		socketTable[index] = socketTable[num_open];
	}
	else
	{
		socketTable[index] = 0;
	}

	if (index != num_open)
	{
		free(clientTable[index]);
		clientTable[index] = (char *)calloc(strlen(clientTable[num_open]), sizeof(char));
		strcpy(clientTable[index], clientTable[num_open]);
	}
	free(clientTable[num_open]);
	clientTable[num_open] = NULL;
}

/* Validate handle upon request */
void Server::handleCheck(char *destination, int destinationLength)
{
	Message *messagePacket;
	int error;
	/* Create message packet */
	messagePacket = new Message();
	messagePacket->set_to(destination, destinationLength);
	messagePacket->set_flag(GOOD_HANDLE);
	messagePacket->set_sequence_number(sequence_number++);
	int payloadLen = messagePacket->packet();

	/* Get file descriptor by the handle */
	int client_fd = fdByHandle(destination, destinationLength);

	/* Send call */
	if ((error = send(client_fd, messagePacket->sendable(), payloadLen, 0)) == GEN_ERROR)
	{
		throw SEND_FAIL;
	}
	print_tables();
	delete messagePacket;
}

/* Function for bad handles */
void Server::badHandle(int fd, char *handle, int length)
{
	Message *messagePacket;
	int error;

	/* Print the handle */
	printf("Bad handle ");
	for (int i = 0; i < length; i++)
	{
		printf("%c", handle[i]);
	}
	printf("\n");

	messagePacket = new Message();
	messagePacket->set_to(handle, length);
	messagePacket->set_flag(ERR_INITIAL_PCKT);
	messagePacket->set_sequence_number(sequence_number++);
	int payloadLen = messagePacket->packet();

	/* Send command to close the connection of the bad handle */
	if ((error = send(fd, messagePacket->sendable(), payloadLen, 0)) == GEN_ERROR)
	{
		throw SEND_FAIL;
	}
	delete messagePacket;
	close_connection(fd);
}

/* These next few function signatures explain the function.
	They are nearly identical in execution. */
void Server::listCommandLength(int fd)
{
	Message *messagePacket;
	int error;

	printf("Sending list length %d", num_open);
	messagePacket = new Message();
	messagePacket->set_flag(HANDLE_LIST_LEN);
	messagePacket->set_int(num_open);
	messagePacket->set_sequence_number(sequence_number++);
	int payloadLen = messagePacket->packet();

	if ((error = send(fd, messagePacket->sendable(), payloadLen, 0)) == GEN_ERROR)
	{
		throw SEND_FAIL;
	}

	delete messagePacket;
}

void Server::requestResponse(int fd, Message *clientMessagePacket)
{
	Message *messagePacket;
	int error;
	char handle[MAX_HANDLE_LENGTH + 2];

	int index = ntohl(((uint32_t *)clientMessagePacket->get_text())[0]);

	if (index < 0 || index > num_open)
		throw LIST_EX;

	strcpy(handle + 1, clientTable[index]);
	int handle_length = strlen(handle + 1);
	handle[0] = handle_length;

	messagePacket = new Message();
	messagePacket->set_from("Server", 6);
	messagePacket->set_flag(END_OF_LIST);
	messagePacket->set_text(handle, handle_length + 1);
	messagePacket->set_sequence_number(sequence_number++);
	int payloadLen = messagePacket->packet();

	if ((error = send(fd, messagePacket->sendable(), payloadLen, 0)) == GEN_ERROR)
	{
		throw SEND_FAIL;
	}
	delete messagePacket;
}

void Server::invalidHandleRequest(int fd, Message *clientMessagePacket)
{
	Message *messagePacket;
	int error;

	messagePacket = new Message();
	messagePacket->set_flag(CLIENT_NAME_ERROR);
	messagePacket->set_sequence_number(sequence_number++);
	int payloadLen = messagePacket->packet();

	if ((error = send(fd, messagePacket->sendable(), payloadLen, 0)) == GEN_ERROR)
	{
		throw SEND_FAIL;
	}

	delete messagePacket;
}

void Server::invalidDestination(int fd, char *handle, int length)
{
	Message *messagePacket;
	int error;

	printf("Unknown destination ");
	messagePacket = new Message();
	messagePacket->set_to(handle, length);
	messagePacket->set_flag(7);
	messagePacket->set_sequence_number(sequence_number++);
	int payloadLen = messagePacket->packet();

	error = send(fd, messagePacket->sendable(), payloadLen, 0);

	if (error == GEN_ERROR)
	{
		throw SEND_FAIL;
	}

	delete messagePacket;
}

void Server::exitResponse(int fd)
{
	Message *messagePacket;
	int error;

	messagePacket = new Message();
	messagePacket->set_flag(EXIT_REPLY);
	messagePacket->set_sequence_number(sequence_number++);
	int payloadLen = messagePacket->packet();

	if ((error = send(fd, messagePacket->sendable(), payloadLen, 0)) == GEN_ERROR)
		throw SEND_FAIL;

	close_connection(fd);
}

/* Find file descriptor by the handle name */
int Server::fdByHandle(char *handle, int length)
{
	char *handle_str;
	handle_str = (char *)calloc(length + 1, sizeof(char));
	memcpy(handle_str, handle, length);
	/* Search client table for handle string */
	for (int i = 0; i < num_clients; i++)
	{
		if (strcmp(handle_str, clientTable[i]) == 0)
		{
			free(handle_str);
			return socketTable[i];
		}
	}
	free(handle_str);
	throw HANDLE_EX;
}

/* Verify handle exists, return a boolean-type response. */
int Server::findHandle(char *handle, int length)
{
	char *handle_str;
	handle_str = (char *)calloc(length + 1, sizeof(char));
	memcpy(handle_str, handle, length);
	handle_str[length] = '\0';

	for (int i = 0; i < num_clients; i++)
	{
		if (strcmp(handle_str, clientTable[i]) == 0)
		{
			free(handle_str);
			return 1;
		}
	}

	free(handle_str);
	return 0;
}

/* Print out tables */
void Server::print_tables()
{
	printf("\nCurrent handles:\n");
	printf("----------------\n");
	printf("fd        handle\n");
	printf("----------------\n");
	for (int i = 0; i < num_open; i++)
		printf("%d         %s\n", socketTable[i], clientTable[i]);
	printf("\n\n");
}

/* Resize client and socket tables  */
void Server::resize()
{
	int *new_sockets;
	char **new_clients;
	if (num_open == capacity - 2)
	{
		new_sockets = (int *)calloc(2 * capacity, sizeof(int));
		new_clients = (char **)calloc(2 * capacity, sizeof(char *));
		memcpy(new_sockets, socketTable, capacity);
		memcpy(new_clients, clientTable, capacity);
		free(socketTable);
		free(clientTable);
		socketTable = new_sockets;
		clientTable = new_clients;
		capacity = 2 * capacity;
	}
}

void mainHelp(int argc, char **argv) {
	Server *server;
	if (argc > 2) {
		throw ARGUMENT_ERROR;
	}
	server = new Server(argc, argv);
	server->setup();
	server->liveServer();
}

void exceptionWrap(int ex) {
	if (ex == ARGUMENT_ERROR)
		cout << "server <port #>";
	else if (ex == CLIENT_NAME_ERROR)
		cout << "Socket failed, errno " << errno << "\n";
	else if (ex == BIND_EX)
		cout << "Failed to bind, errno " << errno << "\n";
	else if (ex == LISTEN_EX)
		cout << "listen(), errno " << errno << "\n";
	else if (ex == SELECT_EX)
		cout << "select(), errno " << errno << "\n";
	else if (ex == ACCEPT_EX)
		cout << "accept(), errno " << errno << "\n";
	else if (ex == SEND_FAIL)
		cout << "send(), errno " << errno << "\n";
	else
		cout << "Unknown exception.\n";
}

void mainServerWrapper(int argc, char **argv) {
	try {
		mainHelp(argc, argv);
	} catch (int ex) {
		exceptionWrap(ex);
	}
}

int main(int argc, char **argv) {
	mainServerWrapper(argc, argv);
	return 0;
}