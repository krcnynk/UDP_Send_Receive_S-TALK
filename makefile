CC=gcc
CFLAGS=-I.

s_talk:	list.o main.o
	$(CC) -o s-talk list.o main.o -pthread