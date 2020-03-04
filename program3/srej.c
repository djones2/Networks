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

#include "slidingWindow.h"
#include "srej.h"
#include "cpe464.h"
#include "networks.h"

int retrieveHeader(uint8_t * data_buf, int recv_len, uint8_t * flag, uint32_t * sequence_number) {
    
}

int32_t recv_buf(uint8_t * buf, int32_t len, int32_t recv_socket_number, Connection * connection,
                    uint8_t * flag, uint32_t * sequence_number) 
{
    uint8_t data_buf[MAX_PACKET_SIZE];
    int32_t recv_len = 0;
    int32_t data_len = 0;

    recv_len = safeRecvfrom(recv_socket_number, data_buf, len, connection);

    data_len = retrieveHeader(data_buf, recv_len, flag, sequence_number);
    // FINISH and go back to server
}