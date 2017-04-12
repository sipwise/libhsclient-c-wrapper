CFILES=$(wildcard *.c)
BINFILES=$(CFILES:.c=.bin)

all: $(BINFILES)

%.bin: %.c
	gcc -o $@ $< -Wall -I../include -L../src -lhsclient-c-wrapper

clean:
	rm -f *.bin
