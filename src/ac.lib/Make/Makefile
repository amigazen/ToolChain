
OBJS = main.o make.o rules.o reader.o input.o macro.o check.o

CFLAGS =

make : $(OBJS)
	ccx -o make $(OBJS)

clean:
	delete (\#?.o|make)