#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <cerrno>
#include <stdio.h>

#include "message.h"
#include "cclient.h"

using namespace std;

int main(int argc, char **argv) {
	char *handle;
	struct hostent *server;
	int port;
	Cclient *client;
	
	try {
		if (argc != 4)
			throw ARG_EX;
			
		handle = argv[1];
		server = gethostbyname(argv[2]);
		stringstream(argv[3]) >> port;
		
		if (strlen(handle) > MAX_PAYLOAD)
			throw HANDLE_LENGTH_EX;
	
		if (server == NULL)
			throw IP_EX;
	
		client = new Cclient(handle, server, port);
		client->init();
		client->createMessage();
		client->exit();
	}
	catch(int ex) {
		if (ex == ARG_EX)
			cout << "cclient <handle> <server-IP> <server-port>\n";
		else if (ex == IP_EX) 
			cout << "Could not resolve hostname " << argv[2] << "\n";
		else if (ex == SOCK_EX)
			cout << "socket() failed. errno " << errno << "\n";
		else if (ex == CONNECT_EX)
			cout << "connect() failed. errno " << errno << "\n";
		else if (ex == PACK_EX)
			cout << "Message packing failed. errno " << errno << "\n";
		else if (ex == UNPACK_EX)
			cout << "Message unpacking failed. errno " << errno << "\n";
		else if (ex == SEND_EX)
			cout << "Message sending failed. errno " << errno << "\n";
		else if (ex == HANDLE_EX)
			cout << "Handle " << handle << " is already taken.\n";
		else if (ex == HANDLE_LENGTH_EX)
			cout << "Handles must be under 100 characters.\n";
		else
			cout << "Unknown exception " << ex << "\n";
	}
}
