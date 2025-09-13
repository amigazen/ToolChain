#
#  Utility is named ccx to avoid conflict with Manx cc.
#  Rename to "cc" if you prefer.
#

CC	= ccx
PROG	= ccx
AINCLUDES = NDU:include.h
CFLAGS	= -Wc-b +I$(AINCLUDES)
#CFLAGS	= -n +l -Damiga
#CFLAGS	= +l +m -Damiga
#CFLAGS	= +l -Damiga
CP	= copy
RM	= delete
PDCDIR	= pdc:bin

OBJS	= ccx.o

all:	$(PROG)

$(PROG):	$(OBJS)
	ccx $(OBJS)

clean:
	-$(RM) $(CC)
	-$(RM) $(OBJS)

install:	$(PROG)
	$(CP) $(PDCDIR)/$(PROG) $(PROG).bkp
	$(CP) $(PROG) $(PDCDIR)/$(PROG)
