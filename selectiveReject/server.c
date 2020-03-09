/* Server stop and wait code by Hugh Smith.
	Adapted for sliding window protocol */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "networks.h"
#include "cpe464.h"
#include "slidingWindowUtil.h"

typedef enum State STATE;

enum State
{
	START,
	DONE,
	FILENAME,
	SEND_DATA,
	PACKET_CHECK,
	WAIT_ON_ACK,
	END_OF_FILE_TRANSFER
};

void process_server(int serverSocketNumber);
void process_client(int32_t serverSocketNumber, uint8_t *buf, int32_t recv_len, Connection *client,
					Packet *packet);
STATE filename(Connection *client, Packet *packet, uint8_t *buf, int32_t recv_len, int32_t *data_file,
			   int32_t *bufsize, Window *window);
STATE send_data(Connection *client, Packet *packet, int32_t data_file,
				int32_t buf_size, int32_t *seq_num, Window *window, int *eof_flag);
STATE packet_check(Connection *client, Window *window, int eof_flag);
STATE wait_on_ack(Connection *client, Window *window);
STATE eof_transfer(Connection *client, int eof_flag);
int processArgs(int argc, char **argv);

int main(int argc, char **argv)
{
	int32_t serverSocketNumber = 0;
	int portNumber = 0;

	portNumber = processArgs(argc, argv);
	sendtoErr_init(atof(argv[1]), DROP_ON, FLIP_ON, DEBUG_ON, RSEED_ON);
	/*set up the main server port */
	serverSocketNumber = udpServerSetup(portNumber);

	process_server(serverSocketNumber);

	return 0;
}

void process_server(int serverSocketNumber)
{
	pid_t pid = 0;
	int status = 0;
	uint8_t buf[MAX_LEN];
	Connection client;
	int32_t recv_len = 0;
	Packet packet;

	while (1)
	{
		if (select_call(serverSocketNumber, LONG_TIME, 0, NOT_NULL) == 1)
		{
			recv_len = recv_buf(&packet, serverSocketNumber, &client);
			if (recv_len != CRC_ERROR)
			{
				if ((pid = fork()) < 0)
				{
					perror("fork");
					exit(-1);
				}
				if (pid == 0)
				{
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

void process_client(int32_t serverSocketNumber, uint8_t *buf, int32_t recv_len, Connection *client,
					Packet *packet)
{
	STATE state = START;
	int32_t data_file = 0;
	int32_t buf_size = 0;
	int32_t seq_num = START_SEQ_NUM;
	Window window;
	int eof_flag;

	while (state != DONE)
	{

		switch (state)
		{

		case START:
			state = FILENAME;
			break;

		case FILENAME:
			state = filename(client, packet, buf, recv_len, &data_file, &buf_size, &window);
			break;

		case SEND_DATA:
			state = send_data(client, packet, data_file, buf_size, &seq_num, &window, &eof_flag);
			break;

		case PACKET_CHECK:
			state = packet_check(client, &window, eof_flag);
			break;

		case WAIT_ON_ACK:
			state = wait_on_ack(client, &window);
			break;

		case END_OF_FILE_TRANSFER:
			state = eof_transfer(client, eof_flag);
			break;

		case DONE:
			close_window(&window);
			break;

		default:
			state = DONE;
			break;
		}
	}
}

STATE filename(Connection *client, Packet *packet, uint8_t *buf, int32_t recv_len, int32_t *data_file,
			   int32_t *buf_size, Window *window)
{
	int32_t window_size;
	uint8_t response[1];
	char fname[MAX_LEN];

	memcpy(buf_size, packet->data, SIZE_OF_BUF_SIZE);
	memcpy(&(window_size), packet->data + SIZE_OF_BUF_SIZE, SIZE_OF_BUF_SIZE);
	create_window(window, window_size);
	memcpy(fname, packet->data + (2 * SIZE_OF_BUF_SIZE), recv_len - (2 * SIZE_OF_BUF_SIZE));

	/* Create client socket to allow for processing this particular client */
	client->sk_num = safeGetUdpSocket();

	if (((*data_file) = open(fname, O_RDONLY)) < 0)
	{
		send_buf(response, 0, client, FNAME_BAD, 0);
		return DONE;
	}
	else
	{
		send_buf(response, 0, client, FNAME_OK, 0);
		return SEND_DATA;
	}
}

STATE send_data(Connection *client, Packet *packet, int32_t data_file,
				int buf_size, int32_t *seq_num, Window *window, int *eof_flag)
{
	int32_t len_read = 0;

	if (window->current > window->upper_edge)
	{
		return WAIT_ON_ACK;
	}
	else
	{
		len_read = read(data_file, packet->data, buf_size);
		packet->seq_num = window->current;
		packet->size = len_read + HEADER_LENGTH;

		switch (len_read)
		{
		case -1:
			perror("send_data, read error");
			return DONE;
			break;
		case 0:
			*eof_flag = packet->seq_num;
			return WAIT_ON_ACK;
			break;
		default:
			window->current++;
			create_packet(packet);
			send_additional_packet(packet, client);
			packet->flag = DATA;
			to_window_buffer(window, packet);
			break;
		}
	}

	// Check for RRs and SREJs
	if (select_call(client->sk_num, 0, 0, NOT_NULL) == 1)
	{
		return PACKET_CHECK;
	}
	return SEND_DATA;
}

STATE packet_check(Connection *client, Window *window, int eof_flag)
{
	uint32_t crc_check = 0;
	Packet expected;
	Packet resend;
	int32_t rr, srej;

	crc_check = recv_buf(&expected, client->sk_num, client);

	if (crc_check == CRC_ERROR)
	{
		return WAIT_ON_ACK;
	}

	if (expected.flag == RR)
	{
		rr = expected.seq_num;

		if (rr > window->lower_edge)
			pull_window_blinds(window, rr);

		if (rr == eof_flag)
			return END_OF_FILE_TRANSFER;
	}
	else if (expected.flag == SREJ)
	{
		//printf("Resending packet %d\n", expected.seq_num);
		srej = expected.seq_num;
		get_from_buffer(window, &resend, srej);
		resend.flag = RESEND;
		send_additional_packet(&resend, client);
	}

	return SEND_DATA;
}

STATE wait_on_ack(Connection *client, Window *window)
{
	static int32_t retryCount = 0;
	Packet packet;

	retryCount++;
	if (retryCount > 10)
	{
		//printf("Sent data 10 times, no ACK, client session terminated.\n");
		return DONE;
	}

	if (select_call(client->sk_num, 1, 0, NOT_NULL) != 1)
	{
		get_from_buffer(window, &packet, window->lower_edge);
		packet.flag = RESEND;

		if (send_additional_packet(&packet, client) < 0)
		{
			perror("send packet, timeout_on_ack");
			exit(-1);
		}

		return WAIT_ON_ACK;
	}

	retryCount = 0;
	return PACKET_CHECK;
}

STATE eof_transfer(Connection *client, int eof_flag)
{
	static int32_t retryCount = 0;
	uint32_t crc_check = 0;
	Packet packet;
	Packet expected;
	// Send EOF packet
	packet.flag = END_OF_FILE;
	packet.size = HEADER_LENGTH;
	packet.seq_num = eof_flag;
	create_packet(&packet);
	send_additional_packet(&packet, client);
	retryCount++;
	if (retryCount > 10)
	{
		return DONE;
	}

	if (select_call(client->sk_num, 1, 0, NOT_NULL) == 1)
	{
		retryCount = 0;

		crc_check = recv_buf(&expected, client->sk_num, client);

		if (crc_check == CRC_ERROR)
		{
			return END_OF_FILE_TRANSFER;
		}

		if (expected.flag == RR && expected.seq_num == eof_flag + 1)
		{
			return DONE;
		}
	}

	return END_OF_FILE_TRANSFER;
}

int processArgs(int argc, char **argv)
{
	int portNumber = 0;

	if (argc < 2 || argc > 3)
	{
		printf("Usage %s error-rate [port-number]\n", argv[0]);
		exit(-1);
	}
	if (atoi(argv[1]) < 0 || atoi(argv[1]) >= 1)
	{
		printf("Error rate needs to be between 0 and less then 1 and is %d\n", atoi(argv[5]));
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