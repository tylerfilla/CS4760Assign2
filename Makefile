#
# CS 4760
# Assignment 1
# Tyler Filla
#

AR=ar
CC=cc
CFLAGS=-Wall -std=c99

all: master palin

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

%.a: %.o
	$(AR) rcs $@ $<

master: master.o shared.a
	$(CC) -o $@ $^ $(CFLAGS)

palin: palin.o shared.a
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm *.a *.o master palin

.PHONY: clean
