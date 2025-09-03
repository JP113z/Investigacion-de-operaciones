CC = gcc
CFLAGS = $(shell pkg-config --cflags gtk+-3.0)
LIBS = $(shell pkg-config --libs gtk+-3.0)

TARGETS = main pending

all: $(TARGETS)

main: main.c
	$(CC) main.c -o main $(CFLAGS) $(LIBS)

pending: pending.c
	$(CC) pending.c -o pending $(CFLAGS) $(LIBS)

clean:
	rm -f $(TARGETS)

