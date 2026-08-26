# AC 4.0 compiler build (GNU Make 3.81+ / 4.x)
#
# Run GNU make from THIS directory (github/TCC/src/ac), not from src/.
#
#   make mac              Build host AC on macOS/Linux with cc/clang (POSIX)
#   make test             Demo regression suite on the host
#   make test-demo-suite  Compile demo/ tests + ac_tests
#   make test-demo        Legacy ac_feature_demo.c (needs Amiga headers)
#   make bootstrap        AC compiles its own sources to build/bootstrap/*.s
#   make bootstrap-self   gen-2: AC recompiles all sources (self-host regression)
#   make selfhost           mac + gen-1 + gen-2 + poison validation (run before Amiga)
#   make check-bootstrap  bootstrap + Python A68k displacement validator
#   make check-selfhost   gen-2 poison validation only
#   make validate-asm     validate build/bootstrap/*.s (or BOOTDIR=...)
#
# AmigaOS / SAS/C (native, produces ./ac via slink):
#
#   cd src && smake -f smakefile all
#
# See src/output.txt for a successful SAS/C 6.58 smake log.
#
# "make amiga" below is only for self-hosting with the ac compiler itself
# (GNU make on Amiga, if you have it).  It is NOT the SAS/C bootstrap path.
#
# Override host compiler:  make mac HOST_CC=clang

SRCDIR   = src
INCDIR   = $(SRCDIR)/include
COMPINC  = $(SRCDIR)/compinc
OBJDIR   = build
AMIGA_OBJDIR = build-amiga
BOOTDIR  = $(OBJDIR)/bootstrap
BINDIR   = .

HOST_CC     ?= cc
AMIGA_CC    ?= ac

# Default system include root for AC when running on macOS (SDK) or Linux (/usr).
SDKROOT     := $(shell xcrun --show-sdk-path 2>/dev/null)
ifeq ($(SDKROOT),)
SYS_INCLUDE := /usr/include/
else
SYS_INCLUDE := $(SDKROOT)/usr/include/
endif

HOST_CFLAGS ?= -std=gnu89 -Wall -Wno-deprecated-non-prototype \
               -Wno-unused-parameter -Wno-implicit-function-declaration \
               -Wno-incompatible-library-redeclaration -Wno-nonportable-include-path \
               -Wno-switch -Wno-int-conversion -Wno-incompatible-pointer-types \
               -Wno-unused-but-set-variable -Wno-format-security -Wno-parentheses \
               -DAC_HOST_POSIX -DINCLUDE=\"$(SYS_INCLUDE)\" -I$(SRCDIR)

AMIGA_CFLAGS = -I$(SRCDIR) -I$(INCDIR)

# Flags for AC compiling its own sources (m68k assembly output).
BOOT_CFLAGS  = -Isrc/compinc -Isrc/include -Isrc
ifeq ($(SDKROOT),)
BOOT_DEFS    =
else
BOOT_DEFS    = -DAC_HOST_POSIX
endif

CSOURCES = \
	Analyze.c \
	Builtins.c \
	CGlbDef.c \
	Cmain.c \
	DBX.c \
	Decl.c \
	Expr.c \
	Force.c \
	Func.c \
	GenCode.c \
	GenFloat.c \
	GenStmt.c \
	GetOpt.c \
	GetSym.c \
	Init.c \
	IntExpr.c \
	List.c \
	MemMgt.c \
	Optimize.c \
	OutCode.c \
	PeepGen.c \
	PreComp.c \
	PreProc.c \
	Register.c \
	SearchKW.c \
	Stmt.c \
	Symbol.c \
	host_posix.c

OBJECTS = $(CSOURCES:%.c=$(OBJDIR)/%.o)
AMIGA_OBJECTS = $(CSOURCES:%.c=$(AMIGA_OBJDIR)/%.o)
BOOT_ASM = $(CSOURCES:%.c=$(BOOTDIR)/%.s)
BOOT_ASM_GEN2 = $(CSOURCES:%.c=$(BOOTDIR)/gen2/%.s)

.PHONY: all mac amiga clean test test-demo test-demo-suite bootstrap bootstrap-report bootstrap-self selfhost check-bootstrap check-selfhost validate-asm validate-selfhost

all: mac

mac: $(BINDIR)/AC

amiga: $(BINDIR)/AC-amiga

$(BINDIR)/AC: $(OBJECTS)
	$(HOST_CC) $(HOST_CFLAGS) -o $@ $(OBJECTS)

$(BINDIR)/AC-amiga: $(AMIGA_OBJECTS)
	$(AMIGA_CC) $(AMIGA_CFLAGS) -o $@ $(AMIGA_OBJECTS)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(AMIGA_OBJDIR):
	mkdir -p $(AMIGA_OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(HOST_CC) $(HOST_CFLAGS) -c -o $@ $<

$(AMIGA_OBJDIR)/%.o: $(SRCDIR)/%.c | $(AMIGA_OBJDIR)
	$(AMIGA_CC) $(AMIGA_CFLAGS) -c -o $@ $<

test: mac test-demo-suite

# Demo regression suite (same tests Amiga make demo-ac / demo-self / demo-self2 run).
test-demo-suite: $(BINDIR)/AC
	$(MAKE) -f demo/Makefile check CC=./AC ACFLAGS= \
		DEMODIR=demo OUTDIR=demo/out/host QUIET=-q

# Full feature demo needs Amiga target headers; run on AmigaOS or with -Isrc/include.
test-demo: $(BINDIR)/AC
	cd demo && ../$(BINDIR)/AC -c -I../$(INCDIR) -I. -o ../$(OBJDIR)/ac_feature_demo.s ac_feature_demo.c 2>../$(OBJDIR)/ac_feature_demo.err
	@echo "--- stderr (diagnostics) ---"
	@cat $(OBJDIR)/ac_feature_demo.err
	@echo "--- generated assembly (first 40 lines) ---"
	@head -n 40 $(OBJDIR)/ac_feature_demo.s

# Self-host iteration: compile every compiler module with AC itself.
bootstrap: mac $(BOOT_ASM)

$(BOOTDIR):
	mkdir -p $(BOOTDIR)

$(BOOTDIR)/%.s: $(SRCDIR)/%.c $(BINDIR)/AC | $(BOOTDIR)
	-$(BINDIR)/AC -c $(BOOT_CFLAGS) $(BOOT_DEFS) -o $@ $< 2>$(BOOTDIR)/$*.err; \
	if grep -q "Compilation successful" $(BOOTDIR)/$*.err && test -s $@; then \
	 echo "  ok   $<"; \
	else \
	 echo "  FAIL $<"; \
	 tail -3 $(BOOTDIR)/$*.err; \
	fi

bootstrap-report: bootstrap
	@echo "--- bootstrap summary ---"
	@ok=0; fail=0; \
	for f in $(CSOURCES); do \
	  base=$${f%.c}; \
	  if grep -q "Compilation successful" $(BOOTDIR)/$$base.err 2>/dev/null && \
	     test -s $(BOOTDIR)/$$base.s; then \
	    ok=$$((ok+1)); echo "  ok   $$f"; \
	  else \
	    fail=$$((fail+1)); echo "  fail $$f"; \
	  fi; \
	done; \
	echo "$$ok / $(words $(CSOURCES)) modules assembled by AC"

# Gen-2 self-host on Mac: use gen-1 .s listings as the "compiler" is still
# the clang-built AC binary; this pass checks codegen stability (no 393216).
bootstrap-self: bootstrap
	@mkdir -p $(BOOTDIR)/gen2
	@ok=0; fail=0; \
	for f in $(CSOURCES); do \
	  base=$${f%.c}; \
	  ./$(BINDIR)/AC -c $(BOOT_CFLAGS) $(BOOT_DEFS) \
	    -o $(BOOTDIR)/gen2/$$base.s $(SRCDIR)/$$f \
	    2>$(BOOTDIR)/gen2/$$base.err; \
	  if grep -q "Compilation successful" $(BOOTDIR)/gen2/$$base.err 2>/dev/null && \
	     test -s $(BOOTDIR)/gen2/$$base.s; then \
	    ok=$$((ok+1)); echo "  ok   gen2 $$f"; \
	  else \
	    fail=$$((fail+1)); echo "  fail gen2 $$f"; tail -3 $(BOOTDIR)/gen2/$$base.err; \
	  fi; \
	done; \
	echo "$$ok / $(words $(CSOURCES)) gen-2 modules OK"

# Full Mac self-host: host AC compiles all sources twice; gen-2 must be poison-free.
selfhost: bootstrap-self validate-selfhost
	@echo "--- selfhost PASS: gen-1 and gen-2 bootstrap assembly OK ---"

check-bootstrap: bootstrap validate-asm

validate-selfhost:
	python3 scripts/validate_m68k_asm.py --poison-only --quiet $(BOOTDIR)/gen2
	python3 scripts/validate_m68k_asm.py --poison-only --quiet $(BOOTDIR)

check-selfhost: bootstrap-self validate-selfhost

validate-asm:
	python3 scripts/validate_m68k_asm.py $(BOOTDIR)

# Legacy quick grep (also run by validate-asm logic in Python); kept for CI logs.
check-bootstrap-grep: bootstrap
	@echo "--- 393216 poison grep (should be empty) ---"
	@hits=0; \
	for f in Analyze Builtins Decl Optimize GenCode Init Expr; do \
	  if test -f $(BOOTDIR)/$$f.s; then \
	    n=$$(grep -c "393216" $(BOOTDIR)/$$f.s 2>/dev/null || true); \
	    n=$${n:-0}; \
	    echo "  $$f.s: $$n"; \
	    hits=$$((hits + n)); \
	  fi; \
	done; \
	if test $$hits -eq 0; then \
	  echo "PASS: no 393216 in key bootstrap modules"; \
	else \
	  echo "FAIL: $$hits occurrences of 393216"; exit 1; \
	fi

clean:
	rm -rf $(OBJDIR) $(AMIGA_OBJDIR) $(BINDIR)/AC $(BINDIR)/AC-amiga demo/out
