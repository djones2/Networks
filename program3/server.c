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
#include <sys/stat.h>
#include <fcntl.h>

#include "gethostbyname.h"
#include "networks.h"
#include "cpe464.h"
#include "srej.h"
#include "slidingWindow.h"

#define MAXBUF 80

typedef enum State STATE;

enum State
{
	START,
	DONE,
	FILENAME,
	SEND_DATA,
	WAIT_ON_ACK,
	TIMEOUT_ON_ACK,
	WAIT_ON_EOF_ACK,
	TIMEOUT_ON_EOF_ACK
};

void process_server(int serverSocketNumber);
void process_client(int32_t socketNum, uint8_t *buf, int32_t recv_len, Connection *client);
STATE filename(Connection *client, uint8_t *buf, int32_t recv_len, int32_t *data_file, int32_t *buf_size);
STATE send_data(Connection *client, uint8_t *packet, int32_t *packet_len, int32_t data_file, int32_t *buf_size, uint32_t *seq_num);
STATE timeout_on_ack(Connection *client, uint8_t *packet, int32_t packet_len);
STATE timeout_on_eof_ack(Connection *client, uint8_t *packet, int32_t packet_len);
STATE wait_on_ack(Connection *client);
STATE wait_on_eof_ack(Connection *client);
int process_args(int argc, char **argv);

int main(int argc, char *argv[])
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

void process_server(int serverSocketNumber)
{
	pid_t pid = 0;
	int status = 0;
	uint8_t buf[MAX_PACKET_SIZE];
	Connection client;
	uint8_t flag = 0;
	uint32_t seq_num = 0;
	int32_t recv_len = 0;

	// Get a new client connection, fork child, non-blocking waitpid()
	while (1)
	{
		// Block waiting for a new client
		if (select_call(serverSocketNumber, LONG_TIME, 0, NOT_NULL) == 1)
		{
			recv_len = recv_buf(buf, MAX_PACKET_SIZE, serverSocketNumber, &client, &flag, &seq_num);
			if (recv_len != CRC_ERROR)
			{
				if ((pid = fork()) < 0)
				{
					perror("fork");
					exit(-1);
				}
				if (pid == 0)
				{
					// Child process - new process for each client
					printf("Child fork() = child pid: %d\n", getpid());
					process_client(serverSocketNumber, buf, recv_len, &client);
					exit(0);
				}
			}
		}
		while (waitpid(-1, &status, WNOHANG) > 0)
			;
		{
			// Non-blocking wait
		}
	}
}

void process_client(int32_t socketNum, uint8_t *buf, int32_t recv_len, Connection *client)
{
	STATE state = START;
	int32_t data_file = 0;
	int32_t packet_len = 0;
	uint8_t packet[MAX_PACKET_SIZE];
	int32_t buf_size = 0;
	uint32_t seq_num = FIRST_SEQ_NUM;

	while (state != DONE)
	{
		switch (state)
		{
		case START:
			state = FILENAME;
			break;

		case FILENAME:
			state = filename(client, buf, recv_len, &data_file, &buf_size);
			break;

		case SEND_DATA:
			state = send_data(client, packet, &packet_len, packet_len, buf_size, &seq_num);
			break;

		case WAIT_ON_ACK:
			state = wait_on_ack(client);
			break;

		case TIMEOUT_ON_ACK:
			state = timeout_on_ack(client, packet, packet_len);
			break;

		case WAIT_ON_EOF_ACK:
			state = wait_on_eof_ack(client);
			break;

		case TIMEOUT_ON_EOF_ACK:
			state = timeout_on_eof_ack(client, packet, packet_len);
			break;

		case DONE:
			break;

		default:
			printf("Error - no state found\n");
			state = DONE;
			break;
		}
	}
}

STATE filename(Connection *client, uint8_t *buf, int32_t recv_len, int32_t *data_file, int32_t *buf_size)
{
	uint8_t response[1];
	char fname[MAX_PACKET_SIZE];
	STATE returnValue = DONE;

	// Extract buffer size used for sending data and file name
	memcpy(buf_size, buf, SIZE_OF_BUF_SIZE);
	*buf_size = ntohl(*buf_size);
	memcpy(fname, &buf[sizeof(*buf_size)], recv_len - SIZE_OF_BUF_SIZE);

	// Create client socket for processing of client
	client->socket_number = safeGetUdpSocket();

	// Checking filename and setting resulting state
	if (((*data_file) = open(fname, O_RDONLY)) < 0)
	{
		send_buf(response, 0, client, FNAME_BAD, 0, buf);
		returnValue = DONE;
	}
	else
	{
		send_buf(response, 0, client, FNAME_GOOD, 0, buf);
		returnValue = SEND_DATA;
	}
	// Return state
	return returnValue;
}

STATE send_data(Connection *client, uint8_t *packet, int32_t *packet_len, int32_t data_file, int32_t *buf_size, uint32_t *seq_num)
{
	uint8_t buf[MAX_PACKET_SIZE];
	int32_t len_read = 0;
	STATE returnValue = DONE;

	// Amount of data read from file
	len_read = read(data_file, buf, buf_size);

	switch (len_read)
	{
	// Read error from file
	case -1:
		perror("In send data, read error");
		returnValue = DONE;
		break;
	// Successful read to the end of the file, send EOF ack
	case 0:
		(*packet_len) = send_buf(buf, 1, client, END_OF_FILE, *seq_num, packet);
		returnValue = WAIT_ON_EOF_ACK;
		break;

	// More data left to send in the packet, send next and increment sequence number
	default:
		(*packet_len) = send_buf(buf, len_read, client, DATA, *seq_num, packet);
		(*seq_num)++;
		returnValue = WAIT_ON_ACK;
		break;
	}
	return returnValue;
}

STATE wait_on_ack(Connection *client)
{
	STATE returnValue = DONE;
	uint32_t crc_check = 0;
	uint8_t buf[MAX_PACKET_SIZE];
	int32_t len = MAX_PACKET_SIZE;
	uint8_t flag = 0;
	uint32_t seq_num = 0;
	static int retryCount = 0;

	if ((returnValue = processSelect(client, &retryCount, TIMEOUT_ON_ACK, SEND_DATA, DONE)) == SEND_DATA)
	{
		crc_check = recv_buf(buf, len, client->socket_number, client, &flag, &seq_num);

		// If crc error, ignore packet
		if (crc_check == CRC_ERROR)
		{
			returnValue = WAIT_ON_ACK;
		}
		else if (flag != ACK)
		{
			printf("In wait_on_ack() but not an ACK flag received: %d\n", flag);
			returnValue = DONE;
		}
	}
	return returnValue;
}

STATE wait_on_eof_ack(Connection *client)
{
	STATE returnValue = DONE;
	uint32_t crc_check = 0;
	uint8_t buf[MAX_PACKET_SIZE];
	int32_t len = MAX_PACKET_SIZE;
	uint8_t flag = 0;
	uint32_t seq_num = 0;
	static int retryCount = 0;

	if ((returnValue = processSelect(client, &retryCount, TIMEOUT_ON_EOF_ACK, DONE, DONE)) == DONE)
	{
		crc_check = recv_buf(buf, len, client->socket_number, client, &flag, &seq_num);

		// If crc error, ignore packet
		if (crc_check == CRC_ERROR)
		{
			returnValue = WAIT_ON_EOF_ACK;
		}
		else if (flag != EOF_ACK)
		{
			printf("In wait_on_ack() but not an ACK flag received: %d\n", flag);
			returnValue = DONE;
		}
		else
		{
			printf("File transferred successfully.\n");
			returnValue = DONE;
		}
	}
	return returnValue;
}

STATE timeout_on_ack(Connection *client, uint8_t *packet, int32_t packet_len)
{
	safeSendto(packet, packet_len, client);
	return WAIT_ON_ACK;
}

STATE timeout_on_eof_ack(Connection *client, uint8_t *packet, int32_t packet_len)
{
	safeSendto(packet, packet_len, client);
	return WAIT_ON_EOF_ACK;
}

int checkArgs(int argc, char **argv)
{
	// Checks args and returns port number
	int portNumber = 0;

	if (argc > 3 || argc < 2)
	{
		printf("Usage %s error-rate [port-number]\n", argv[0]);
		exit(-1);
	}

	if (argc == 3)
	{
		portNumber = atoi(argv[2]);
	}
	else
	{
		portNumber = 0;
	}
	return portNumber;
}
