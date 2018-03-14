CFLAGS = -Wall -std=c99 -g

append-write: append-write.o
	$(CC) $(CFLAGS) -o $@ $<

append-check: append-check.o
	$(CC) $(CFLAGS) -o $@ $<
