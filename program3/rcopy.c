// Client side - UDP Code				    
// By Hugh Smith	4/1/2017		

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

#include "gethostbyname.h"
#include "networks.h"
#include "cpe464.h"
#include "srej.h"
#include "slidingWindow.h"


typedef enum State STATE;

enum State {
	DONE, FILENAME, RECV_DATA, FILE_OK, START_STATE
};

void processFile(char * argv[]);
STATE start_state(char ** argv, Connection * server, uint32_t * clientSeqNum);
STATE filename(char * fname, int32_t buf_size, Connection * server);
STATE recv_data(int32_t output_file, Connection *server, uint32_t * clientSeqNum);
STATE file_ok(int *outputFileFD, char * outputFileName);
void checkArgs(int argc, char ** argv);

int main (int argc, char *argv[])
 {
	checkArgs(argc, argv);
	
	sendtoErr_init(atof(argv[4]), DROP_ON, FLIP_ON, DEBUG_ON, RSEED_ON);

	processFile(argv);

	return 0;
}

void processFile(char * argv[]) {
	Connection * server;
	uint32_t clientSeqNum = 0;
	int32_t output_file_fd = 0;
	STATE state = START_STATE;

	while (state != DONE)
	{
		switch (state)
		{
		case START_STATE:
			state = start_state(argv, &server, &clientSeqNum);
			break;

		case FILENAME:
			state = filename(argv[1], atoi(argv[3]), &server);
			break;

		case FILE_OK:
			state = file_ok(&output_file_fd, argv[2]);
			break;

		case RECV_DATA:
			state = recv_data(output_file_fd, &server, &clientSeqNum);
			break;

		case DONE:
			break;

		default:
			printf("Error - no state found\n");
			break;
		}
	}
}

STATE start_state(char ** argv, Connection * server, uint32_t * clientSeqNum) {
	uint8_t packet[MAX_PACKET_SIZE];
	uint8_t buf[MAX_PACKET_SIZE];
	int fileNameLen = strlen(argv[1]);
	STATE returnValue = FILENAME;
	uint32_t bufferSize = 0;

	if (server->socket_number >0) {
		close(server->socket_number);
	}
	if (udpClientSetup(argv[5], atoi(argv[6]), server) < 0) {
		returnValue = DONE;
	} else {
		bufferSize = htonl(atoi(argv[3]));
		memcpy(buf, &bufferSize, SIZE_OF_BUF_SIZE);
		memcpy(&buf[SIZE_OF_BUF_SIZE], argv[1], fileNameLen);
		printIPv6Info(&server->remote);
		send_buf(buf, fileNameLen + SIZE_OF_BUF_SIZE, server, FNAME, *clientSeqNum, packet);
		(*clientSeqNum)++;
		returnValue = FILENAME;
	}
	return returnValue;
}

STATE filename(char * fname, int32_t buf_size, Connection * server) {
	int returnValue = START_STATE;
	uint8_t packet[MAX_PACKET_SIZE];
	uint8_t flag = 0;
	uint32_t seq_num = 0;
	int32_t recv_check = 0;
	static int retryCount = 0;

	if ((returnValue = processSelect(server, &retryCount, START_STATE, FILE_OK, DONE)) == FILE_OK)
	{
		recv_check = recv_buf(packet, MAX_PACKET_SIZE, server->socket_number, server, &flag, &seq_num);

		if (recv_check == CRC_ERROR)
		{
			returnValue = START_STATE;
		} 
		else if (flag == FNAME_BAD)
		{
			printf("File %s not found.\n", fname);
			returnValue = DONE;
		} 
		else if (flag == DATA)
		{
			returnValue = FILE_OK;
		}
	}
	return returnValue;
}

STATE file_ok(int *outputFileFD, char * outputFileName) {
	STATE returnValue = DONE;

	if ((*outputFileFD = open(outputFileName, O_CREAT | O_TRUNC | O_WRONLY, 0600)) < 0) {
		perror("File open error: ");
		returnValue = DONE;
	} else {
		returnValue = RECV_DATA;
	}
	return returnValue;
}

STATE recv_data(int32_t output_file, Connection *server, uint32_t * clientSeqNum) {
	uint32_t seq_num = 0;
	uint32_t ackSeqNum = 0;
	uint8_t flag = 0;
	int32_t data_len = 0;
	uint8_t data_buf[MAX_PACKET_SIZE];
	uint8_t packet[MAX_PACKET_SIZE];
	static int32_t expected_seq_num = FIRST_SEQ_NUM;

	if (select_call(server->socket_number, LONG_TIME, 0, NOT_NULL) == 0) {
		printf("Timeoout after 10 seconds, assuming server disconnected.\n");
		return DONE;
	}
	data_len = recv_buf(data_buf, MAX_PACKET_SIZE, server->socket_number, server, &flag, &seq_num);

	if (data_len == CRC_ERROR) {
		return RECV_DATA;
	} 
	if (flag == END_OF_FILE) {
		send_buf(packet, 1, server, EOF_ACK, *clientSeqNum, packet);
		(*clientSeqNum)++;
		printf("File done\n");
		return DONE;
	} else {
		ackSeqNum = htonl(seq_num);
		send_buf((uint *)&ackSeqNum, sizeof(ackSeqNum), server, ACK, *clientSeqNum, packet);
		(*clientSeqNum)++;
	}
	if (seq_num == expected_seq_num) {
		expected_seq_num++;
		write(output_file, &data_buf, data_len);
	}
	return RECV_DATA;
}

void checkArgs(int argc, char * argv[])
{
	/* check command line arguments  */
	if (argc != 8)
	{
		printf("Usage: %s from-filename to-filename window-size buffer-size error-percent remote-machine remote-port\n", argv[0]);
		exit(-1);
	}
	if (strlen(argv[1]) > 1000)
	{
		printf("From-filename too long - needs to be less than 1000 and is $d\n", argv[1]);
		exit(-1);
	}
	if (strlen(argv[2]) > 1000)
	{
		printf("To-filename too long - needs to be less than 1000 and is $d\n", argv[2]);
		exit(-1);
	}
	// insert window size check here
	if (atoi(argv[3]) < 400 || atoi(argv[3]) > 1400)
	{
		printf("Buffer needs to be between 400 and 1400 and is $d\n", argv[3]);
		exit(-1);
	}

	if (atoi(argv[4]) < 0 || atoi(argv[4]) >= 1)
	{
		printf("Error percent needs to be between 0 and 1 and is $d\n", argv[4]);
		exit(-1);
	}

    printf("from file: %s\n", argv[1]);
    printf("to file: %s\n", argv[2]);
    printf("window size: %d\n", atoi(argv[3]));
    printf("buffer size: %d\n", atoi(argv[4]));
    printf("error percent: %s\n", argv[5]);
    printf("remote machine: %d\n", atoi(argv[6]));
    printf("remote port: %d\n", atoi(argv[7]));		
}





