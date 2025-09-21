#
CC	= ccx
#
#	In order to recompile some of these library files, you need the
#	standard Amiga header files that can be purchased directly from
#	Commodore as part of the Native Developer Update.  If you own one of
#	the commercial C compilers, their headers will a suffice.  In
#	order to build these library functions, be sure that PDC knows where
#	to find your Amiga header files.  (Here, I use NDU:include.h)
#
CFLAGS	= +INDU:include.h
#CFLAGS	= +INDU:include.h -Wa-d

LIBRARIAN	= libr
INSERT		= -r
SORT		= -x
RM		= delete

SRC1 = bcmp.c bcopy.c bzero.c index.c memccpy.c memchr.c memcmp.c
SRC2 = memcpy.c memset.c rindex.c strcat.c strchr.c strcmp.c strcpy.c strcspn.c
SRC3 = strerror.c strlen.c strncat.c strncmp.c strncpy.c strpbrk.c strrchr.c
SRC4 = strspn.c strstr.c strtok.c tolower.c toupper.c
SRCX = strdup.c strndup.c strdcat.c

OBJ1 = bcmp.o bcopy.o bzero.o index.o memccpy.o memchr.o memcmp.o
OBJ2 = memcpy.o memset.o rindex.o strcat.o strchr.o strcmp.o strcpy.o strcspn.o
OBJ3 = strerror.o strlen.o strncat.o strncmp.o strncpy.o strpbrk.o strrchr.o
OBJ4 = strspn.o strstr.o strtok.o tolower.o toupper.o
OBJX = strdup.o strndup.o strdcat.o

.SUFFIXES:	.o .a .s .c .lib

.c.o:
	$(CC) $(CFLAGS) -c $*.c

All: $(OBJS) collect

collect: $(OBJ1) $(OBJ2) $(OBJ3) $(OBJ4) $(OBJX)
	$(LIBRARIAN) $(INSERT) $(SORT) PDC:lib/PDC.lib $(OBJ1) $(OBJ2) $(OBJ3)
	$(LIBRARIAN) $(INSERT) $(SORT) PDC:lib/PDC.lib $(OBJ4) $(OBJX)

clean:
	delete \#?.o
