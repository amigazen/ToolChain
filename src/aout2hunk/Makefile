# aout2hunk - Convert a.out object files to Amiga DOS hunk format (C89)
# Build: make (from this directory)

CC = cc
CFLAGS = -Wall -Wextra -pedantic -std=c89 -O2

aout2hunk: aout2hunk.c aout.h
	$(CC) $(CFLAGS) -o aout2hunk aout2hunk.c

clean:
	rm -f aout2hunk

.PHONY: clean
