CC = gcc
CFLAGS = $(shell pkg-config --cflags gtk+-3.0)
LIBS = $(shell pkg-config --libs gtk+-3.0)

TARGETS = main pending PR01/floyd

all: $(TARGETS)

main: main.c
	$(CC) main.c -o main $(CFLAGS) $(LIBS)

pending: pending.c
	$(CC) pending.c -o pending $(CFLAGS) $(LIBS)

PR01/floyd: PR01/floyd.c
	$(CC) PR01/floyd.c -o PR01/floyd $(CFLAGS) $(LIBS) -lm

clean:
	rm -f main pending PR01/floyd
