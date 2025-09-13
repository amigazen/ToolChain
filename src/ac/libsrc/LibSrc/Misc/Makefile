
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

# Makefile to generate miscellaneous functions

CC	= ccx
CFLAGS	= +INDU:include.h
AFLAGS	= -Wa-Ii:
LIBR	= libr
ADD	= -a
INSERT = -r
SORT	= -x

SRC	= getopt.c time.c
OBJ	= getopt.o time.o

.SUFFIXES: .c .asm .o .lib

.asm.o:
	$(CC) $(AFLAGS) -c $*.asm

.c.o: 
	$(CC) $(CFLAGS) -c $*.c

all:	$(OBJ) collect

collect: $(OBJ)
	$(LIBR) $(INSERT) $(SORT) PDC:lib/pdc.lib $(OBJ)

clean:
	delete \#?.o
