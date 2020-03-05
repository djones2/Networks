
// Hugh Smith April 2017
// Network code to support TCP/UDP client and server connections

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "networks.h"
#include "libcpe464/networks/network-hooks.h"
#include "gethostbyname.h"

int safeGetUdpSocket()
{
	int socketNum = 0;
	if ((socketNum = socket(AF_INET6, SOCK_DGRAM, 0)) < 0)
	{
		perror("safe socket() call error");
		exit(-1);
	}
	return socketNum;
}

int safeSendto(uint8_t *packet, uint32_t len, Connection *to)
{
	int send_len = 0;
	if ((send_len = sendtoErr(to->socket_number, packet, len, 0, (struct sockaddr *)&(to->remote), to->length)) < 0)
	{
		perror("sendto: ");
		exit(-1);
	}
	return send_len;
}

int safeRecvfrom(int recv_sk_num, uint8_t *packet, int len, Connection *from)
{
	int recv_len = 0;
	from->length = sizeof(struct sockaddr_in6);

	if ((recv_len = recvfrom(recv_sk_num, packet, len, 0, (struct sockaddr *)&(from->remote), &from->length)) < 0)
	{
		perror("recvfrom: ");
		exit(-1);
	}
	return recv_len;
}

int udpServerSetup(int portNumber)
{
	struct sockaddr_in6 server;
	int socketNum = 0;
	int serverAddrLen = 0;

	socketNum = safeGetUdpSocket();

	// set up the socket
	server.sin6_family = AF_INET6;		  // internet (IPv6 or IPv4) family
	server.sin6_addr = in6addr_any;		  // use any local IP address
	server.sin6_port = htons(portNumber); // if 0 = os picks

	// bind the name (address) to a port
	if (bind(socketNum, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		perror("bind() call error");
		exit(-1);
	}

	/* Get the port number */
	serverAddrLen = sizeof(server);
	getsockname(socketNum, (struct sockaddr *)&server, (socklen_t *)&serverAddrLen);
	printf("Server using Port #: %d\n", ntohs(server.sin6_port));

	return socketNum;
}

int udpClientSetup(char *hostname, int port_num, Connection *connection)
{
	memset(&connection->remote, 0, sizeof(struct sockaddr_in6));
	connection->socket_number = 0;
	connection->length = sizeof(struct sockaddr_in6);
	connection->remote.sin6_family = AF_INET6;
	connection->remote.sin6_port = htons(port_num);

	connection->socket_number = safeGetUdpSocket();

	if (gethostbyname6(hostname, &connection->remote) == NULL)
	{
		printf("Host not found: %s\n", hostname);
		return -1;
	}
	printf("Server info - ");
	printIPv6Info(&connection->remote);
	return 0;
}

int select_call(int32_t socket_num, int32_t seconds, int32_t microseconds, int32_t set_null)
{
	// This only works on 1 socket
	fd_set fdvar;
	struct timeval aTimeOut;
	struct timeval *timeout = NULL;

	if (set_null == NOT_NULL)
	{
		aTimeOut.tv_sec = seconds;
		aTimeOut.tv_usec = microseconds;
		timeout = &aTimeOut;
	}
	// reset fd's
	FD_ZERO(&fdvar);
	FD_SET(socket_num, &fdvar);

	if (select(socket_num + 1, (fd_set *)&fdvar, (fd_set *)0, (fd_set *)0, timeout) < 0)
	{
		perror("select()");
		exit(-1);
	}
	if (FD_ISSET(socket_num, &fdvar))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

void printIPv6Infor(struct sockaddr_in6 *client)
{
	char ipString[INET6_ADDRSTRLEN];
	inet_ntop(AF_INET6, &client->sin6_addr, ipString, sizeof(ipString));
	printf("IP: %s Port: %d \n", ipString, ntohs(client->sin6_port));
}