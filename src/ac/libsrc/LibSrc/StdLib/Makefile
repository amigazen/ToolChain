
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

#  Makefile to generate standard C library functions

CC	= ccx
CFLAGS	= +INDU:include.h
AFLAGS	= -Wa-INDU:include.i
LIBR	= libr
ADD	= -a
INSERT = -r
SORT	= -x
LIBRARY	= PDC:lib/PDC.lib
RM	= delete

SRC1	= system0.asm
SRC1	= environment.c system.c
SRC2	= strtol.c strtoul.c strtod.c strtotd.c strtotud.c
SRC3	= atof.c atoi.c atol.c
OBJ0	= system0.o
OBJ1	= environment.o system.o
OBJ2	= strtol.o strtoul.o strtod.o strtosd.o strtosud.o
OBJ3	= atof.o atoi.o atol.o

OBJS	= $(OBJ0) $(OBJ1) $(OBJ2) $(OBJ3) 

.SUFFIXES: .c .asm .o .lib

.asm.o:
	$(CC) $(AFLAGS) -c $*.asm

.c.o: 
	$(CC) $(CFLAGS) -c $*.c

all:	$(OBJS)

collect: $(OBJS)
	$(LIBR) $(INSERT) $(SORT) $(LIBRARY) $(OBJS)

clean:
	$(RM) \#?.o
