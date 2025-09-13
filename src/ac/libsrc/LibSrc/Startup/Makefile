
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

#  Makefile to generate library code relating to PDC's runtime environment

#	In order to recompile some of these library files, you need the
#	standard Amiga header files that can be purchased directly from
#	Commodore as part of the Native Developer Update.  If you own one of
#	the commercial C compilers, their headers will a suffice.  In
#	order to build these library functions, be sure that PDC knows where
#	to find your Amiga header files.  (Here, I use NDU:include.h for my C
#	headers and acrt0 expects a directory assign'd the logical name I: in 
#	order to assemble).  For the assembly flags used for acrt0.asm, 
#	assign i: to the directory containing your Amiga headers.  (If you
#	don't have 'em, you should read the documentation re: standard headers
#	and libraries.)
#
CFLAGS	= +INDU:include.h
#CFLAGS	= +INDU:include.h -Wa-d
AFLAGS  = -Wa -INDU:include.i
#AFLAGS  = -Wa "-d -Ii:"

LIBRARIAN = libr
INSERT	= -r
SORT	= -x
LIBDIR	= pdc:lib
LIB	= PDC.lib

CC	= ccx
AS	= a68k
CP	= copy
TOUCH	= touch
LIBRARIAN = libr
SORT	= -x

SRCS = TWspec.c 
OBJS = TWspec.o

STARTUPSRC = acrt0.asm
STARTUPOBJ = acrt0.o

#	Integral part of acrt0.asm at present
SRCOLD = cli_parse.c wb_parse.c
OBJOLD = cli_parse.o wb_parse.o

.SUFFIXES:	.c .s .asm .o

.asm.o:
	$(CC) $(AFLAGS) -o $*.o -c $*.asm

.c.o:
	$(CC) $(CFLAGS) -o $*.o -c $*.c

all:	$(OBJS) collect

collect: $(OBJS) $(STARTUPOBJ)
	$(LIBRARIAN) $(INSERT) $(SORT) $(LIBDIR)/$(LIB) $(OBJS)
	$(CP) $(STARTUPOBJ) $(LIBDIR)

clean:
	delete \#?.o
