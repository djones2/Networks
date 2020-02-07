#ifndef SERVER_H
#define SERVER_H

#include "message.h"

/* Server structure */
class Server {
	fd_set read_set;
	int seqNum;
	int socketNum;
	int currentSocket;
	int *openConnections;
	int openSocketCount;
	int clientCount;
	int capacity;
	int lastFD;
	int port;
	char **activeClients;
	struct sockaddr_in address;
	struct timeval select_timer;
	
public: 
	/* Main server functions */
	Server(int argc, char **argv);
	void setup();
	void serve();
	/* Server actions */
	void initPacketResponse();
	void addHandle(int fd, char *handle, int length);
	void broadcastMessage(Message *messagePacket, int msg_size);
	void closeClient(int fd);
	/* Packet processing */
	void processMessage(int fd);	
	void initPacket(int fd, Message *messagePacket);
	void forward(int fd, Message *messagePacket, int msg_size);
	/* Acknowledge/reject actions */
	void validHandle(char *dest, int destinationHandleLen);
	void invalidHandle(int fd, char *handle, int length);
	void listReply(int fd);
	void handleRequestResponse(int fd, Message *client_msg);
	void invalidHandleRequest(int fd, Message *client_msg);
	void invalidDestination(int fd, char *handle, int length);
	void clientExit(int fd);
	/* Helper functions */
	int fdByHandle(char *handle, int length);
	int findHandle(char *handle, int length);
	void tableSize();
	void getTables();
	void serverException(int ex);
};

#endif
