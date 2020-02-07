# Makefile for CPE464 tcp test code
# written by Hugh Smith - April 2019
# Adapted for C++ compilation by 
# Daniel Jones - February 2020

COMPILER = g++
FLAGS = -Wall -Werror

all:  cclient server

cclient: cclient.cpp message.h message.cpp cclient.h
	$(COMPILER) $(FLAGS) cclient.cpp message.cpp -o cclient

server: server.cpp server.h message.h message.cpp
	$(COMPILER) $(FLAGS) server.cpp message.cpp -o server

# quiet: server.cpp cclient.cpp message.h message.cpp server_main.cpp
# 	$(COMPILER) cclient.cpp message.cpp -o cclient
# 	$(COMPILER) server.cpp server_main.cpp message.cpp -o server

clean:
	rm cclient server
