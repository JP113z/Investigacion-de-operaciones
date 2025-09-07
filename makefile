CC = gcc
CFLAGS = $(shell pkg-config --cflags gtk+-3.0)
LIBS = $(shell pkg-config --libs gtk+-3.0)

TARGETS = main pending floyd

all: $(TARGETS)

main: main.c
	$(CC) main.c -o main $(CFLAGS) $(LIBS)

pending: pending.c
	$(CC) pending.c -o pending $(CFLAGS) $(LIBS)

floyd: PR01/floyd.c
	$(CC) PR01/floyd.c -o floyd $(CFLAGS) $(LIBS)

clean:
	rm -f $(TARGET)