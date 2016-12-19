# Makefile

CC=gcc
SOBJS=server.o
COBJS=lotto.o
SOUT=server
COUT=lotto
CFLAGS=

all: server lotto

server: $(SOBJS)
	$(CC) -o $(SOUT) $(SOBJS) $(CFLAGS)

lotto: $(COBJS)
	$(CC) -o $(COUT) $(COBJS) $(CFLAGS)

server.o: server.c lotto.h
	$(CC) -c server.c $(CFLAGS)

lotto.o: lotto.c
	$(CC) -c lotto.c $(CFLAGS)

clean:
	rm -f $(SOBJS) $(COBJS) $(SOUT) $(COUT)
