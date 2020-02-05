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
#include <cerrno>

#include "server.h"

using namespace std;

int main(int argc, char **argv) {
	Server *server;
	
	try {
		if (argc > 2)
			throw ARG_EX;
	
		server = new Server(argc, argv);
		server->setup();
		server->serve();
		server->shutdown();
	}
	catch(int ex) {
		if (ex == ARG_EX)
			cout << "server <port #>";
		else if (ex == SOCK_EX)
			cout << "Socket failed, errno "  << errno << "\n";
		else if (ex == BIND_EX)
			cout << "Failed to bind, errno "  << errno << "\n";
		else if (ex == LISTEN_EX)
			cout << "listen(), errno "  << errno << "\n";
		else if (ex == SELECT_EX)
			cout << "select(), errno "  << errno << "\n";
		else if (ex == ACCEPT_EX)
			cout << "accept(), errno "  << errno << "\n";
		else if (ex == SEND_EX)
			cout << "send(), errno "  << errno << "\n";
		else
			cout << "Unknown exception.\n";
	}
}