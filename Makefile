CC=gcc
CFLAGS=-Wall -Werror -g -Wextra -Wno-unused-parameter
LDFLAGS=-lpthread

all:  mtserver mtclient
clean: 
	rm mtserver *.o

mtserver: mtserver.c mtserver.h
	$(CC) $(CFLAGS) $(LDFLAGS) $? -o $@

mtclient: mtclient.c
	$(CC) $(CFLAGS) $(LDFLAGS) $? -o $@

pi : CFLAGS += -Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s
pi : mtserver
