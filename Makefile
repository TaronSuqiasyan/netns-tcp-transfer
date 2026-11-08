CC      = gcc
CFLAGS  = -Wall -Wextra -O2

all: sender/sender receiver/receiver

sender/sender: sender/sender.c
	$(CC) $(CFLAGS) -o $@ $<

receiver/receiver: receiver/receiver.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f sender/sender receiver/receiver receiver/received_*

.PHONY: all clean
