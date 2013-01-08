CC=gcc
CFLAGS=-Wall -Werror -g -Wextra -Wno-unused-parameter
LDFLAGS=

all:  mtserver
clean: 
	rm mtserver *.o

mtserver: mtserver.c mtserver.h
	$(CC) $(CFGLAGS) $(LDFLAGS) $? -o $@
