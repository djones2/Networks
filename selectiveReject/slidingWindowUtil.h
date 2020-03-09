#ifndef WINDOW_H
#define WINDOW_H
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

typedef struct window Window;

struct window
{
	int32_t lower_edge;
	int32_t current;
	int32_t upper_edge;
	int32_t size;
	int *dirty_bit;
	// This is my circular queue. This grows with each window
	// entry and is checked to be within window bounds. Packets
	// are entered in through a modulo.
	Packet *buffer;
};

void create_window(Window *window, int size);
void to_window_buffer(Window *window, Packet *packet);
void get_from_buffer(Window *window, Packet *packet, int seq_num);
void remove_from_buffer(Window *window, Packet *packet, int seq_num);
void pull_window_blinds(Window *window, int new_bottom);
int check_valid(Window *window, int seq_num);
int in_window(Window *window, int seq_num);
// Haha...
void close_window(Window *window);
int flush_buffer(Window *window);

#endif
