/* rcopy - client in this situation. Adopted from Stop and Wait protocol
	program written by Dr. Smith. */

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
#include "cpe464.h"
#include "slidingWindowUtil.h"

typedef enum State STATE;

enum State
{
	DONE,
	FILENAME,
	RECV_DATA,
	FILE_OK,
	START_STATE,
	RECOVERY
};

void processFile(char *argv[]);
STATE start_state(char *argv[], Connection *server);
STATE filename(char *fname, int32_t buf_size, Window *window);
STATE recv_data(Window *window, int32_t output_file_fd);
STATE recovery(Window *window, int32_t output_file_fd);
void send_rr(int seq_num);
void send_srej(int seq_num);
void check_args(int argc, char **argv);

// Global server allows for more flexibility in monitoring windows for multiple clients
Connection server;

int main(int argc, char **argv)
{

	check_args(argc, argv);
	sendtoErr_init(atof(argv[5]), DROP_ON, FLIP_ON, DEBUG_ON, RSEED_ON);
	processFile(argv);
	return 0;
}

void processFile(char *argv[])
{
	int32_t output_file_fd = 0;
	int32_t retryCount = 0;
	Window window;
	create_window(&window, atoi(argv[3]));
	STATE state = FILENAME;

	state = FILENAME;

	while (state != DONE)
	{

		switch (state)
		{
		case START_STATE:
			state = start_state(argv, &server);
			break;

		case FILENAME:
			if (udpClientSetup(argv[6], atoi(argv[7]), &server) < 0)
			{
				state = DONE;
			}
			state = filename(argv[1], atoi(argv[4]), &window);
			retryCount++;
			if (retryCount > 9)
			{
				state = DONE;
			}
			break;

		case FILE_OK:
			retryCount = 0;

			if ((output_file_fd = open(argv[2], O_CREAT | O_TRUNC | O_WRONLY, 0600)) < 0)
			{
				perror("Error on open for output of file: %s");
				exit(-1);
			}
			else
			{
				state = RECV_DATA;
			}
			break;

		case RECV_DATA:
			state = recv_data(&window, output_file_fd);
			break;

		case RECOVERY:
			state = recovery(&window, output_file_fd);
			break;

		case DONE:
			close_window(&window);
			close(output_file_fd);
			break;
		default:
			break;
		}
	}
}

// This state became useless once the sliding window protocol was implemented and
// more logic needed to exist in the process_file function
STATE start_state(char *argv[], Connection *server)
{
	STATE returnValue = FILENAME;
	return returnValue;
}

STATE filename(char *fname, int32_t buf_size, Window *window)
{
	STATE returnValue = FILE_OK;
	uint8_t buf[MAX_LEN];
	int32_t fname_len = strlen(fname) + 1;
	int32_t recv_check = 0;
	Packet packet;
	memcpy(buf, &buf_size, SIZE_OF_BUF_SIZE);
	memcpy(buf + SIZE_OF_BUF_SIZE, &(window->size), SIZE_OF_BUF_SIZE);
	memcpy(&buf[SIZE_OF_BUF_SIZE * 2], fname, fname_len);

	send_buf(buf, fname_len + (SIZE_OF_BUF_SIZE * 2), &server, FNAME, 0);

	if (select_call(server.sk_num, SHORT_TIME, 0, NOT_NULL) == 1)
	{
		recv_check = recv_buf(&packet, server.sk_num, &server);

		if (packet.flag == FNAME_BAD)
		{
			perror(" Error: file %s not found on server: ");
			exit(-1);
		}

		if (recv_check == CRC_ERROR)
		{
			returnValue = FILENAME;
		}
		returnValue = FILE_OK;
	}
	// if we have connected to the server before, close before reconnecting
	if (returnValue == FILENAME)
	{
		close(server.sk_num);
	}
	return returnValue;
}

STATE recv_data(Window *window, int32_t output_file_fd)
{
	int32_t data_len = 0;
	Packet data_packet;
	if (select_call(server.sk_num, LONG_TIME, 0, NOT_NULL) == 0)
	{
		return DONE;
	}

	data_len = recv_buf(&data_packet, server.sk_num, &server);
	// Check CRC error
	if (data_len == CRC_ERROR)
	{
		window->current = window->lower_edge;
		send_srej(window->lower_edge);
		return RECOVERY;
	}
	// Check EOF
	if (data_packet.flag == END_OF_FILE)
	{
		send_rr(data_packet.seq_num + 1);
		return DONE;
	}
	// Check sequence number equalling lower edge to slide window
	// and write data to output file
	if (data_packet.seq_num == window->lower_edge)
	{
		send_rr(window->lower_edge);
		pull_window_blinds(window, window->lower_edge + 1);
		write(output_file_fd, data_packet.data, data_len);
	}
	else if (data_packet.seq_num > window->upper_edge)
	{
		// Sequence number higher than upper edge (shouldn't get here til EOF)
		return DONE;
	}
	else if (data_packet.seq_num < window->lower_edge)
	{
		// Packet received is lower than edge, send RR
		send_rr(window->lower_edge);
	}
	else
	{
		// Packets out of sorts. Set current to
		// lower edge, send RR, wait.
		to_window_buffer(window, &data_packet);
		window->current = window->lower_edge;
		send_rr(window->lower_edge);
		return RECOVERY;
	}
	return RECV_DATA;
}

// Similar to RECV data, only out of order of window
STATE recovery(Window *window, int32_t output_file_fd)
{
	int i;
	int32_t data_len = 0;
	Packet data_packet;
	Packet to_write;

	if (select_call(server.sk_num, 10, 0, NOT_NULL) == 0)
	{
		printf("Timeout after 10 seconds, client done.\n");
		return DONE;
	}

	data_len = recv_buf(&data_packet, server.sk_num, &server);

	if (data_len == CRC_ERROR)
	{
		return RECOVERY;
	}

	if (data_packet.flag == END_OF_FILE)
	{
		if (window->lower_edge == data_packet.seq_num)
		{
			send_rr(window->lower_edge + 1);
			return DONE;
		}
		return RECOVERY;
	}
	// Packet recovery logic
	if (in_window(window, data_packet.seq_num))
	{
		to_window_buffer(window, &data_packet);
		for (i = window->lower_edge; i <= window->upper_edge + 1; i++)
		{
			window->current = i;
			if (check_valid(window, i) == 0)
			{
				if (window->current != window->upper_edge + 1 && window->current != window->lower_edge)
				{
					// not the packet we expected
					send_srej(window->current);
				}
				break;
			}
		}

		// Packet we expected, write buffer to output file
		for (i = window->lower_edge; i < window->current; i++)
		{
			if (i <= window->upper_edge)
			{
				remove_from_buffer(window, &to_write, i);
				write(output_file_fd, to_write.data, data_len);
			}
		}
		// Reset window
		if (window->current > window->lower_edge)
		{
			pull_window_blinds(window, window->current);
		}
		// Send approprate RR
		send_rr(window->current);

		// All packets accounted for
		if ((window->current > window->upper_edge) || flush_buffer(window))
		{
			pull_window_blinds(window, window->current);
			return RECV_DATA;
		}

		return RECOVERY;
	}
	// Base case
	send_rr(window->current);
	return RECOVERY;
}

void send_rr(int seq_num)
{
	Packet rr;
	rr.seq_num = seq_num;
	rr.flag = RR;
	rr.size = HEADER_LENGTH;
	create_packet(&rr);
	send_additional_packet(&rr, &server);
	//printf("%d\n", rr.seq_num);
}

void send_srej(int seq_num)
{
	Packet srej;
	srej.seq_num = seq_num;
	srej.flag = SREJ;
	srej.size = HEADER_LENGTH;
	create_packet(&srej);
	send_additional_packet(&srej, &server);
}

void check_args(int argc, char **argv)
{
	if (argc != 8)
	{
		printf("Usage %s from-filename to-filename window-size buffer-size error-percent remote-machine remote-port\n", argv[0]);
		exit(-1);
	}

	if (strlen(argv[1]) > FILENAME_LEN)
	{
		printf("FROM filename to long, needs to be less than 1000 and is %d\n", (int)strlen(argv[1]));
		exit(-1);
	}

	if (strlen(argv[2]) > FILENAME_LEN)
	{
		printf("TO filename to long, needs to be less than 1000 and is %d\n", (int)strlen(argv[2]));
		exit(-1);
	}
	// window = argv[3]
	if (atoi(argv[4]) < MIN_LEN || atoi(argv[4]) > MAX_LEN)
	{
		printf("Buffer size needs to be between 400 and 1400 and is %d\n", atoi(argv[4]));
		exit(-1);
	}

	if (atoi(argv[5]) < 0 || atoi(argv[5]) >= 1)
	{
		printf("Error rate needs to be between 0 and less then 1 and is %d\n", atoi(argv[5]));
		exit(-1);
	}
}
