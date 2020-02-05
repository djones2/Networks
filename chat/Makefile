# Makefile for CPE464 tcp test code
# written by Hugh Smith - April 2019
# Adapted for C++ compilation by 
# Daniel Jones - February 2020

COMPILER = g++
FLAGS = -Wall -Werror

all:  cclient server

cclient: cclient.cpp message.h message.cpp cclientMain.cpp cclient.h
	$(COMPILER) $(FLAGS) cclient.cpp cclientMain.cpp message.cpp -o cclient

server: serverMain.cpp server.cpp server.h serverMain.cpp message.h message.cpp
	$(COMPILER) $(FLAGS) server.cpp serverMain.cpp message.cpp -o server
	
clean:
	rm cclient server