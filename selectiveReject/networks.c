/* Written by Hugh Smith, copied for Program 3 use.
	Some functions adopted for similar use case. */

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
#include <errno.h>

#include "networks.h"
#include "cpe464.h"
#include "gethostbyname.h"
#include "srej.h"

int safeGetUdpSocket()
{
	int socketNumber = 0;

	if ((socketNumber = socket(AF_INET6, SOCK_DGRAM, 0)) < 0)
	{
		perror("socket");
		exit(-1);
	}
	return socketNumber;
}

int safeSendTo(Packet *packet, Connection *to)
{
	int send_len = 0;
	if ((send_len = sendtoErr(to->sk_num, packet->full_packet, packet->size, 0,
							  (struct sockaddr *)&(to->remote), to->len)) < 0)
	{
		perror("safeSendTo: ");
		exit(-1);
	}
	return send_len;
	/* 	if ((sendingLen = sendtoErr(connection->sk_num, packet.full_packet, packet.size, 0,
		(struct sockaddr *) &(connection->remote), connection->len)) < 0) {
		perror("send_buf: ");
		exit(-1);
	} */
}

int udpServerSetup(int portNumber)
{
	struct sockaddr_in6 local;
	int socketNumber = 0; //socket descriptor
	int len = 0;

	//create the socket
	socketNumber = safeGetUdpSocket();

	//set up the socket
	local.sin6_family = AF_INET6;		 //internet family
	local.sin6_addr = in6addr_any;		 //wild card machine address
	local.sin6_port = htons(portNumber); // let system choose the port

	//bind the name (address) to a port
	if (bind(socketNumber, (struct sockaddr *)&local, sizeof(local)) < 0)
	{
		perror("udpServerSetup, bind");
		exit(-1);
	}
	len = sizeof(local);
	getsockname(socketNumber, (struct sockaddr *)&local, (socklen_t *)&len);
	printf("Server using Port #: %d\n", ntohs(local.sin6_port));

	return socketNumber;
}

int safeRecvFrom(int recv_sk_num, Packet *packet, Connection *from)
{
	int recv_len = 0;
	uint32_t remote_len = sizeof(struct sockaddr_in6);
	if ((recv_len = recvfrom(recv_sk_num, packet->full_packet, MAX_LEN + HEADER_LENGTH,
							 0, (struct sockaddr *)&(from->remote), &remote_len)) < 0)
	{
		perror("safeRecvFrom: ");
		exit(-1);
	}
	return recv_len;
}

int udpClientSetup(char *hostname, uint16_t port_num, Connection *connection)
{

	memset(&connection->remote, 0, sizeof(struct sockaddr_in6));
	connection->sk_num = 0;
	connection->len = sizeof(struct sockaddr_in6);
	connection->remote.sin6_family = AF_INET6;
	connection->remote.sin6_port = htons(port_num);

	// create the socket
	if ((connection->sk_num = socket(AF_INET6, SOCK_DGRAM, 0)) < 0)
	{
		perror("udpClientSetup: ");
		exit(-1);
	}

	if (gethostbyname6(hostname, &connection->remote) == NULL)
	{
		printf("Host not found: %s\n", hostname);
		return -1;
	}
	return 0;
}

int select_call(int32_t socket_num, int32_t seconds, int32_t microseconds, int32_t set_null)
{

	fd_set fdvar;
	struct timeval aTimeOut;
	struct timeval *timeout = NULL;

	if (set_null == NOT_NULL)
	{
		aTimeOut.tv_sec = seconds;		 // set timeout to 1 second
		aTimeOut.tv_usec = microseconds; // set timeout (in micro-second)
		timeout = &aTimeOut;
	}

	FD_ZERO(&fdvar); //reset variables
	FD_SET(socket_num, &fdvar);

	if (select(socket_num + 1, (fd_set *)&fdvar, (fd_set *)0, (fd_set *)0, timeout) < 0)
	{
		perror("select_call: ");
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
