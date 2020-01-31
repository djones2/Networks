#ifndef CCLIENT_H_
#define CCLIENT_H_

#include <stdint.h>

#define MAXBUF 1024
#define DEBUG_FLAG 1
#define xstr(a) str(a)
#define str(a) #a

#define INITIAL_PCKT 1
#define GOOD_HANDLE 2
#define ERR_INITIAL_PCKT 3
#define BROADAST_PCKT 4
#define MESSAGE_PCKT 5
#define ERR_MESSAGE_PCKT 7
#define EXIT_REQUEST 8
#define EXIT_REPLY 9
#define HANDLE_LIST_REQUEST 10
#define HANDLE_LIST_LEN 11
#define HANDLE_INFO 12
#define END_OF_LIST 13
#define MAX_HANDLE_LEN 100
#define PAYLOAD_LEN 200

typedef struct chat_header {
    uint16_t MSG_PCKT_LEN;
    uint8_t FLAG;
} chat_header;

typedef struct destination_client {
    uint8_t handle_len;
    char handle[MAX_HANDLE_LEN];
} destination_client;

typedef struct message_packet {
    chat_header ch;
    u_int8_t sender_handle_len;
    char sender[MAX_HANDLE_LEN];
    u_int8_t dest_handle_num;
    destination_client dest_clients[9];
    char payload[PAYLOAD_LEN];
} message_packet;

typedef struct client {
    int socket_number;
    char handle[MAX_HANDLE_LEN];
} client;

void sendToServer(int socketNum);
void checkArgs(int argc, char * argv[]);

#endif