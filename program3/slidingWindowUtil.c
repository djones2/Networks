#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "slidingWindowUtil.h"

// Establish the window and it's attributes
void create_window(Window *window, int size)
{
	window->lower_edge = 1;
	window->current = 1;
	window->upper_edge = size;
	window->size = size;
	// Calloc for window with amount for enough packets
	// in the buffer
	window->buffer = calloc(size, sizeof(Packet));
	// Int size for the dirty bit for valid buffer
	// locations
	window->dirty_bit = calloc(size, sizeof(int));
}

// Insert into window buffer on srej or similar
void to_window_buffer(Window *window, Packet *packet)
{
	int buf = (packet->seq_num - 1) % window->size;
	memcpy(window->buffer + buf, packet, sizeof(Packet));
	window->dirty_bit[buf] = 1;
}

// Get item from buffer
void get_from_buffer(Window *window, Packet *packet, int seq_num)
{
	int buf_index = (seq_num - 1) % window->size;
	memcpy(packet, window->buffer + buf_index, sizeof(Packet));
}

// Remove item from buffer
void remove_from_buffer(Window *window, Packet *packet, int seq_num)
{
	int buf_index = (seq_num - 1) % window->size;
	memcpy(packet, window->buffer + buf_index, sizeof(Packet));
	window->dirty_bit[buf_index] = 0;
}

// Change window attributes due to packet activity
void pull_window_blinds(Window *window, int new_bottom)
{
	int i;
	for (i = window->lower_edge; i < new_bottom; i++)
	{
		window->dirty_bit[(i - 1) % window->size] = 0;
	}
	window->upper_edge = new_bottom + window->size - 1;
	window->lower_edge = new_bottom;
	window->current = window->current < window->lower_edge ? window->lower_edge : window->current;
}

int check_valid(Window *window, int seq_num)
{
	return window->dirty_bit[(seq_num - 1) % window->size];
}

int flush_buffer(Window *window)
{
	int i;

	for (i = window->lower_edge; i <= window->upper_edge; i++)
		if (window->dirty_bit[(i - 1) % window->size] == 1)
			return 0;

	return 1;
}

int in_window(Window *window, int seq_num)
{
	return seq_num >= window->lower_edge && seq_num <= window->upper_edge;
}

// Free w
void close_window(Window *window)
{
	free(window->buffer);
	free(window->dirty_bit);
}
