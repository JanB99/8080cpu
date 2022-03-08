CC=gcc
CFLAGS=-Wall

all:build


build:
	$(CC) -o output main.c cpu.c $(CFLAGS)

run:build
	output

clean:
	del output