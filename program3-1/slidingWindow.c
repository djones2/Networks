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

void create_window(Window *window, int window_size)
{
    window->lower = 1;
    window->current = 1;
    //window->upper = window->lower + window_size;
    window->upper = window_size;
    window->size = window_size;
    window->buffer = calloc(window_size, sizeof(Packet));
    window->valid = calloc(window_size, sizeof(int));
}

int is_closed(Window *window)
{
    return window->current > window->upper;
}

void to_window_buffer(Window *window, Packet *packet)
{
    int i = (packet->seq_num - 1) % window->size;
    memcpy(window->buffer + i, packet, sizeof(Packet));
    window->valid[i] = 1;
}

void from_buffer(Window *window, Packet *packet, int seq_num)
{
    int i = (seq_num - 1) % window->size;
    memcpy(packet, window->buffer + i, sizeof(Packet));
}

void remove_item(Window *window, Packet *packet, int seq_num)
{
    int i = (seq_num - 1) % window->size;
    memcpy(packet, window->buffer + i, sizeof(Packet));
    window->valid[i] = 0;
}

void slide_window(Window *window, int lower_edge)
{
    int i;
    for (i = window->lower; i < lower_edge; i++) {
        window->valid[(i - 1) % window->size] = 0;
    }

    window->lower = lower_edge;
    window->upper = lower_edge + window->size - 1;

/*     if (window->current < window->lower) {
        window->current = window->lower;
    }
    CHECK THIS LOGIC 
     */
    window->current = window->current < window->lower ? window->lower : window->current;
}

int is_valid(Window *window, int seq_num)
{
    return window->valid[(seq_num - 1) % window->size];
}


int all_valid(Window *window)
{
    int i;

    for (i = window->lower; i <= window->upper; i++)
        if (window->valid[(i - 1) % window->size] == 0)
            return 0;

    return 1;
}

int flush_buffer(Window *window)
{
    int i;
    for (i = window->lower; i <= window->upper; i++) {
        if (window->valid[(i - 1) % window->size] == 1) {
            return 0;
        }
    }
    return 1;
}

int window_check(Window *window, int seq_num)
{
    return seq_num >= window->lower && seq_num <= window->upper;
}

void free_window(Window *window)
{
    free(window->buffer);
    free(window->valid);
}

// use for debug then delete
void print_window(Window *window)
{
    int i;

    printf("Window: (%d %d %d) valid: ", window->lower, window->current, window->upper);

    for (i = window->lower - 1; i < window->lower - 1 + window->size; i++)
    {
        printf("%d", window->valid[i % window->size]);
    }
    printf("\n");
}