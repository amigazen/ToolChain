# AC 4.0 feature demonstration - build with ccx on AmigaOS

EXEC = ac_feature_demo
SRCS = ac_feature_demo.c ac_feature_demo_pragmas.c
INCS = -I.. -I../src/include

all: $(EXEC)

$(EXEC): $(SRCS) ac_feature_demo.h
	ccx $(INCS) $(SRCS) -o $(EXEC)

clean:
	delete $(EXEC)
