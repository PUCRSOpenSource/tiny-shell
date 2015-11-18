IDIR = ./include
SDIR = ./src
ODIR = ./obj

.PHONY: clean, mrproper

CC = gcc
CFLAGS = -g -Wall

_DEPS = fat.h shell.h
DEPS  = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ  = main.o fat.o shell.o
OBJ   = $(patsubst %,$(ODIR)/%,$(_OBJ))






$(ODIR)/%.o: $(SDIR)/%.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

tp2: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o core.*

mrproper: clean
	rm -f ./fat.part
	rm -f tp2
