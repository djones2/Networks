#ifndef SERVER_H
#define SERVER_H

#include "message.h"

class Server {	
	int answering_machine; // socket to listen() on
	int current_socket;
	int *open_sockets; // open connections to clients
	char **clients; // array of client handles
	int num_open;
	int num_clients;
	int capacity;
	int highest_fd;
	fd_set read_set;
	struct timeval wait_time;
	int port;
	struct sockaddr_in address;
	//int unconfirmed_clients;
	int sequence_number;
	
public: // Lifecycle
	Server(int argc, char **argv);
	void setup();
	void serve();
	void shutdown();
	
private: 
	// Packets
	void process_packet(int fd);	
	void initial_packet(int fd, Message *msg);
	void message_packet(int fd, Message *msg, int msg_size);
	
	// Workflow
	void answer();
	void learn_handle(int fd, char *handle, int length);
	void broadcast(Message *msg, int msg_size);
	void close_connection(int fd);
	
	// Responses to client
	void handleCheck(char *to, int to_length);
	void bad_handle_response(int fd, char *handle, int length);
	void list_length_response(int fd);
	void handle_request_response(int fd, Message *client_msg);
	void bad_handle_request_response(int fd, Message *client_msg);
	void bad_destination_response(int fd, char *handle, int length);
	void exit_response(int fd);
	
	// Utility
	int  find_by_handle(char *handle, int length);
	int handle_exists(char *handle, int length);
	void resize();
	void print_tables();
};

#endif
