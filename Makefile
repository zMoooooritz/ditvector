
SHELL = /bin/sh

NAME = ditvector
CC = g++
CFLAGS = -Wall -g -std=c++20

.PHONY: all example test clean info

all: example

example: example.o
	@$(CC) $(CFLAGS) -o example example.o

example.o: example.cpp avl.hpp bit_vector.hpp bit_vector.cpp
	@$(CC) $(CFLAGS) -c example.cpp

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

