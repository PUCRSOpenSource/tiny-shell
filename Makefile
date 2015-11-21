IDIR = ./include
SDIR = ./src
ODIR = ./obj

.PHONY: clean, mrproper

CC = gcc
CFLAGS = -g -Wall -I$(IDIR)

_DEPS =  fat.h shell.h
DEPS  = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = fat.o shell.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

all: sisop_tp2

$(ODIR)/%.o: $(SDIR)/%.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

sisop_tp2: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f *.o core.*

mrproper: clean
	rm -f sisop_tp2
