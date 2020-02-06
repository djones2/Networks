#ifndef SERVER_H
#define SERVER_H

#include "message.h"

/* Class for all server instances */
class Server
{
	/* Server attributes */
	fd_set read_set;
	struct timeval wait_time;
	int port;
	struct sockaddr_in address;
	int sequence_number;
	int socketNum; 
	int currentClientSocket;
	int *socketTable;
	char **clientTable;	
	int num_open;
	int num_clients;
	int capacity;
	int highest_fd;

public: 
	/* Server setup */
	Server(int argc, char **argv);
	void setup();
	void liveServer();

private:
	void mainServerWrapper(int argc, char **argv);
	void exceptionWrap(int ex);
	void mainHelp(int argc, char **argv);
	/* Helper functions */
	int fdByHandle(char *handle, int length);
	int findHandle(char *handle, int length);
	void resize();
	void print_tables();
	/* Packet processing */
	void process_packet(int fd);
	void initial_packet(int fd, Message *messagePacket);
	void message_packet(int fd, Message *messagePacket, int payloadLen);
	void answer();
	void learn_handle(int fd, char *handle, int length);
	void broadcast(Message *messagePacket, int payloadLen);
	void close_connection(int fd);
	/* Client interactions */
	void handleCheck(char *destination, int destinationLength);
	void badHandle(int fd, char *handle, int length);
	void listCommandLength(int fd);
	void requestResponse(int fd, Message *clientMessagePacket);
	void invalidHandleRequest(int fd, Message *clientMessagePacket);
	void invalidDestination(int fd, char *handle, int length);
	void exitResponse(int fd);

};

#endif
