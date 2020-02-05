#include <iostream>
#include <sstream>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <cerrno>

#include "server.h"
#include "message.h"

#define MAXBUF 1024

using namespace std;

/*
 *	 Lifecycle Functions
 */ 

Server::Server(int argc, char **argv) {
	port = 0;

	if (argc == 2)
		stringstream(argv[1]) >> port;
	else
		port = 0;
}

void Server::setup() {
	socklen_t address_length;
	
	answering_machine = socket(AF_INET, SOCK_STREAM, 0);
	sequence_number = 0;
	
	if (answering_machine == -1)
		throw SOCK_EX;

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(port);

	if (bind(answering_machine, (const sockaddr*)&address, sizeof(address)) == -1)
		throw BIND_EX;

	if (listen(answering_machine, 10) == -1)
		throw LISTEN_EX;

	address_length = sizeof(address);

	if (getsockname(answering_machine, (struct sockaddr*)&address, &address_length) == -1)
		throw PORT_EX;
		
	if (port == 0)
		printf("Server is using port %d\n", ntohs(address.sin_port));

	highest_fd = answering_machine;

	num_open = 0;
	num_clients = 0;
	
	capacity = MAXBUF;
	open_sockets = (int *) calloc(capacity, sizeof(int));
	clients = (char **) calloc(capacity, sizeof(char *));
}

void Server::serve() {	
	// service loop
	while(true) {
		
		//setup 
		FD_ZERO(&read_set);
		wait_time.tv_sec = 1;
		wait_time.tv_usec = 1000;
	
		FD_SET(answering_machine, &read_set);
		for (int i = 0; i < num_open; i++) {
			current_socket = open_sockets[i];
			FD_SET(current_socket, &read_set);
		}
	
		//select
		if (select(highest_fd + 1, &read_set, NULL, NULL, &wait_time) == -1)
			throw SELECT_EX;
		
		//set up new connection
		if (FD_ISSET(answering_machine, &read_set))
			answer();

		//handle client traffic
		for (int i = 0; i < num_open; i++) {
			current_socket = open_sockets[i];
			if (FD_ISSET(current_socket, &read_set)) {
				//receive from a client
				process_packet(current_socket);
			}
		}
	}
}

void Server::shutdown() {
	
}

/*
 *	 Packet Processing Functions
 */ 
 
void Server::process_packet(int fd) {
	uint8_t buffer[MAXBUF];
	int message_size, flag;
	Message *msg;

	message_size = recv(fd, buffer, MAXBUF, 0);

	if (message_size) {
		
		msg = new Message(buffer, message_size);
		msg->print();
		flag = msg->get_flag();
		
		if (flag == 1) 
			initial_packet(fd, msg);
		else if (flag == 6) {
			message_packet(fd, msg, message_size);
		}
		else if (flag == 8) {
			exit_response(fd);
		}
		else if (flag == 10) {
			list_length_response(fd);
		}
		else if (flag == 12) {
			try {
				handle_request_response(fd, msg);
			}
			catch (int ex) {
				if (ex == LIST_EX)
					bad_handle_request_response(fd, msg);
				else
					throw ex;
			}
		}
		
		
		delete msg;
	}
	// Connection closed externally
	else {
		//printf("Cclient dropped!\n");
		close_connection(fd);
	}
	
}

void Server::initial_packet(int fd, Message *msg) {
	try {
		learn_handle(fd, msg->get_from(), msg->get_from_length());
		handleCheck(msg->get_from(), msg->get_from_length());
	}
	catch (int ex) {
		if (ex == HANDLE_EX) {
			//printf("Handle is null or already exists\n");
			bad_handle_response(fd, msg->get_from(), msg->get_from_length());
		}
		else 
			printf("Caught exception %d while learning/confirming handle\n", ex);
	}
}

void Server::message_packet(int fd, Message *msg, int msg_size) {
	int route;
	
	try {
		if (msg->get_to_length() == 0) {
			broadcast(msg, msg_size);
		}
		else {
			route = find_by_handle(msg->get_to(), msg->get_to_length());
			if (send(route, msg->sendable(), msg_size, 0) == -1)
				throw SEND_EX;
		}
	}
	catch (int ex) {
		if (ex == HANDLE_EX) {
			bad_destination_response(fd, msg->get_to(), msg->get_to_length());
		}
		else
			throw ex;
	}
}

/*
 *	 Workflow Functions
 */ 

void Server::answer() {
	resize();
	
	open_sockets[num_open] = accept(answering_machine, NULL, 0);
	clients[num_open] = (char *) calloc(1, sizeof(char));
	
	if (open_sockets[num_open] == -1)
		throw ACCEPT_EX;
		
	if (open_sockets[num_open] > highest_fd)
		highest_fd = open_sockets[num_open];
		
	num_open++;
	num_clients++;
}

void Server::learn_handle(int fd, char *handle, int length) {
	int index = 0;
	
	for (int i = 0; i < num_open; i++) 
		if (open_sockets[i] == fd)
			index = i;
	
	if (handle[0] == '\0' || handle_exists(handle, length)) {
		throw HANDLE_EX;
	}	
	
	if (clients[index] != NULL) {
		free(clients[index]);
		clients[index] = NULL;
	}
		
	clients[index] = (char *) calloc(length + 1, sizeof(char));
	memcpy(clients[index], handle, length);
	clients[index][length] = '\0';
}

void Server::broadcast(Message *msg, int msg_size) {
	char *sender;
	int length;
	
	length = msg->get_from_length();
	
	sender = (char *) calloc(length + 1, sizeof(char));
	memcpy(sender, msg->get_from(), length);
	sender[length] = '\0';

	for (int i = 0; i < num_open; i++)
		if (strcmp(sender, clients[i]) != 0)
			if (send(open_sockets[i], msg->sendable(), msg_size, 0) == -1)
				throw SEND_EX; 
}

void Server::close_connection(int fd) {
	close(fd);
	int i;
	int index = 0;
	for (i = 0; i < num_open; i++) {
		if (open_sockets[i] == fd) {
			index = i;
			break;
		}
	}
	
	num_open--;
	num_clients--;
	
	if (index != num_open) 
		open_sockets[index] = open_sockets[num_open];
	else
		open_sockets[index] = 0;
	
	if (index != num_open) {
		free(clients[index]);
		clients[index] = (char *)calloc(strlen(clients[num_open]), sizeof(char));
		strcpy(clients[index], clients[num_open]);
	}
	
	free(clients[num_open]);
	clients[num_open] = NULL;
}

/*
 *	Responses to Cclient
 */
void Server::handleCheck(char *to, int to_length) {
	Message *msg;
	int client_fd, error, msg_size;
	
	msg = new Message();
	msg->set_to(to, to_length);
	msg->set_flag(2);
	msg->set_sequence_number(sequence_number++);
	msg_size = msg->pack();
	
	client_fd = find_by_handle(to, to_length);
		
	error = send(client_fd, msg->sendable(), msg_size, 0);
		
	if (error == -1) {
		throw SEND_EX;
	}
	
	print_tables();
	
	delete msg;
}

void Server::bad_handle_response(int fd, char *handle, int length) {
	Message *msg;
	int error, msg_size;
	
	printf("Bad handle ");
	for (int i = 0; i < length; i++)
		printf("%c", handle[i]);
	printf("\n");
	
	msg = new Message();
	msg->set_to(handle, length);
	msg->set_flag(3);
	msg->set_sequence_number(sequence_number++);
	msg_size = msg->pack();
		
	error = send(fd, msg->sendable(), msg_size, 0);
		
	if (error == -1) {
		throw SEND_EX;
	}

	delete msg;
	
	close_connection(fd);
}

void Server::list_length_response(int fd) {
	Message *msg;
	int error, msg_size;
	
	printf("Sending list length %d", num_open);
	msg = new Message();
	msg->set_flag(11);
	msg->set_int(num_open);
	msg->set_sequence_number(sequence_number++);
	msg_size = msg->pack();
		
	error = send(fd, msg->sendable(), msg_size, 0);
		
	if (error == -1) {
		throw SEND_EX;
	}

	delete msg;
}

void Server::handle_request_response(int fd, Message *client_msg) {
	Message *msg;
	int error, msg_size, index;
	char handle[MAX_HANDLE_LENGTH + 2];
	int handle_length;
	
	index = ntohl(((uint32_t *)client_msg->get_text())[0]);
	
	if (index < 0 || index > num_open) 
		throw LIST_EX;
	
	strcpy(handle+1, clients[index]);
	handle_length = strlen(handle+1);
	handle[0] = handle_length;
	
	//printf("Sending handle %s", handle);
	msg = new Message();
	msg->set_from("Server", 6);
	msg->set_flag(13);
	msg->set_text(handle, handle_length + 1);
	msg->set_sequence_number(sequence_number++);
	msg_size = msg->pack();
		
	error = send(fd, msg->sendable(), msg_size, 0);
		
	if (error == -1) {
		throw SEND_EX;
	}

	delete msg;
}

void Server::bad_handle_request_response(int fd, Message *client_msg) {
	Message *msg;
	int error, msg_size;
	
	msg = new Message();
	msg->set_flag(14);
	msg->set_sequence_number(sequence_number++);
	msg_size = msg->pack();
		
	error = send(fd, msg->sendable(), msg_size, 0);
		
	if (error == -1) {
		throw SEND_EX;
	}

	delete msg;
}

void Server::bad_destination_response(int fd, char *handle, int length) {
	Message *msg;
	int error, msg_size;
	
	printf("Unknown destination ");
	msg = new Message();
	msg->set_to(handle, length);
	msg->set_flag(7);
	msg->set_sequence_number(sequence_number++);
	msg_size = msg->pack();
		
	error = send(fd, msg->sendable(), msg_size, 0);
		
	if (error == -1) {
		throw SEND_EX;
	}

	delete msg;

}

void Server::exit_response(int fd) {
	Message *msg;
	int msg_size, error;
	
	msg = new Message();
	msg->set_flag(9);
	msg->set_sequence_number(sequence_number++);
	msg_size = msg->pack();
	
	error = send(fd, msg->sendable(), msg_size, 0);
	
	if (error == -1)
		throw SEND_EX;
		
	close_connection(fd);
}

/*
 *	Utility Functions
 */

int Server::find_by_handle(char *handle, int length) {
	char *handle_str;
	
	handle_str = (char *) calloc(length + 1, sizeof(char));
	memcpy(handle_str, handle, length);
	for (int i = 0; i < num_clients; i++) {
		if (strcmp(handle_str, clients[i]) == 0) {
			free(handle_str);
			return open_sockets[i];
		}
	}
	
	free(handle_str);
	throw HANDLE_EX;
}

int Server::handle_exists(char *handle, int length) {
	char *handle_str;
	
	handle_str = (char *) calloc(length + 1, sizeof(char));
	memcpy(handle_str, handle, length);
	handle_str[length] = '\0';
	
	for (int i = 0; i < num_clients; i++) {
		if (strcmp(handle_str, clients[i]) == 0) {
			free(handle_str);
			return 1;
		}
	}
	
	free(handle_str);
	return 0;
}

void Server::resize() {
	int *new_sockets;
	char **new_clients;

	if (num_open == capacity - 2) {
		new_sockets = (int *)calloc(2*capacity, sizeof(int));
		new_clients = (char **)calloc(2*capacity, sizeof(char *));
		memcpy(new_sockets, open_sockets, capacity);
		memcpy(new_clients, clients, capacity);
		free(open_sockets);
		free(clients);
		open_sockets = new_sockets;
		clients = new_clients;
		capacity = 2*capacity;
	}
}

void Server::print_tables() {
	printf("\nCurrent handles:\n");
	printf("----------------\n");
	printf("fd        handle\n");
	printf("----------------\n");
	for (int i = 0; i < num_open; i++) 
		printf("%d         %s\n", open_sockets[i], clients[i]);
	printf("\n\n");
}