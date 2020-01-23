# Example makefile for CPE464 program 1
#
# 

CC = gcc
CFLAGS = -g -Wall 


all:  trace 

trace: trace.c checksum.c ARP_packet.c ethernet_packet.c ICMP_packet.c \
		IP_packet.c TCP_packet.c UDP_packet.c
	$(CC) $(CFLAGS) -o trace trace.c checksum.c ARP_packet.c ethernet_packet.c ICMP_packet.c \
		IP_packet.c TCP_packet.c UDP_packet.c -lpcap 

clean:
	rm -f trace
