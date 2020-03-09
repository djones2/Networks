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
#include <sys/wait.h>

#include "gethostbyname.h"
#include "networks.h"
#include "cpe464.h"
#include "srej.h"
#include "slidingWindow.h"

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
	TIMEOUT_ON_EOF_ACK,
	LAST_PACKET,
	COMMUNICATE
};

void process_server(int serverSocketNumber);
void process_client(int32_t socketNum, uint8_t *buf, int32_t recv_len, Connection *client, Packet *packet);
STATE filename(Connection *client, uint8_t *buf, int32_t recv_len, int32_t *data_file, int32_t *buf_size, Packet *packet, Window *window);
STATE send_data(Connection *client, Packet *packet, int32_t *packet_len, int32_t data_file, int32_t buf_size, uint32_t *seq_num,
				Window *window, int *eof_marker);
STATE communicate (Connection *client, Window *window, int eof_marker);
STATE timeout_on_ack(Connection *client, uint8_t *packet, int32_t packet_len);
STATE timeout_on_eof_ack(Connection *client, uint8_t *packet, int32_t packet_len);
STATE wait_on_ack(Connection *client, Window *window);
STATE wait_on_eof_ack(Connection *client);
STATE last_packet(Connection *client, int eof_index);
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
	uint8_t buf[MAX_LEN];
	Connection client;
	//uint8_t flag = 0;
	//uint32_t seq_num = 0;
	int32_t recv_len = 0;
	Packet packet;

	// Get a new client connection, fork child, non-blocking waitpid()
	while (1)
	{
		// Block waiting for a new client
		if (select_call(serverSocketNumber, SHORT_TIME, 0, NOT_NULL) == 1)
		{
			// TODO: fix recv_buf -> recv_packet
			recv_len = recv_packet(&packet, serverSocketNumber, &client);
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
					//printf("Child fork() = child pid: %d\n", getpid());
					process_client(serverSocketNumber, buf, recv_len, &client, &packet);
					exit(0);
				}
			}
		}
		while (waitpid(-1, &status, WNOHANG) > 0)
		{
			// Non-blocking wait
		}
	}
}

void process_client(int32_t socketNum, uint8_t *buf, int32_t recv_len, Connection *client, Packet *packet)
{
	STATE state = START;
	int32_t data_file = 0;
	int32_t packet_len = 0;
	//uint8_t packet[MAX_LEN];
	int32_t buf_size = 0;
	uint32_t seq_num = FIRST_SEQ_NUM;
	int eof_marker;
	Window window;

	while (state != DONE)
	{
		switch (state)
		{
		case START:
			state = FILENAME;
			break;

		case FILENAME:
			//seq_num = 1; // works better with logic for recovery
			state = filename(client, buf, recv_len, &data_file, &buf_size, packet, &window);
			break;

		case SEND_DATA:
			state = send_data(client, packet, &packet_len, data_file, buf_size, &seq_num, &window, &eof_marker);
			break;

		case WAIT_ON_ACK:
			state = wait_on_ack(client, &window);
			break;

/* 		case TIMEOUT_ON_ACK:
			state = timeout_on_ack(client, packet, packet_len);
			break;
 
		case WAIT_ON_EOF_ACK:
			state = wait_on_eof_ack(client);
			break;

		case TIMEOUT_ON_EOF_ACK:
			state = timeout_on_eof_ack(client, packet, packet_len);
			break;
*/		
		// This is where the SREJ logic is stored to communicate between
		// server and client
		case COMMUNICATE:
			state = communicate(client, &window, eof_marker);

		case LAST_PACKET:
			state = last_packet(client, eof_marker);

		case DONE:
			break;

		default:
			printf("Error - no state found\n");
			state = DONE;
			break;
		}
	}
}

// TODO: ADD WINDOW PARAMETER
STATE filename(Connection *client, uint8_t *buf, int32_t recv_len, int32_t *data_file, int32_t *buf_size, Packet *packet, Window *window)
{
	uint8_t response[1];
	char fname[MAX_LEN];
	STATE returnValue = DONE;
	int32_t window_size;

	// Extract buffer size used for sending data and file name
	memcpy(buf_size, packet->payload, SIZE_OF_BUF_SIZE);
	memcpy(&(window_size), packet->payload + SIZE_OF_BUF_SIZE, SIZE_OF_BUF_SIZE);
	create_window(window, window_size);
	//*buf_size = ntohl(*buf_size);
	memcpy(fname, packet->payload + 8, recv_len - (2 * SIZE_OF_BUF_SIZE));
	// TODO: INIT WINDOW

	// Create client socket for processing of client
	client->socket_number = safeGetUdpSocket();

	// Checking filename and setting resulting state
	if (((*data_file) = open(fname, O_RDONLY)) < 0)
	{
		// TODO: SEND_BUF to SEND_PACKET
		send_buf(response, 0, client, FNAME_BAD, 0, buf);
		returnValue = DONE;
	}
	else
	{
		send_packet(response, 0, client, FNAME_GOOD, 0);
		returnValue = SEND_DATA;
	}
	// Return state
	return returnValue;
}

STATE send_data(Connection *client, Packet *packet, int32_t *packet_len, int32_t data_file, int32_t buf_size, uint32_t *seq_num,
				Window *window, int *eof_marker)
{
	//uint8_t buf[MAX_LEN];
	int32_t len_read = 0;
	STATE returnValue = DONE;

	if(is_closed(window)) {
		returnValue = WAIT_ON_ACK;
	} else {
		// Amount of data read from file
		len_read = read(data_file, packet->payload, buf_size);
		packet->seq_num = window->current;
		packet->packet_size = HEADER_LENGTH + len_read;

		switch (len_read)
		{
		// Read error from file
		case -1:
			perror("In send data, read error");
			returnValue = DONE;
			break;
		// Successful read to the end of the file, send EOF ack
		case 0:
			//(*packet_len) = send_buf(buf, 1, client, END_OF_FILE, *seq_num, packet);
			*eof_marker = packet->seq_num;
			returnValue = WAIT_ON_EOF_ACK;
			break;

		// More data left to send in the packet, send next and increment sequence number
		default:
			//(*packet_len) = send_buf(buf, len_read, client, DATA, *seq_num, packet);
			//(*seq_num)++;
			//returnValue = WAIT_ON_ACK;
			packet->flag = DATA;
			createPacket(packet);
			sendAdditionalPacket(packet, client);
			window->current++;
			to_window_buffer(window, packet);
			returnValue = SEND_DATA;
			break;
		}
	}

	if (select_call(client->socket_number, 0, 0, NOT_NULL) == 1) {
		returnValue = COMMUNICATE;
	}

	return returnValue;
}

// This is mainly a copy of wait_on_ack after the select call
STATE communicate (Connection *client, Window *window, int eof_marker) {
	STATE returnValue = SEND_DATA;
	uint32_t crc_check = 0;
	//uint8_t buf[MAX_LEN];
	//int32_t len = MAX_LEN;
	//uint8_t flag = 0;
	//uint32_t seq_num = 0;
	//static int retryCount = 0;
	Packet incoming;
	Packet missed;
	int32_t rr;
	int32_t srej;

	crc_check = recv_packet(&incoming, client->socket_number, client);

	// If crc error, ignore packet
	if (crc_check == CRC_ERROR)
	{
		returnValue = WAIT_ON_ACK;
	}
	// RR logic
	if (incoming.flag == RR)
	{
		rr = incoming.seq_num;
		// See if this is the last packet
		if (rr == eof_marker) {
			returnValue = LAST_PACKET;
		} 
		// Slide window if the RR # is greater than lower edge
		// set to lower edge
		if (rr > window->lower) {
			slide_window(window, rr);
		}
	}  // SREJ logic
	else if (incoming.flag == SREJ) {
		srej = incoming.seq_num;
		from_buffer(window, &missed, srej);
		missed.flag = MISSED;
		sendAdditionalPacket(&missed, client);
	}
	return returnValue;
}

STATE wait_on_ack(Connection *client, Window *window)
{
	STATE returnValue = COMMUNICATE;
/* 	- Logic split to COMMUNICATE and 
		parameters set in Packet
	uint32_t crc_check = 0;
	uint8_t buf[MAX_LEN];
	int32_t len = MAX_LEN;
	uint8_t flag = 0;
	uint32_t seq_num = 0; */
	static int retryCount = 0;
	Packet packet;

/* 	if ((returnValue = processSelect(client, &retryCount, TIMEOUT_ON_ACK, SEND_DATA, DONE)) == SEND_DATA)
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
	} */

	retryCount++;
	if (retryCount > 10) {
		returnValue = DONE;
		return returnValue;
	}
	if (select_call(client->socket_number, SHORT_TIME, 0, NOT_NULL) != 1) {
		from_buffer(window, &packet, window->lower);
		packet.flag = MISSED;
		if (sendAdditionalPacket(&packet, client) < 0) {
			perror("waited on ack err: ");
			exit(-1);
		}
		returnValue = WAIT_ON_ACK;
		return returnValue;
	}
	retryCount = 0;
	return returnValue;
}

STATE wait_on_eof_ack(Connection *client)
{
	STATE returnValue = DONE;
	uint32_t crc_check = 0;
	uint8_t buf[MAX_LEN];
	int32_t len = MAX_LEN;
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

// encapsulate the eof situation
STATE last_packet(Connection *client, int eof_marker) {
	STATE returnValue = DONE;
	static int32_t retryCount = 0;
	uint32_t crc_check = 0;
	Packet packet;
	Packet last_packet;

	packet.flag = END_OF_FILE;
	// only send the header with the eof flag
	packet.packet_size = HEADER_LENGTH;
	packet.seq_num = eof_marker;
	createPacket(&packet);
	// Send packet
	sendAdditionalPacket(&packet, client);
	
	// Handle the last packet case
	retryCount++;
	// 10 attempts at sending
	if (retryCount > 10) {
		return DONE;
	}
	
	if (select_call(client->socket_number, SHORT_TIME, 0, NOT_NULL) == 1) {
		retryCount = 0;
		crc_check = recv_packet(&last_packet, client->socket_number, client);
	
		if (crc_check == CRC_ERROR) {
			returnValue = LAST_PACKET;
		}
	
		if ((last_packet.seq_num == eof_marker + 1) && (last_packet.flag == RR)) {
			returnValue = DONE;
		}
	}
	return returnValue;
}

int process_args(int argc, char **argv)
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
	if (atoi(argv[1]) < 0 || atoi(argv[1]) >= 1)
	{
		printf("Error percent needs to be between 0 and 1 and is %d\n", atoi(argv[1]));
		exit(-1);
	}
	return portNumber;
}
