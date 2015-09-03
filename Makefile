CC=gcc
CPPFLAGS=-I.
CFLAGS=-g -Wall -Wextra -std=gnu11 -pedantic
LIBS=

RM=rm -f

.PHONY: all clean rebuild

all: socketcan-raw-demo socketcan-bcm-demo socketcan-cyclic-demo

socketcan-raw-demo: socketcan-raw-demo.o util.o
	$(CC) -o $@ $^ $(LIBS)

socketcan-bcm-demo: socketcan-bcm-demo.o util.o
	$(CC) -o $@ $^ $(LIBS)

socketcan-cyclic-demo: socketcan-cyclic-demo.o
	$(CC) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) *.o
	$(RM) socketcan-raw-demo
	$(RM) socketcan-bcm-demo
	$(RM) socketcan-cyclic-demo

rebuild: clean all

