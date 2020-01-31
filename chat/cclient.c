/******************************************************************************
* myClient.c
*
*****************************************************************************/

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
#include <math.h>
#include <ctype.h>

#include "networks.h"
#include "cclient.h"

void sendToServer(int socketNum)
{
	char sendBuf[MAXBUF]; //data buffer
	int sendLen = 0;	  //amount of data to send
	int sent = 0;		  //actual amount of data sent/* get the data and send it   */

	printf("Enter the data to send: ");
	scanf("%" xstr(MAXBUF) "[^\n]%*[^\n]", sendBuf);

	sendLen = strlen(sendBuf) + 1;
	printf("read: %s len: %d\n", sendBuf, sendLen);

	sent = send(socketNum, sendBuf, sendLen, 0);
	if (sent < 0)
	{
		perror("send call");
		exit(-1);
	}

	printf("String sent: %s \n", sendBuf);
	printf("Amount of data sent is: %d\n", sent);
}

void checkArgs(int argc, char *argv[])
{
	/* Check command line arguments  */
	if (argc != 4)
	{
		printf("usage: %s handle-name server-name port-number \n", argv[0]);
		exit(1);
	}
	/* Check handle length */
	else if (strlen(argv[1]) > MAX_HANDLE_LEN)
	{
		fprintf(stderr, "Invalid handle, longer than 100 characters: %s\n", argv[1]);
		exit(1);
	}
	/* Check handle first letter */
	else if (isdigit(argv[1][0]))
	{
		fprintf(stderr, "Invalid handle, name cannot start with number: %s\n", argv[1]);
		exit(1);
	}
}

/* Create client */
client initClient(int socket, char *handle) {
	client cclient;
	cclient.socket_number = socket;
	strncpy(cclient.handle, handle, strlen(handle));
	return cclient;
}

int main(int argc, char *argv[])
{
	int socketNum = 0; //socket descriptor
	/* Client being used */
	client cclient;
	/* Check CLA's */
	checkArgs(argc, argv);
	/* set up the TCP Client socket  */
	socketNum = tcpClientSetup(argv[2], argv[3], DEBUG_FLAG);
	cclient = initClient(socketNum, argv[1]);

	sendToServer(socketNum);

	close(socketNum);

	return 0;
}
