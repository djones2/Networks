/* Server side - UDP Code				    */
/* By Hugh Smith	4/1/2017	*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "gethostbyname.h"
#include "networks.h"
#include "srej.h"
#include "cpe464.h"

#define MAXBUF 80

typedef enum State STATE;

enum STATE {
	START, DONE, FILENAME_STATE, SEND_DATA, WAIT_ON_ACK, TIMEOUT_ON_ACK,
	WAIT_ON_EOF_ACK, TIMEOUT_ON_EOF_ACK
};

void process_server(int serverSocketNumber);
void process_client(int32_t socketNum, uint8_t * buf, int32_t recv_len, Connection * client);
State filename(Connection * client, uint8_t * buf, int32_t recv_len, int32_t * data_file, int32_t * buf_size);
State send_data(Connection * client, uint8_t * packet, int32_t * packet_len, int32_t data_file, int32_t * buf_size, uint32_t seq_num);
State timeout_on_ack(Connection * client, uint8_t * packet, int32_t * packet_len);
State timeout_on_eof_ack(Connection * client, uint8_t * packet, int32_t * packet_len);
State wait_on_ack(Connection * client);
State wait_on_eof_ack(Connection * client);
int process_args(int argc, char ** argv);


int main (int argc, char *argv[])
{ 
	int32_t serverSocketNumber = 0;				
	int portNumber = 0;

	portNumber = process_args(argc, argv);

	// Init packet sending, dropping packet, etc.
	sendtoErr_init(atof(argv[1]), DROP_ON, FLIP_ON, DEBUG_ON, RSEED_ON);
	
	// Set up main server port
	serverSocketNumber = udpServerSetup(portNumber);

	// Main work loop
	process_server(serverSocketNumber);
	
	return 0;
}

void process_server(int serverSocketNumber) {
	pid_t pid = 0;
	int status = 0; 
	uint8_t buf[MAX_PACKET_SIZE];
}

void processClient(int socketNum)
{
	int dataLen = 0; 
	char buffer[MAXBUF + 1];	  
	struct sockaddr_in6 client;		
	int clientAddrLen = sizeof(client);	
	
	buffer[0] = '\0';
	while (buffer[0] != '.')
	{
		dataLen = safeRecvfrom(socketNum, buffer, MAXBUF, 0, (struct sockaddr *) &client, &clientAddrLen);
	
		printf("Received message from client with ");
		printIPInfo(&client);
		printf(" Len: %d %s\n", dataLen, buffer);

		// just for fun send back to client number of bytes received
		sprintf(buffer, "bytes: %d", dataLen);
		safeSendto(socketNum, buffer, strlen(buffer)+1, 0, (struct sockaddr *) & client, clientAddrLen);

	}
}

void processServer(int serverSocketNum) {
	pid_t pid = 0;
	int status = 0;
	uint8_t buff[MAX_PACKET_SIZE];
	Connection client;
	uint8_t flag = 0;
	uint32_t sequence_num = 0;
	int32_t recv_len = 0;

	// Get new client connection, fork() a child with parents in a non-blocking
	// waitpid()
	while(1) {
		// Non-blocking wait
		if(select_call(serverSocketNum, LONG_TIME, 0, NOT_NULL) == 1){
			recv_len = recv_buf(buf, MAX_LEN, serverSocketNum, &client, &flag, &sequence_num);


		}
	}
}

int checkArgs(int argc, char ** argv)
{
	// Checks args and returns port number
	int portNumber = 0;

	if (argc > 3 || argc < 2)
	{
		fprintf(stderr, "usage ./server error-percent [port-number]\n");
		exit(-1);
	}
	
	if (argc == 3)
	{
		portNumber = atoi(argv[2]);
	} else {
		portNumber = 0;
	}
	return portNumber;
}


