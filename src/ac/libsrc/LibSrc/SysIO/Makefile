
#  Libraries and headers for PDC release 3.3 (C) 1989 Lionel Hummel.
#  PDC Software Distribution (C) 1989 Lionel Hummel and Paul Petersen.
#  PDC I/O Library (C) 1987 by J.A. Lydiatt.
# 
#  This code is freely redistributable upon the conditions that this 
#  notice remains intact and that modified versions of this file not
#  be included as part of the PDC Software Distribution without the
#  express consent of the copyright holders.  No warrantee of any
#  kind is provided with this code.  For further information, contact:
# 
#   PDC Software Distribution    Internet:                     BIX:
#   P.O. Box 4006             or hummel@cs.uiuc.edu            lhummel
#   Urbana, IL  61801-8801       petersen@uicsrd.csrd.uiuc.edu

#  Makefile to generate UNIX-style I/O library functions

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
AFLAGS	=
#CFLAGS	= +INDU:include.h -g
#AFLAGS	= -g

LIBRARIAN = libr
INSERT  = -r
SORT	= -x
LIBDIR	= pdc:lib
PDCINCDIR	= pdc:include
LIB	= PDC.lib

SRC1 = Chk_Abort.c _abort.c exit.c isatty.c access.c
OBJ1 = Chk_Abort.o _abort.o exit.o isatty.o access.o
SRC2 = malloc.c rename.c unlink.c _main.c ctype.c
OBJ2 = malloc.o rename.o unlink.o _main.o ctype.o
SRC3 = close.c lseek.c open.c close.c read.c syserr.c write.c
OBJ3 = close.o lseek.o open.o close.o read.o syserr.o write.o
SRC4 = lib.asm globals.c
OBJ4 = lib.o globals.o
SRC5 = sleep.c 
OBJ5 = sleep.o 

ALLOBJ = $(OBJ1) $(OBJ2) $(OBJ3) $(OBJ4) $(OBJ5)

.SUFFIXES:	.o .a .asm .s .c

.c.o:
	$(CC) $(CFLAGS) -c $*.c

.asm.o:
	$(CC) $(AFLAGS) -c $*.asm

all:	$(ALLOBJ) collect

collect: $(ALLOBJ)
	$(LIBRARIAN) $(INSERT) $(SORT) $(LIBDIR)/$(LIB) $(ALLOBJ)

clean:
	delete \#?.o
