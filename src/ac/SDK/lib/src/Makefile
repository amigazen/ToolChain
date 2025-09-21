# The following commands need to be in your path: 
#    join cd bind make append

# Note: This makefile assumes your AC Program disk has been assigned the
# logical name of "AC:" and that the .fd files are to be taken from your
# 1.3 Extras disk.
#
# If you have Amiga C language header files (such as those in the
# NDU or the commercial compilers), you can type "make complete"
# and the version of amiga.lib generated will also contain the
# support functions whose source is contained in this directory.

# What we're building:
TARGETLIB = AC:lib/amiga.lib

# The location of this directory:
BINDDIR	  = AC:lib/src/

# The directory containing your .fd files:
FDDIR	  = "Extras 1.3:FD1.3"

# The directory containing your assembly language includes if you have them:
ASMINCLUDES = i:
AFLAGS	  = -I$(ASMINCLUDES)

# The directory containing your C language includes if you have them:
CINCLUDES = vd0:include
CFLAGS	  = +I$(CINCLUDES)

LIBS1	= dos_lib.lib			 \
	  exec_lib.lib			 \
	  graphics_lib.lib		 \
	  icon_lib.lib			 \
	  intuition_lib.lib		 \
	  layers_lib.lib		 \
	  mathieeedoubbas_lib.lib	 \
	  mathieeedoubtrans_lib.lib	 \
	  potgo_lib.lib			 \
	  console_lib.lib		 \
	  timer_lib.lib

LIBS2	= translator_lib.lib		 \
	  diskfont_lib.lib		 \
	  expansion_lib.lib 		 \
	  mathffp_lib.lib		 \
	  mathtrans_lib.lib		 \
	  romboot_lib.lib		 \
	  clist_lib.lib

LIBS 	= $(LIBS1) $(LIBS2)

OBJS	= NewList.o	\
	  DeletePort.o DeleteExtIO.o DeleteStdIO.o DeleteTask.o	\
	  CreatePort.o CreateExtIO.o CreateStdIO.o CreateTask.o

.SUFFIXES:  .fd .lib .asm .o

.c.o:
	ccx -c $(CFLAGS) $*.c

.asm.o:
	A68k $(AFLAGS) $*.asm

.fd.lib:
	bind $*.fd -f $(BINDDIR)bindfile -oRAM:$*.lib -rPDC -wRAM:

stubs:
	cd $(FDDIR)
	make -f $(BINDDIR)Makefile $(LIBS)
	cd RAM:
	append $(TARGETLIB) $(LIBS)
	delete \#?.lib
	cd $(BINDDIR)

functions:	$(OBJS)
	append $(TARGETLIB) $(OBJS)

complete: stubs functions

clean:
	delete RAM:\#?.lib

dos_lib.lib: dos_lib.fd

exec_lib.lib: exec_lib.fd

graphics_lib.lib: graphics_lib.fd

icon_lib.lib: icon_lib.fd

intuition_lib.lib: intuition_lib.fd

layers_lib.lib: layers_lib.fd

mathieeedoubbas_lib.lib: mathieeedoubbas_lib.fd

mathieeedoubtrans_lib.lib: mathieeedoubtrans_lib.fd

potgo_lib.lib: potgo_lib.fd

console_lib.lib: console_lib.fd

timer_lib.lib: timer_lib.fd

translator_lib.lib: translator_lib.fd

diskfont_lib.lib: diskfont_lib.fd

expansion_lib.lib: expansion_lib.fd

mathffp_lib.lib: mathffp_lib.fd		

mathtrans_lib.lib: mathtrans_lib.fd

romboot_lib.lib: romboot_lib.fd

clist_lib.lib: clist_lib.fd
