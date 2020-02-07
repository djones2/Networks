#ifndef CCLIENT_H
#define CCLIENT_H

/* Client structure */
class Cclient {
	int seqNum;
	char *handle;
	struct hostent *server;
	int port;
	int socket_fd;
	struct sockaddr_in server_address;
	bool exit;
	
public:
	/* Main client operations */
	Cclient(char *handle, struct hostent *server, int port);
	void createClient();
	void clientMain();
	void clientExit();
	/* %_ functions */
	int sendClientMessage(char *input);
	void broadcast(char *input);
	void listCommand();
	int  listLengthRequest();
	void requestHandle(int index);
	/* Helper functions */
	void validHandle();
	void unpackMessage();
	void getInput();
	void clientWrap(char* handle, struct hostent *server, int port, Cclient *client);
};

#endif
