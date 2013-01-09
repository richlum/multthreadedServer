CC=gcc
CFLAGS=-Wall -Werror -g -Wextra -Wno-unused-parameter
LDFLAGS=-lpthread

all:  mtserver
clean: 
	rm mtserver *.o

mtserver: mtserver.c mtserver.h
	$(CC) $(CFLAGS) $(LDFLAGS) $? -o $@
