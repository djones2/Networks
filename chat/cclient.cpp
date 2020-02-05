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
    Message *msg;
    int msg_size;
    int sendb;
    exitb = false;
    sequence_number = 0;

    /* Set up server information */
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons((short)port);

    /* Cooy address information */
    memcpy(&(server_address.sin_addr.s_addr), server->h_addr, server->h_length);

    /* Obtain socket file descriptor */
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        throw SOCK_EX;

    /* Create connection */
    if (connect(socket_fd, (struct sockaddr *)(&server_address), 16) == -1)
        throw CONNECT_EX;

    /* Populate message fields */
    msg = new Message();
    msg->set_flag(INITIAL_PCKT);
    msg->set_from(handle, strlen(handle));
    msg->set_sequence_number(sequence_number++);
    /* Package the message into a packet */
    msg_size = msg->pack();

    /* Send call */
    if ((sendb = send(socket_fd, msg->sendable(), msg_size, 0)) == -1)
        throw SEND_EX;

    /* Delete temporary message data structure */
    delete msg;
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
        if (select(socket_fd + 1, &read_set, NULL, NULL, &wait_time) == -1) {
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
    Message *msg;
    uint8_t *buffer;
    int msg_size;
    int error;
    int flag;

    /* Exit request */
    msg = new Message();
    msg->set_flag(EXIT_REQUEST);
    msg->set_from(handle, strlen(handle));
    msg_size = msg->pack();


    msg->set_sequence_number(sequence_number++);

    if ((error = send(socket_fd, msg->sendable(), msg_size, 0)) == -1)
        throw SEND_EX;

    delete msg;

    /* Wait for ACK from server */
    buffer = (uint8_t *)calloc(MAXBUF, sizeof(uint8_t));

    /* Obtain message size. If 0, connection closed */
    if ((msg_size = recv(socket_fd, buffer, MAXBUF, 0)) == 0)
        throw CONNECT_EX;

    /* Create new message to get the exit flag */
    msg = new Message(buffer, msg_size);
    flag = msg->get_flag();
    free(buffer);
    delete msg;

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


void Cclient::handleCheck()
{
    uint8_t *buffer;
    int message_size, flag;
    Message *msg;

    buffer = (uint8_t *)calloc(MAXBUF, sizeof(uint8_t));

    message_size = recv(socket_fd, buffer, MAXBUF, 0);

    if (message_size == 0)
        throw CONNECT_EX;

    msg = new Message(buffer, message_size);
    flag = msg->get_flag();
    free(buffer);
    delete msg;

    if (flag == 2)
        return;
    else if (flag == 3)
        throw HANDLE_EX;
    else
        throw MYSTERY_EX;
}

void Cclient::inputCheck()
{
    char input_buffer[MAXBUF];
    char sel;
    int sent_to_self;

    sent_to_self = 0; //a flag which lets us know whether or not to print out the prompt

    fgets(input_buffer, MAXBUF, stdin);
    try
    {
        if (strlen(input_buffer) > 1)
        {
            input_buffer[0] == '%' ? sel = input_buffer[1] : sel = input_buffer[0];

            if (sel == 'm' || sel == 'M')
            {
                sent_to_self = sendMessage(input_buffer);
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
                printf("M - message\n");
                printf("B - broadcast\n");
                printf("L - list\n");
                printf("E - exit\n");
            }
        }
        else
            printf("%%<option> <handle> <message>\n");
    }
    catch (int ex)
    {
        if (ex == MESSAGE_SIZE_EX)
            printf("Messages must be under 1000 characters.\n");
        else
            throw ex;
    }

    if (!sent_to_self)
    {
        printf("> ");
        fflush(stdout);
    }
}

void Cclient::messageCheck()
{
    uint8_t *buffer;
    int message_size, flag;
    char handle[101];
    Message *msg;

    buffer = (uint8_t *)calloc(MAXBUF, sizeof(uint8_t));

    message_size = recv(socket_fd, buffer, MAXBUF, 0);

    // connection closed
    if (message_size == 0)
        throw CONNECT_EX;

    msg = new Message(buffer, message_size);

    flag = msg->get_flag();

    printf("\n");

    if (flag == 6)
        msg->print();
    else if (flag == 7)
    {
        memcpy(handle, msg->get_to(), msg->get_to_length());
        handle[msg->get_to_length()] = '\0';
        printf("Cclient with handle %s does not exist.\n", handle);
    }

    delete msg;
    free(buffer);

    printf("> ");
    fflush(stdout);
}

int Cclient::sendMessage(char *input)
{
    char to[101];
    int to_length;
    char garbage[100];
    char *message_start;
    int message_size;
    Message *msg;
    int error, sent_to_self;

    sscanf(input, "%s %100s", garbage, to);
    to_length = strlen(to);

    message_start = strstr(input, to) + strlen(to) + 1;

    sent_to_self = (strcmp(to, handle) == 0);

    if (strlen(message_start) > MAX_PAYLOAD)
        throw MESSAGE_SIZE_EX;

    for (message_size = 0; message_start[message_size] != '\0'; message_size++)
        ;

    message_size++;

    msg = new Message();
    msg->set_to(to, to_length);
    msg->set_from(handle, strlen(handle));

    if (message_size == 0)
        msg->set_text("\n", 1);
    else
        msg->set_text(message_start, message_size);

    msg->set_flag(6);
    msg->set_sequence_number(sequence_number++);
    message_size = msg->pack();

    error = send(socket_fd, msg->sendable(), message_size, 0);

    if (error == -1)
        throw SEND_EX;

    return sent_to_self;
}

void Cclient::broadcastMessage(char *input)
{
    char garbage[100];
    char *message_start;
    int message_size;
    Message *msg;
    int error;

    sscanf(input, "%s", garbage);

    message_start = input + strlen(garbage) + 1;

    if (strlen(message_start) > MAX_PAYLOAD)
        throw MESSAGE_SIZE_EX;

    for (message_size = 0; message_start[message_size] != '\0'; message_size++)
        ;

    message_size++;

    msg = new Message();
    msg->set_from(handle, strlen(handle));
    msg->set_text(message_start, message_size);

    if (message_size == 0)
        msg->set_text("\n", 1);
    else
        msg->set_text(message_start, message_size);

    msg->set_flag(6);
    msg->set_sequence_number(sequence_number++);
    message_size = msg->pack();

    error = send(socket_fd, msg->sendable(), message_size, 0);

    if (error == -1)
        throw SEND_EX;
}

void Cclient::listCommand()
{
    int list_length;

    list_length = listLengthRequest();

    for (int i = 0; i < list_length; i++)
        handleRequest(i);
}

int Cclient::listLengthRequest()
{
    Message *msg;
    uint8_t buffer[MAXBUF];
    int msg_size, error, flag, length;

    //Request list length from server, flag 10
    msg = new Message();
    msg->set_flag(10);
    msg->set_from(handle, strlen(handle));
    msg->set_sequence_number(sequence_number++);
    msg_size = msg->pack();

    error = send(socket_fd, msg->sendable(), msg_size, 0);

    if (error == -1)
        throw SEND_EX;

    delete msg;

    //Get list length from server
    msg_size = recv(socket_fd, buffer, MAXBUF, 0);

    // Connection lost ?
    if (msg_size == 0)
        throw CONNECT_EX;

    msg = new Message(buffer, msg_size);
    flag = msg->get_flag();
    length = ntohl(((uint32_t *)msg->get_text())[0]);
    delete msg;

    // Check Flag for 11
    if (flag != 11)
        throw LIST_EX;

    return length;
}

void Cclient::handleRequest(int index)
{
    Message *msg;
    uint8_t buffer[MAXBUF];
    char *incoming_handle;
    uint8_t handle_length;
    int msg_size, error, flag;

    //Request handle with index
    msg = new Message();
    msg->set_flag(12);
    msg->set_from(handle, strlen(handle));
    msg->set_int(index);
    msg->set_sequence_number(sequence_number++);
    msg_size = msg->pack();

    error = send(socket_fd, msg->sendable(), msg_size, 0);

    if (error == -1)
        throw SEND_EX;

    delete msg;

    msg_size = recv(socket_fd, buffer, MAXBUF, 0);
    // Connection lost ?
    if (msg_size == 0)
        throw CONNECT_EX;

    msg = new Message(buffer, msg_size);
    flag = msg->get_flag();
    incoming_handle = msg->get_text();

    delete msg;

    // Check Flag for 13
    if (flag == 13)
    {

        handle_length = incoming_handle[0];

        for (int i = 1; i <= handle_length; i++)
            printf("%c", incoming_handle[i]);

        printf("\n");
    }
    else if (flag == 14)
    { 
        // unsure
    }
    else
        throw LIST_EX;
}
