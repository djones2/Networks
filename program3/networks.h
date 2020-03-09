/* Written by: Dr. Hugh Smith
	Adapted for Sliding Window Protocol 
*/

#ifndef _NETWORKS_H_
#define _NETWORKS_H_

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

#include "cpe464.h"
#include "gethostbyname.h"
#include "srej.h"

#define BACKLOG 10

enum SELECT
{
	SET_NULL,
	NOT_NULL
};

typedef struct connection Connection;

struct connection
{
	int32_t sk_num;
	struct sockaddr_in6 remote;
	uint32_t len;
};

int safeGetUdpSocket();
int safeSendTo(Packet *packet, Connection *to);
int safeRecvFrom(int recv_sk_num, Packet *packet, Connection *from);
int udpServerSetup(int portNumber);
int udpClientSetup(char *hostname, uint16_t port_num, Connection *connection);
int select_call(int32_t socket_num, int32_t seconds, int32_t microseconds, int32_t set_null);
// I changed processSelect() to implemented with my main work loop in order to monitor
// both retryCount and the window size without having to deal with the pointers involved.
// Also got rid of IP print statement.

#endif
