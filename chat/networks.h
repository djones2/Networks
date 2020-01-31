
/* 	Code originally give to Prof. Smith by his TA in 1994.
	No idea who wrote it.  Copy and use at your own Risk
*/


#ifndef __NETWORKS_H__
#define __NETWORKS_H__

#define BACKLOG 10

/* A pseudo-hashmap that stores the open
	connections in a network. */
typedef struct socket_handle_map {
	int socket_number;
	char handle[100];
	struct socket_handle_map *next;
} socket_handle_map;

// for the server side
int tcpServerSetup(int portNumber);
int tcpAccept(int server_socket, int debugFlag);

// for the client side
int tcpClientSetup(char * serverName, char * port, int debugFlag);



#endif
