.PHONY: clean, mrproper
CC = gcc
CFLAGS = -g -Wall

all: tp2

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

tp2: tp2.o
	$(CC) $(CFLAGS) -o $@ $+

clean:
	rm -f *.o core.*

mrproper: clean
	rm -f tp2
