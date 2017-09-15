#
# CS 4760
# Assignment 1
# Tyler Filla
#

CC=cc
CFLAGS=-Wall -std=c99

all: master palin

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

master: master.o
	$(CC) -o $@ $< $(CFLAGS)

palin: palin.o
	$(CC) -o $@ $< $(CFLAGS)

clean:
	rm *.o master palin

.PHONY: clean
