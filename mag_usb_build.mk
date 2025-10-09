# Assuming you have a Makefile like this:
CC = gcc
CFLAGS = -Wall -O2 -lpthread

OBJS = main.o i2c.o cmdmgr.o rm3100.o config.o

mag-usb: $(OBJS)
	$(CC) -o mag-usb $(OBJS) $(CFLAGS)

main.o: main.c main.h
	$(CC) $(CFLAGS) -c main.c

config.o: config.c config.h main.h
	$(CC) $(CFLAGS) -c config.c

i2c.o: i2c.c i2c.h
	$(CC) $(CFLAGS) -c i2c.c

cmdmgr.o: cmdmgr.c cmdmgr.h
	$(CC) $(CFLAGS) -c cmdmgr.c

rm3100.o: rm3100.c rm3100.h
	$(CC) $(CFLAGS) -c rm3100.c

clean:
	rm -f *.o mag-usb

.PHONY: clean
