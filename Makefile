
SHELL = /bin/sh

NAME = ditvector
CC = g++
CFLAGS = -Wall -g -std=c++20

.PHONY: all test clean info

all: test

test: test.o
	@$(CC) $(CFLAGS) -o test test.o

profile: test.o
	@$(CC) $(CFLAGS) -pg -o profile test.o

test.o: test.cpp avl.hpp bit_vector.hpp bit_vector.cpp
	@$(CC) $(CFLAGS) -c test.cpp

clean:
	@rm -rf *.o

info:
	@echo ""

