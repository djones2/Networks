#ifndef __WINDOW_H__
#define __WINDOW_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdint.h>

#include "gethostbyname.h"
#include "networks.h"
#include "cpe464.h"
#include <sys/types.h>
#include "srej.h"
#include "slidingWindow.h"
#include "checksum.h"

typedef struct window Window;

struct window {
	int32_t lower;
	int32_t current;
	int32_t upper;
	int32_t size;
	Packet *buffer;
	// dirty bit
	int *valid;
};

void create_window(Window *window, int window_size);
void to_window_buffer(Window *window, Packet *packet);
void from_buffer(Window *window, Packet *packet, int seq_num);
void remove_item(Window *window, Packet *packet, int seq_num);
void slide_window(Window *window, int lower_edge);
// see if these can be combined
int is_valid(Window *window, int seq_num);
int is_closed(Window *window);
int window_check(Window *window, int seq_num);
void free_window(Window *window);
int flush_buffer(Window *window);
void print_window(Window *window);

#endif