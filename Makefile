CC = cc
CFLAGS = -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L
OBJS = main.o ipc.o tree.o cJSON.o layout_map.o flatten.o

swaytile: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^
	rm -f *.o

clean:
	rm -f swaytile *.o
