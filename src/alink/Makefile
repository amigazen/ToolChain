# alink - Amiga DOS object linker (C89, standard library only)
# Build with: make -f alink/Makefile (from TCC/src) or make (from alink/)

CC = scc
CFLAGS = -O2
ALINK_SRC = alink.c read.c sym.c section.c correct.c write.c mem.c io.c
ALINK_OBJ = $(ALINK_SRC:.c=.o)

all: alink

alink: $(ALINK_OBJ)
	$(CC) $(CFLAGS) -o $@ $(ALINK_OBJ)

%.o: %.c alink.h doshunks.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f alink $(ALINK_OBJ)

.PHONY: all clean
