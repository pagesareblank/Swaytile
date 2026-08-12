CC = cc
CFLAGS = -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L

PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin

OBJS = main.o ipc.o tree.o cJSON.o layout_map.o flatten.o

swaytile: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^
	rm -f *.o

install: swaytile
	install -d $(BINDIR)
	install -m 755 swaytile $(BINDIR)/swaytile

uninstall:
	rm -f $(BINDIR)/swaytile

clean:
	rm -f swaytile *.o
