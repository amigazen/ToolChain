
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

# Makefile to generate PDC's Math.lib

# i: should be assigned to the directory containing Amiga/Metacomco format 
# assembly language header files.
AFLAGS	= -Ii:

LIBR	= libr
ADD	= -a
INSERT	= -r
SORT	= -x

LIB	= PDC:lib/Math.lib

LEVEL0	= ldexp.o frexp.o modf.o matherr.o pmlerr.o sign.o mod.o poly.o
LEVEL1	= dabs.o acosh.o asinh.o atan2.o atanh.o max.o min.o cordic.o 
LEVEL2	= rint.o 
FPSTUB	= single.o double.o ieeedoubtrans.o
FPIO	= mprintf.o mscanf.o

FPOLD1	= floor.o atan.o sin.o cos.o tan.o tanh.o sinh.o cosh.o exp.o 
FPOLD2	= log.o pow.o sqrt.o asin.o acos.o log10.o 

OBJS	= $(LEVEL2) $(LEVEL1) $(LEVEL0) $(FPSTUB) $(FPIO)

.SUFFIXES: .c .o .asm

.c.o:
	CCX -c $*.c

.asm.o:
	A68K $(AFLAGS) $*.asm

all:	$(FPSTUB) $(OBJS)

collect:	$(FPSTUB) $(OBJS)
	$(LIBR) $(INSERT) $(SORT) $(LIB) $(OBJS)

$(OBJS)  : pml.h

clean:	
	delete \#?.o
