#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <cerrno>

#include "message.h"
#include "cclient.h"

using namespace std;

/* Create client instance */
Cclient::Cclient(char *handle, struct hostent *server, int port)
{
    this->handle = handle;
    this->server = server;
    this->port = port;
}

/* Initialize client fields */
void Cclient::init()
{
    /* Create client fields */
    Message *messagePacket;
    int payloadLen;
    int sendb;
    exitb = false;
    sequence_number = 0;

    /* Set up server information */
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons((short)port);

    /* Cooy address information */
    memcpy(&(server_address.sin_addr.s_addr), server->h_addr, server->h_length);

    /* Obtain socket file descriptor */
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == GEN_ERROR)
        throw CLIENT_NAME_ERROR;

    /* Create connection */
    if (connect(socket_fd, (struct sockaddr *)(&server_address), 16) == GEN_ERROR)
        throw SOCKET_FAIL;

    /* Populate message fields */
    messagePacket = new Message();
    messagePacket->set_flag(INITIAL_PCKT);
    messagePacket->set_from(handle, strlen(handle));
    messagePacket->set_sequence_number(sequence_number++);
    /* Package the message into a packet */
    payloadLen = messagePacket->packet();

    /* Send call */
    if ((sendb = send(socket_fd, messagePacket->sendable(), payloadLen, 0)) == GEN_ERROR)
        throw SEND_FAIL;

    /* Delete temporary message data structure */
    delete messagePacket;
    /* Check handle for availability */
    handleCheck();
}

/* Create connection to message chat */
void Cclient::createMessage()
{
    /* Create values for catching user input to message structure */
    struct timeval wait_time;
    fd_set read_set;
    printf("$: ");
    fflush(stdout);

    /* This is the loop to obtain information */
    while (!exitb)
    {
        FD_ZERO(&read_set);
        /* Setting last parameter for select call */
        wait_time.tv_sec = 1;
        wait_time.tv_usec = 0;

        /* Setting socket file descriptor */
        FD_SET(socket_fd, &read_set);
        FD_SET(0, &read_set);

        /* Select call */
        if (select(socket_fd + 1, &read_set, NULL, NULL, &wait_time) == GEN_ERROR) {
            throw SELECT_EX;
        } 
        if (FD_ISSET(socket_fd, &read_set)) {
            messageCheck();
        }
    }
}

/* Exit call */
void Cclient::exit()
{
    Message *messagePacket;
    uint8_t *buffer;
    int payloadLen;
    int error;
    int flag;

    /* Exit request */
    messagePacket = new Message();
    messagePacket->set_flag(EXIT_REQUEST);
    messagePacket->set_from(handle, strlen(handle));
    payloadLen = messagePacket->packet();

    /* Get ready to send() */
    messagePacket->set_sequence_number(sequence_number++);

    if ((error = send(socket_fd, messagePacket->sendable(), payloadLen, 0)) == GEN_ERROR)
        throw SEND_FAIL;

    delete messagePacket;

    /* Wait for ACK from server */
    buffer = (uint8_t *)calloc(MAXBUF, sizeof(uint8_t));

    /* Obtain message size. If 0, connection closed */
    if ((payloadLen = recv(socket_fd, buffer, MAXBUF, 0)) == 0)
        throw SOCKET_FAIL;

    /* Create new message to get the exit flag */
    messagePacket = new Message(buffer, payloadLen);
    flag = messagePacket->get_flag();
    free(buffer);
    delete messagePacket;

    // Check flag for ACK
    if (flag == EXIT_REPLY)
    {
        printf("Connection to server closed.\n");
    }
    else {
        throw EXIT_EX;
    }

    close(socket_fd);
}

/* Check client handle for validity */
void Cclient::handleCheck()
{
    /* Populate fields */
    uint8_t *buffer;
    int message_size, flag;
    Message *messagePacket;

    buffer = (uint8_t *)calloc(MAXBUF, sizeof(uint8_t));

    /* recv call */
    if ((message_size = recv(socket_fd, buffer, MAXBUF, 0)) == 0)
        throw SOCKET_FAIL;

    messagePacket = new Message(buffer, message_size);
    flag = messagePacket->get_flag();
    free(buffer);
    delete messagePacket;

    /* Check flags for response from server */
    if (flag == GOOD_HANDLE){
        return;
    }
    else if (flag == ERR_INITIAL_PCKT) {
        throw HANDLE_EX;
    }
    else {
        throw GEN_ERROR;
    }
}

/* Parse client input on command line */
void Cclient::inputCheck()
{
    char input_buffer[MAXBUF];
    char sel;
    int self_message = 0;

    /* Get input from command line */
    fgets(input_buffer, MAXBUF, stdin);
    try
    {
        if (strlen(input_buffer) > 1)
        {
            /* Allow for missing % */
            if (input_buffer[0] == '%') {
                sel = input_buffer[1];
            } else {
                sel = input_buffer[0];
            }
            if (sel == 'm' || sel == 'M')
            {
                self_message = sendMessage(input_buffer);
            }
            else if (sel == 'b' || sel == 'B')
            {
                broadcastMessage(input_buffer);
            }
            else if (sel == 'l' || sel == 'L')
            {
                listCommand();
            }
            else if (sel == 'e' || sel == 'E')
            {
                exitb = true;
            }
            else
            {
                /* Print if command not valid */
                printf("Invalid command format\n");
            }
        }
        else {
            printf("%%<option> <handle> <server name> <message>\n");
        }
    }
    catch (int ex)
    {
        if (ex == MAX_PAYLOAD)
            printf("Messages must be under 200 characters.\n");
        else
            throw ex;
    }

    if (!self_message)
    {
        printf("$: ");
        fflush(stdout);
    }
}

/* Check the payload being sent */
void Cclient::messageCheck()
{
    uint8_t *buffer;
    int message_size, flag;
    char handle[MAX_HANDLE_LENGTH];
    Message *messagePacket;

    buffer = (uint8_t *)calloc(MAXBUF, sizeof(uint8_t));

    /* Check for a closed connection */
    if ((message_size = recv(socket_fd, buffer, MAXBUF, 0)) == 0)
        throw SOCKET_FAIL;

    messagePacket = new Message(buffer, message_size);
    flag = messagePacket->get_flag();
    printf("\n");

    if (flag == ERR_MESSAGE_PCKT)
    {
        memcpy(handle, messagePacket->get_to(), messagePacket->get_to_length());
        handle[messagePacket->get_to_length()] = '\0';
        printf("Client with handle %s does not exist.\n", handle);
    }

    delete messagePacket;
    free(buffer);
    printf("$: ");
    fflush(stdout);
}

/* Prepare client's message for sending */
int Cclient::sendMessage(char *input)
{
    Message *messagePacket;
    int error, self_message;
    char temp[MAX_HANDLE_LENGTH];
    char *message_start;
    int message_size;
    char destination[MAX_HANDLE_LENGTH];
    int destinationLength;

    /* Get input info about message start and end points */
    sscanf(input, "%s %100s", temp, destination);
    destinationLength = strlen(destination);

    message_start = strstr(input, destination) + strlen(destination) + 1;

    self_message = (strcmp(destination, handle) == 0);

    if (strlen(message_start) > MAX_PAYLOAD)
        throw MESSAGE_SIZE_EX;

    /* Find end of message to size message */
    for (message_size = 0; message_start[message_size] != '\0'; message_size++);
    message_size++;

    /* Populate message fields */
    messagePacket = new Message();
    messagePacket->set_to(destination, destinationLength);
    messagePacket->set_from(handle, strlen(handle));

    if (message_size == 0) {
        /* Send empty message */
        messagePacket->set_text("\n", 1);
    } else {
        messagePacket->set_text(message_start, message_size);
    }

    messagePacket->set_flag(MESSAGE_PCKT);
    messagePacket->set_sequence_number(sequence_number++);
    message_size = messagePacket->packet();

    /* Error check */
    if ((error = send(socket_fd, messagePacket->sendable(), message_size, 0)) == GEN_ERROR)
        throw SEND_FAIL;

    return self_message;
}

void Cclient::broadcastMessage(char *input)
{
    int message_size;
    Message *messagePacket;
    int error;
    char temp[MAX_HANDLE_LENGTH];
    char *message_start;

    /* Retrieve input */
    sscanf(input, "%s", temp);
    message_start = input + strlen(temp) + 1;

    if (strlen(message_start) > MAX_PAYLOAD)
        throw MESSAGE_SIZE_EX;

    /* Find end of message to size message */
    for (message_size = 0; message_start[message_size] != '\0'; message_size++);
    message_size++;

    /* Populate message fields */
    messagePacket = new Message();
    messagePacket->set_from(handle, strlen(handle));
    messagePacket->set_text(message_start, message_size);

    if (message_size == 0) {
        messagePacket->set_text("\n", 1);
    } else {
        messagePacket->set_text(message_start, message_size);
    }

    messagePacket->set_flag(MESSAGE_PCKT);
    messagePacket->set_sequence_number(sequence_number++);
    message_size = messagePacket->packet();

    if ((error = send(socket_fd, messagePacket->sendable(), message_size, 0)) == GEN_ERROR)
        throw SEND_FAIL;
}

/* Initiate list command */
void Cclient::listCommand()
{
    int list_length = listLengthRequest();
    int i;
    for (i = 0; i < list_length; i++) {
        handleRequest(i);
    }
}

/* Request to get the list length */
int Cclient::listLengthRequest()
{
    Message *messagePacket;
    uint8_t buffer[MAXBUF];
    int payloadLen;
    int error;
    int flag;
    int length;

    /* Request length from server */
    messagePacket = new Message();
    messagePacket->set_flag(HANDLE_LIST_REQUEST);
    messagePacket->set_from(handle, strlen(handle));
    messagePacket->set_sequence_number(sequence_number++);
    payloadLen = messagePacket->packet();

    if ((error = send(socket_fd, messagePacket->sendable(), payloadLen, 0)) == GEN_ERROR)
        throw SEND_FAIL;

    delete messagePacket;

    /* Recv call for list length */
    if ((payloadLen = recv(socket_fd, buffer, MAXBUF, 0)) == 0) {
        throw SOCKET_FAIL;
    }

    messagePacket = new Message(buffer, payloadLen);
    flag = messagePacket->get_flag();
    length = ntohl(((uint32_t *)messagePacket->get_text())[0]);
    delete messagePacket;

    if (flag != HANDLE_LIST_LEN) {
        throw LIST_EX;
    }

    return length;
}

/* List of handles request */
void Cclient::handleRequest(int index)
{
    Message *messagePacket;
    uint8_t buffer[MAXBUF];
    char *incoming_handle;
    uint8_t handle_length;
    int error;
    int flag;
    int payloadLen; 

    /* Send request for handle */
    messagePacket = new Message();
    messagePacket->set_flag(HANDLE_LIST_REQUEST);
    messagePacket->set_from(handle, strlen(handle));
    messagePacket->set_int(index);
    messagePacket->set_sequence_number(sequence_number++);
    payloadLen = messagePacket->packet();

    /* Send call */
    if ((error = send(socket_fd, messagePacket->sendable(), payloadLen, 0)) == GEN_ERROR) {
        throw SEND_FAIL;
    }

    delete messagePacket;

    /* Recv call for list length */
    if ((payloadLen = recv(socket_fd, buffer, MAXBUF, 0)) == 0)
        throw SOCKET_FAIL;

    messagePacket = new Message(buffer, payloadLen);
    flag = messagePacket->get_flag();
    incoming_handle = messagePacket->get_text();

    delete messagePacket;

    /* Handle incoming list */
    if (flag == HANDLE_LIST_LEN)
    {
        handle_length = incoming_handle[0];
        for (int i = 1; i <= handle_length; i++) {
            printf("%c", incoming_handle[i]);
        }
        printf("\n");
    } else {
        throw LIST_EX;
    }
}

/* Basic exception catching */
void exceptionWrap(int ex, char **argv, char *handle)
{
    switch (ex)
    {
    case ARGUMENT_ERROR:
        cout << "cclient <handle> <server-IP> <server-port>\n";
        break;
    case CLIENT_NAME_ERROR:
        cout << "Could not resolve hostname " << argv[2] << "\n";
        break;
    case SOCKET_FAIL:
        cout << "socket() failed. errno " << errno << "\n";
        break;
    case SEND_FAIL:
        cout << "Message sending failed. errno " << errno << "\n";
    case HANDLE_EX:
        cout << "Handle " << handle << " is already taken.\n";
        break;
    case HANDLE_LENGTH_EX:
        cout << "Handles must be under 100 characters.\n";
        break;
    default:
        cout << "Unknown exception " << ex << "\n";
    }
}

void mainWrap(int argc, char **argv) {
    char *handle;
	struct hostent *server;
	int port;
    Cclient *client;

    try {
        if (argc != 4)
        {
            throw ARGUMENT_ERROR;
        }
        handle = argv[1];
        server = gethostbyname(argv[2]);
        stringstream(argv[3]) >> port;

        if (server == NULL)
        {
            throw IP_EX;
        }

        if (strlen(handle) > MAX_PAYLOAD)
        {
            throw HANDLE_LENGTH_EX;
        }

        client = new Cclient(handle, server, port);
        client->init();
        client->createMessage();
        client->exit();
    } catch (int ex) {
        exceptionWrap(ex, argv, handle);
    }
}

int main(int argc, char **argv) {
    mainWrap(argc, argv);
    return 0;
}