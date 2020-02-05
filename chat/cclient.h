#ifndef CCLIENT_H
#define CCLIENT_H


/* Class for all client instances */
class Cclient
{
    char *handle;
    struct hostent *server;
    int port;
    int socket_fd;
    struct sockaddr_in server_address;
    bool exitb;
    int sequence_number;

/* Initialize packet, begin message command, exit */
public:
    Cclient(char *handle, struct hostent *server, int port);
    void init();
    void createMessage();
    void exit();

/* Internal operations for every client, 
    made private to not be changed by outside user */
private:
    void handleCheck();
    void messageCheck();
    void inputCheck();
    int sendMessage(char *input);
    void broadcastMessage(char *input);
    void listCommand();
    int listLengthRequest();
    void handleRequest(int index);
};

#endif