
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

#	Makefile for the stdio routines in the PDC Function Libraries

CC	= ccx
LIBRARIAN = libr
ADD	= -a
INSERT	= -r
LIBDIR	= pdc:lib
CFLAGS	= +INDU:include.h
#CFLAGS	= +INDU:include.h -g

SRC1 = bufmgt.c fdopen.c fgets.c fopen.c fclose.c fread.c fwrite.c
SRC2 = fseek.c fgetc.c fputc.c fputs.c
OBJ1 = bufmgt.o fdopen.o fgets.o fopen.o fclose.o fread.o fwrite.o
OBJ2 = fseek.o fgetc.o fputc.o fputs.o
PRINTSRC = fprintf.c format.c fscanf.c
PRINTOBJ = fprintf.o format.o fscanf.o
OLDSRC = gets.c
OLDOBJ = gets.o

.SUFFIXES:	.o .a .c

.c.o:
	$(CC) $(CFLAGS) -c $*.c

# As a default, the gets function is not included in PDC.lib due to its
# inherent susceptibility to overrun.  Do not use it in new code.  If you
# must have it for compatibility reasons, add $(OLDOBJ) to the following
# dependency:

OBJS =	 $(OBJ1) $(OBJ2) $(PRINTOBJ)

All :	 $(OBJS) collect

collect: $(OBJ1) $(OBJ2) $(PRINTOBJ)
	$(LIBRARIAN) $(INSERT) $(LIBDIR)/PDC.lib $(OBJS)

clean:
	delete \#?.o
