# AC 4.0 - Amiga self-host build (GNU Make 3.81+ / 4.x)
#
# Run from THIS directory (github/TCC/src/ac/src):
#
#   smake -f smakefile all     build bootstrap ./ac with SAS/C
#
# Gen-1 (SAS/C ac compiles sources � use until ac-self codegen is trusted):
#
#   make bootstrap-ac          ac -> bootstrap/ac/*.s
#   make assemble-ac           a68k -> bootstrap/ac/*.o
#   make link-ac               Blink -> ac-self
#   make selfhost-ac           bootstrap-ac + assemble-ac + link-ac
#
# Gen-2 (ac-self compiles sources � regression test / next self-host step):
#
#   Sync updated .c files to Amiga, then ALWAYS in this order:
#
#     smake -f smakefile all     rebuild ./ac (SAS/C) from sources
#     make selfhost-ac            ./ac -> bootstrap/ac -> ac-self
#     make clean-self
#     make selfhost-self          ac-self -> bootstrap/self -> ac-self2
#
#   Skipping smake/selfhost-ac leaves stale ac-self; dumpobj then shows
#   "103 items for hunk 2099356" and ac-self2 is not executable.
#
#   make bootstrap-self        ac-self -> bootstrap/self/*.s
#   make assemble-self         a68k -> bootstrap/self/*.o
#   make link-self             Blink -> ac-self2
#   make selfhost-self         full gen-2 pipeline
#
#   make report-ac             list bootstrap/ac/*.s
#   make report-self           list bootstrap/self/*.s
#
# Demo regression suite (/demo):
#
#   make demo-self2 / demo-run-self2   DejaGnu ac_tests + compile smokes
#   Run:  demo-out/ac-self2/ac_tests
#   make demo-ac / demo-self / demo
#   make clean-demo
#
# Toolchain (github/TCC/SDK):
#
#   C runtime = cclib.library (shared) + AC-native CRT in crt/ -- NOT ac.lib.
#   First:  make cclib-crt     (needs ./ac from smake; A68k; Blink)
#   Install LIBS:cclib.library (copy from SDK/cclib.library/SDK/Libs/).
#
#   ../../../SDK/C/A68k        assembler (AC .s)
#   ../../../SDK/C/Blink       linker (AC CRT: no LinkerDB / near data)
#   ../../../SDK/lib/amiga.lib Amiga stubs
#   ../../../SDK/lib/math.lib  AC float helpers (.FDadd etc.)
#
# Capture linker diagnostics:  make link-ac >build.log

ACC         = ac
ACSELF_CC   = ac-self
# Local AC headers first; NDK (-Iinclude:) last so stdlib.h is not the
# NDK/SAS one.  install_bootstrap_includes() uses the same order.
ACFLAGS     = -Iinclude -Iinclude:
# Bootstrap compiles: -q suppresses per-function local-table dumps.
BOOTFLAGS   = $(ACFLAGS) -q

OUTDIR_AC   = bootstrap/ac
OUTDIR_SELF = bootstrap/self

BIN_AC      = ac-self
BIN_SELF    = ac-self2

# dos.library smoke test built by existing ac-self2 (see hello_dos.c).
HELLO_SRC   = hello_dos.c
HELLO_S     = hello_dos.s
HELLO_O     = hello_dos.o
HELLO_BIN   = hello_dos
HELLO_WITH  = hello_dos.with
HELLO_LOG   = hello_dos.blink.log
HELLO_CLOG  = hello_dos.compile.log

# github/TCC/SDK from github/TCC/src/ac/src
SDKC        = ///SDK/C
SDKLIB      = ///SDK/lib
CCLIB_ROOT  = ///SDK/cclib.library
CCLIB_BIN   = $(CCLIB_ROOT)/SDK/Libs/cclib.library

A68K        = $(SDKC)/A68k
LINKER      ?= $(SDKC)/Blink

# AC-native CRT (cclib SetupSTDIO contract). ac_start.o must be first.
CRT_DIR     = crt
CRT_START   = $(CRT_DIR)/ac_start.o
CRT_OBJS    = \
	$(CRT_START) \
	$(CRT_DIR)/ac_crt.o \
	$(CRT_DIR)/ac_lvos.o

LIBS        = $(SDKLIB)/amiga.lib $(SDKLIB)/math.lib

# Same modules as smakefile (actual source spellings on disk).
CSOURCES = \
	Analyze.c \
	Builtins.c \
	CGlbDef.c \
	Cmain.c \
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
	feof.c

# Object link order from MakeWith.PDC plus GetOpt.o (smakefile LIBF).
LINKMODULES = \
	Analyze \
	Expr \
	Init \
	Optimize \
	Register \
	Force \
	Func \
	IntExpr \
	OutCode \
	SearchKW \
	GenCode \
	GenFloat \
	List \
	PeepGen \
	Stmt \
	Cmain \
	GenStmt \
	MemMgt \
	PreComp \
	PreProc \
	Decl \
	GetSym \
	Symbol \
	Builtins \
	CGlbDef \
	GetOpt \
	feof

ASSEMBLY_AC   = $(CSOURCES:%.c=$(OUTDIR_AC)/%.s)
OBJECTS_AC     = $(LINKMODULES:%=$(OUTDIR_AC)/%.o)
LINKWITH_AC    = $(OUTDIR_AC)/link.with
LINKLOG_AC     = $(OUTDIR_AC)/link.log

ASSEMBLY_SELF  = $(CSOURCES:%.c=$(OUTDIR_SELF)/%.s)
OBJECTS_SELF   = $(LINKMODULES:%=$(OUTDIR_SELF)/%.o)
LINKWITH_SELF  = $(OUTDIR_SELF)/link.with
LINKLOG_SELF   = $(OUTDIR_SELF)/link.log

.PHONY: all help \
	bootstrap bootstrap-ac bootstrap-self \
	assemble assemble-ac assemble-self \
	force-assemble-ac force-assemble-self \
	link link-ac link-self \
	selfhost selfhost-ac selfhost-self \
	cclib-crt \
	report report-ac report-self \
	hello-dos \
	demo demo-ac demo-self demo-self2 \
	demo-run-ac demo-run-self demo-run-self2 \
	demo-cli demo-cli-ac demo-cli-self demo-cli-self2 demo-cli-body \
	demo-compile-ac demo-compile-self demo-compile-self2 \
	demo-fail-ac demo-fail-self demo-fail-self2 clean-demo \
	clean clean-ac clean-self clean-hello-dos clean-cclib-crt

all: bootstrap-ac

help:
	@echo "Amiga ac self-host (separate bootstrap trees):"
	@echo ""
	@echo "  C library: cclib.library (dynamic) + AC CRT in $(CRT_DIR)/"
	@echo "    make cclib-crt           build AC CRT (ac + A68k); needs ./ac"
	@echo "    copy $(CCLIB_BIN) to LIBS:cclib.library"
	@echo ""
	@echo "  SAS/C ac (gen-1) -> $(BIN_AC):"
	@echo "    make bootstrap-ac    $(ACC) -> $(OUTDIR_AC)/*.s"
	@echo "    make assemble-ac     A68k -> $(OUTDIR_AC)/*.o"
	@echo "    make link-ac         $(LINKER) -> $(BIN_AC)"
	@echo "    make selfhost-ac     bootstrap + assemble + link"
	@echo "    make report-ac       list $(OUTDIR_AC)/*.s"
	@echo ""
	@echo "  ac-self (gen-2) -> $(BIN_SELF):"
	@echo "    smake -f smakefile all   then: make selfhost-self"
	@echo "    make selfhost-ac         REQUIRED after smake (relinks ac-self)"
	@echo "    make bootstrap-self      $(ACSELF_CC) -> $(OUTDIR_SELF)/*.s"
	@echo "    make assemble-self       A68k -> $(OUTDIR_SELF)/*.o"
	@echo "    make link-self           $(LINKER) -> $(BIN_SELF)"
	@echo "    make selfhost-self       selfhost-ac + gen-2 (recommended)"
	@echo "    make report-self         list $(OUTDIR_SELF)/*.s"
	@echo ""
	@echo "  Smoke test (existing ac-self2 only — does not rebuild the compiler):"
	@echo "    make hello-dos           build $(HELLO_BIN); log: $(HELLO_CLOG)"
	@echo "    Verbose: -a annotate asm, -l listing; -q quiet (no -v flag)"
	@echo ""
	@echo "  Demo regression (DejaGnu/GCC .sum output via ac_tests):"
	@echo "    make demo-self2          compile smokes + link ac_tests"
	@echo "    make demo-run-self2       build and run the suite"
	@echo "    make demo-cli-self2       exercise AC command-line options"
	@echo "    Run:  demo-out/ac-self2/ac_tests"
	@echo "    make demo-ac / demo-self / demo"
	@echo "    make demo-fail-self2      expect static_assert reject"
	@echo "    make clean-demo"
	@echo ""
	@echo "  make clean             remove both trees and linked binaries"
	@echo ""
	@echo "SDK tools:  $(A68K)  $(LINKER)"
	@echo "SAS/C ac:   smake -f smakefile all"
	@echo "cclib CRT:  $(CRT_DIR)/  (make cclib-crt)"

# Default aliases use the SAS/C ac path (safe bootstrap).
bootstrap: bootstrap-ac
assemble: assemble-ac
link: link-ac
selfhost: selfhost-ac
report: report-ac

bootstrap-ac: $(ASSEMBLY_AC)
	@echo "--- $(ACC) bootstrap assembly complete -> $(OUTDIR_AC) ---"

bootstrap-self: selfhost-ac $(ASSEMBLY_SELF)
	@echo "--- $(ACSELF_CC) bootstrap assembly complete -> $(OUTDIR_SELF) ---"

$(OUTDIR_AC)/%.s: %.c ./ac
	-makedir $(OUTDIR_AC)
	-delete $(OUTDIR_AC)/$*.o QUIET
	$(ACC) $(BOOTFLAGS) -c -o $@ $<

$(OUTDIR_SELF)/%.s: %.c $(BIN_AC)
	-makedir $(OUTDIR_SELF)
	-delete $(OUTDIR_SELF)/$*.o QUIET
	$(ACSELF_CC) $(BOOTFLAGS) -c -o $@ $<

# AmigaDOS date-only stamps can leave .o newer than regenerated .s; wipe before A68k.
force-assemble-ac:
	-delete $(OUTDIR_AC)/#?.o ALL QUIET

force-assemble-self:
	-delete $(OUTDIR_SELF)/#?.o ALL QUIET

assemble-ac: force-assemble-ac $(OBJECTS_AC)
	@echo "--- $(OUTDIR_AC) objects assembled ---"

assemble-self: force-assemble-self $(OBJECTS_SELF)
	@echo "--- $(OUTDIR_SELF) objects assembled ---"

$(OUTDIR_AC)/%.o: $(OUTDIR_AC)/%.s
	$(A68K) -o$@ $<

$(OUTDIR_SELF)/%.o: $(OUTDIR_SELF)/%.s
	$(A68K) -o$@ $<

link-ac: $(BIN_AC)

link-self: $(BIN_SELF)

# --- AC-native CRT for cclib.library (crt/) ---
# Needs ./ac (smake) to compile ac_crt.c; A68k for .s; Blink to link apps.
# Runtime: LIBS:cclib.library
cclib-crt: $(CRT_OBJS)
	@echo "--- AC cclib CRT ready in $(CRT_DIR)/ ---"
	@echo "Install shared lib: copy $(CCLIB_BIN) LIBS:cclib.library"

$(CRT_DIR)/ac_start.o: $(CRT_DIR)/ac_start.s
	$(A68K) -o$@ $(CRT_DIR)/ac_start.s

$(CRT_DIR)/ac_lvos.o: $(CRT_DIR)/ac_lvos.s
	$(A68K) -o$@ $(CRT_DIR)/ac_lvos.s

# Compile+assemble in one rule. Do not delete ac_crt.o from a separate
# ac_crt.s recipe: Amiga make caches "file exists" and then skips A68k
# (Blink Error 426: Cannot find object crt/ac_crt.o).
$(CRT_DIR)/ac_crt.o: $(CRT_DIR)/ac_crt.c ./ac
	$(ACC) $(ACFLAGS) -c -o $(CRT_DIR)/ac_crt.s $(CRT_DIR)/ac_crt.c
	$(A68K) -o$@ $(CRT_DIR)/ac_crt.s

# Blink with-file (ac_start.o first).
# Quote "to ..." lines: bare `echo to file` is AmigaDOS Echo TO-redirect,
# so the Blink TO line never lands in the with-file (rc 140 / empty log).
$(LINKWITH_AC): $(CRT_OBJS) $(OBJECTS_AC)
	-delete $(LINKWITH_AC) QUIET
	-delete $(LINKLOG_AC) QUIET
	@echo from $(CRT_START) + >$(LINKWITH_AC)
	@echo $(CRT_DIR)/ac_crt.o $(CRT_DIR)/ac_lvos.o + >>$(LINKWITH_AC)
	@echo $(wordlist 1,8,$(OBJECTS_AC))+ >>$(LINKWITH_AC)
	@echo $(wordlist 9,16,$(OBJECTS_AC))+ >>$(LINKWITH_AC)
	@echo $(wordlist 17,24,$(OBJECTS_AC))+ >>$(LINKWITH_AC)
	@echo $(wordlist 25,999,$(OBJECTS_AC)) >>$(LINKWITH_AC)
	@echo library $(word 1,$(LIBS)) + $(word 2,$(LIBS)) >>$(LINKWITH_AC)
	@echo "to $(BIN_AC)" >>$(LINKWITH_AC)
	@echo ver $(LINKLOG_AC) >>$(LINKWITH_AC)

$(LINKWITH_SELF): $(CRT_OBJS) $(OBJECTS_SELF)
	-delete $(LINKWITH_SELF) QUIET
	-delete $(LINKLOG_SELF) QUIET
	@echo from $(CRT_START) + >$(LINKWITH_SELF)
	@echo $(CRT_DIR)/ac_crt.o $(CRT_DIR)/ac_lvos.o + >>$(LINKWITH_SELF)
	@echo $(wordlist 1,8,$(OBJECTS_SELF))+ >>$(LINKWITH_SELF)
	@echo $(wordlist 9,16,$(OBJECTS_SELF))+ >>$(LINKWITH_SELF)
	@echo $(wordlist 17,24,$(OBJECTS_SELF))+ >>$(LINKWITH_SELF)
	@echo $(wordlist 25,999,$(OBJECTS_SELF)) >>$(LINKWITH_SELF)
	@echo library $(word 1,$(LIBS)) + $(word 2,$(LIBS)) >>$(LINKWITH_SELF)
	@echo "to $(BIN_SELF)" >>$(LINKWITH_SELF)
	@echo ver $(LINKLOG_SELF) >>$(LINKWITH_SELF)

$(BIN_AC): $(LINKWITH_AC)
	$(LINKER) with $(LINKWITH_AC)
	@echo "--- link log ($(BIN_AC)) ---"
	- type $(LINKLOG_AC)

$(BIN_SELF): $(LINKWITH_SELF)
	$(LINKER) with $(LINKWITH_SELF)
	@echo "--- link log ($(BIN_SELF)) ---"
	- type $(LINKLOG_SELF)

selfhost-ac: cclib-crt bootstrap-ac assemble-ac link-ac
	@echo "--- $(BIN_AC) linked with cclib CRT ---"

selfhost-self: cclib-crt bootstrap-self assemble-self link-self
	@echo "--- $(BIN_SELF) linked with cclib CRT ---"
	@echo "Verify offsets first: bootstrap/self/Cmain.s must use 12(A0) not (A0)"

# --- ac-self2 smoke test (minimal first; does not rebuild ac-self2) ---
# After compiler fixes: smake && make selfhost-ac && make clean-self && make selfhost-self
# then: make hello-dos
#
# Phony target name differs from $(HELLO_BIN) so make does not see two recipes
# for the same target (was: hello-dos: hello-dos).

hello-dos: $(HELLO_BIN)
	@echo "--- built $(HELLO_BIN); compile log $(HELLO_CLOG); run: $(HELLO_BIN) ---"

# No dependency on $(BIN_SELF) link recipes — invoke the binary by name only.
# Do not redirect compiler stdout into the log (Amiga shell >> can confuse stdio).
# Compile+assemble together (same make-cache trap as crt/ac_crt.o).
$(HELLO_O): $(HELLO_SRC)
	-delete $(HELLO_CLOG) QUIET
	@echo "hello-dos: invoking $(BIN_SELF) $(ACFLAGS) -c -o $(HELLO_S) $(HELLO_SRC)" >$(HELLO_CLOG)
	$(BIN_SELF) $(ACFLAGS) -c -o $(HELLO_S) $(HELLO_SRC)
	@echo "hello-dos: compile finished" >>$(HELLO_CLOG)
	@echo "--- compile log ($(HELLO_CLOG)) ---"
	- type $(HELLO_CLOG)
	$(A68K) -o$(HELLO_O) $(HELLO_S)

$(HELLO_WITH): $(CRT_OBJS) $(HELLO_O)
	-delete $(HELLO_WITH) QUIET
	-delete $(HELLO_LOG) QUIET
	@echo from $(CRT_START) + >$(HELLO_WITH)
	@echo $(CRT_DIR)/ac_crt.o $(CRT_DIR)/ac_lvos.o $(HELLO_O) >>$(HELLO_WITH)
	@echo library $(word 1,$(LIBS)) >>$(HELLO_WITH)
	@echo "to $(HELLO_BIN)" >>$(HELLO_WITH)
	@echo ver $(HELLO_LOG) >>$(HELLO_WITH)

$(HELLO_BIN): $(HELLO_WITH)
	$(LINKER) with $(HELLO_WITH)
	@echo "--- link log ($(HELLO_BIN)) ---"
	- type $(HELLO_LOG)

# --- demo regression suite ---
# Sources: /demo (Amiga parent).  Output: demo-out/<compiler>/
#
#   make demo-self2     compile-check each test_*.c + link ac_tests (DejaGnu)
#   demo-out/ac-self2/ac_tests
#
# ac_tests prints GCC/DejaGnu-style PASS/FAIL + Summary (compiler .sum format).
DEMO_DIR       = /demo
DEMO_OUT_AC    = demo-out/ac
DEMO_OUT_SELF  = demo-out/ac-self
DEMO_OUT_SELF2 = demo-out/ac-self2

# Compile-only language smokes (must produce .s; not linked).
DEMO_COMPILE = \
	test_align \
	test_binlit \
	test_bss_arrays \
	test_ac_debug_brace \
	test_param_addr \
	test_cclib_syntax \
	test_static_assert \
	test_types_suffixes

# Runnable DejaGnu suite.
DEMO_SUITE = ac_tests

DEMO_ASM_AC    = $(DEMO_COMPILE:%=$(DEMO_OUT_AC)/%.s)
DEMO_ASM_SELF  = $(DEMO_COMPILE:%=$(DEMO_OUT_SELF)/%.s)
DEMO_ASM_SELF2 = $(DEMO_COMPILE:%=$(DEMO_OUT_SELF2)/%.s)

$(DEMO_OUT_AC)/%.s: $(DEMO_DIR)/%.c
	-makedir demo-out
	-makedir $(DEMO_OUT_AC)
	@echo "  $(ACC) $<"
	$(ACC) $(ACFLAGS) -q -c -o $@ $<

$(DEMO_OUT_SELF)/%.s: $(DEMO_DIR)/%.c
	-makedir demo-out
	-makedir $(DEMO_OUT_SELF)
	@echo "  $(ACSELF_CC) $<"
	$(ACSELF_CC) $(ACFLAGS) -q -c -o $@ $<

$(DEMO_OUT_SELF2)/%.s: $(DEMO_DIR)/%.c
	-makedir demo-out
	-makedir $(DEMO_OUT_SELF2)
	@echo "  $(BIN_SELF) $<"
	$(BIN_SELF) $(ACFLAGS) -q -c -o $@ $<

$(DEMO_OUT_AC)/$(DEMO_SUITE).o: $(DEMO_OUT_AC)/$(DEMO_SUITE).s
	$(A68K) -o$@ $<

$(DEMO_OUT_SELF)/$(DEMO_SUITE).o: $(DEMO_OUT_SELF)/$(DEMO_SUITE).s
	$(A68K) -o$@ $<

$(DEMO_OUT_SELF2)/$(DEMO_SUITE).o: $(DEMO_OUT_SELF2)/$(DEMO_SUITE).s
	$(A68K) -o$@ $<

$(DEMO_OUT_AC)/$(DEMO_SUITE): $(DEMO_OUT_AC)/$(DEMO_SUITE).o $(CRT_OBJS)
	-delete $@.with QUIET
	-delete $@.blink.log QUIET
	@echo from $(CRT_START) + >$@.with
	@echo $(CRT_DIR)/ac_crt.o $(CRT_DIR)/ac_lvos.o $< >>$@.with
	@echo library $(word 1,$(LIBS)) >>$@.with
	@echo "to $@" >>$@.with
	@echo ver $@.blink.log >>$@.with
	$(LINKER) with $@.with
	@echo "--- link log ($@) ---"
	- type $@.blink.log

$(DEMO_OUT_SELF)/$(DEMO_SUITE): $(DEMO_OUT_SELF)/$(DEMO_SUITE).o $(CRT_OBJS)
	-delete $@.with QUIET
	-delete $@.blink.log QUIET
	@echo from $(CRT_START) + >$@.with
	@echo $(CRT_DIR)/ac_crt.o $(CRT_DIR)/ac_lvos.o $< >>$@.with
	@echo library $(word 1,$(LIBS)) >>$@.with
	@echo "to $@" >>$@.with
	@echo ver $@.blink.log >>$@.with
	$(LINKER) with $@.with
	@echo "--- link log ($@) ---"
	- type $@.blink.log

$(DEMO_OUT_SELF2)/$(DEMO_SUITE): $(DEMO_OUT_SELF2)/$(DEMO_SUITE).o $(CRT_OBJS)
	-delete $@.with QUIET
	-delete $@.blink.log QUIET
	@echo from $(CRT_START) + >$@.with
	@echo $(CRT_DIR)/ac_crt.o $(CRT_DIR)/ac_lvos.o $< >>$@.with
	@echo library $(word 1,$(LIBS)) >>$@.with
	@echo "to $@" >>$@.with
	@echo ver $@.blink.log >>$@.with
	$(LINKER) with $@.with
	@echo "--- link log ($@) ---"
	- type $@.blink.log

demo-ac: cclib-crt $(DEMO_ASM_AC) $(DEMO_OUT_AC)/$(DEMO_SUITE) demo-cli-ac
	@echo "--- demo-ac OK; run $(DEMO_OUT_AC)/$(DEMO_SUITE) ---"

demo-self: cclib-crt $(DEMO_ASM_SELF) $(DEMO_OUT_SELF)/$(DEMO_SUITE) demo-cli-self
	@echo "--- demo-self OK; run $(DEMO_OUT_SELF)/$(DEMO_SUITE) ---"

demo-self2: cclib-crt $(DEMO_ASM_SELF2) $(DEMO_OUT_SELF2)/$(DEMO_SUITE) demo-cli-self2
	@echo "--- demo-self2 OK; run $(DEMO_OUT_SELF2)/$(DEMO_SUITE) ---"

demo: demo-ac demo-self demo-self2
	@echo "--- demo suite built for $(ACC), $(ACSELF_CC), $(BIN_SELF) ---"

demo-run-ac: demo-ac
	$(DEMO_OUT_AC)/$(DEMO_SUITE)

demo-run-self: demo-self
	$(DEMO_OUT_SELF)/$(DEMO_SUITE)

demo-run-self2: demo-self2
	$(DEMO_OUT_SELF2)/$(DEMO_SUITE)

# --- AC command-line option smoke (see /demo/cli_opts.txt) ---
CLI_PROBE   = $(DEMO_DIR)/cli_probe.c
CLI_MIN     = $(DEMO_DIR)/cli_min.c
CLI_INCDIR  = $(DEMO_DIR)/cli_inc

demo-cli-ac: CLI_CC=$(ACC)
demo-cli-ac: CLI_DIR=$(DEMO_OUT_AC)/cli
demo-cli-ac: demo-cli-body

demo-cli-self: CLI_CC=$(ACSELF_CC)
demo-cli-self: CLI_DIR=$(DEMO_OUT_SELF)/cli
demo-cli-self: demo-cli-body

demo-cli-self2: CLI_CC=$(BIN_SELF)
demo-cli-self2: CLI_DIR=$(DEMO_OUT_SELF2)/cli
demo-cli-self2: demo-cli-body

demo-cli: demo-cli-ac demo-cli-self demo-cli-self2
	@echo "--- demo-cli OK for $(ACC), $(ACSELF_CC), $(BIN_SELF) ---"

demo-cli-body:
	@echo "--- CLI option smoke with $(CLI_CC) -> $(CLI_DIR) ---"
	-makedir demo-out
	-makedir $(CLI_DIR)
	@echo "  -c -o -q -D -I"
	$(CLI_CC) -q -c -o $(CLI_DIR)/probe.s -DCLI_MUST_BE_SET=42 -I$(CLI_INCDIR) $(CLI_PROBE)
	@echo "  -D/-U"
	$(CLI_CC) -q -c -o $(CLI_DIR)/probe_u.s -DCLI_MUST_BE_SET=42 -DCLI_MUST_BE_UNDEF=1 -UCLI_MUST_BE_UNDEF -I$(CLI_INCDIR) $(CLI_PROBE)
	@echo "  -a -b -g -l -r -S (on cli_probe)"
	$(CLI_CC) -q -c -a -b -g -l -r -S -o $(CLI_DIR)/probe_flags.s -DCLI_MUST_BE_SET=42 -I$(CLI_INCDIR) $(CLI_PROBE)
	@echo "  -n (optimize off; uses cli_min — #if compares break under -n)"
	$(CLI_CC) -q -c -n -o $(CLI_DIR)/min_n.s $(CLI_MIN)
	@echo "  -f gcc|sasc|pdc and -f 5"
	$(CLI_CC) -q -c -f gcc -o $(CLI_DIR)/probe_fgcc.s -DCLI_MUST_BE_SET=42 -I$(CLI_INCDIR) $(CLI_PROBE)
	$(CLI_CC) -q -c -f sasc -o $(CLI_DIR)/probe_fsasc.s -DCLI_MUST_BE_SET=42 -I$(CLI_INCDIR) $(CLI_PROBE)
	$(CLI_CC) -q -c -f pdc -o $(CLI_DIR)/probe_fpdc.s -DCLI_MUST_BE_SET=42 -I$(CLI_INCDIR) $(CLI_PROBE)
	$(CLI_CC) -q -c -f 5 -o $(CLI_DIR)/probe_fp.s -DCLI_MUST_BE_SET=42 -I$(CLI_INCDIR) $(CLI_PROBE)
	@echo "  -W column|no-column|error|no-error"
	$(CLI_CC) -q -c -W column -o $(CLI_DIR)/probe_wcol.s -DCLI_MUST_BE_SET=42 -I$(CLI_INCDIR) $(CLI_PROBE)
	$(CLI_CC) -q -c -W no-column -o $(CLI_DIR)/probe_wnocol.s -DCLI_MUST_BE_SET=42 -I$(CLI_INCDIR) $(CLI_PROBE)
	$(CLI_CC) -q -c -W error -o $(CLI_DIR)/probe_werr.s -DCLI_MUST_BE_SET=42 -I$(CLI_INCDIR) $(CLI_PROBE)
	$(CLI_CC) -q -c -W no-error -o $(CLI_DIR)/probe_wnoerr.s -DCLI_MUST_BE_SET=42 -I$(CLI_INCDIR) $(CLI_PROBE)
	@echo "  -E (preprocess-only flag accepted)"
	$(CLI_CC) -q -E -c -o $(CLI_DIR)/probe_E.s -DCLI_MUST_BE_SET=42 -I$(CLI_INCDIR) $(CLI_PROBE)
	@echo "  -P 0 (precomp enable)"
	$(CLI_CC) -q -c -P 0 -o $(CLI_DIR)/probe_P.s -DCLI_MUST_BE_SET=42 -I$(CLI_INCDIR) $(CLI_PROBE)
	@echo "  -L (not implemented message)"
	$(CLI_CC) -q -c -L ignored -o $(CLI_DIR)/probe_L.s -DCLI_MUST_BE_SET=42 -I$(CLI_INCDIR) $(CLI_PROBE)
	@echo "  -s (strip toggle; documented unimplemented)"
	$(CLI_CC) -q -c -s -o $(CLI_DIR)/probe_s.s -DCLI_MUST_BE_SET=42 -I$(CLI_INCDIR) $(CLI_PROBE)
	@echo "  -? help (exits 1)"
	-$(CLI_CC) -?
	@echo "--- CLI option smoke OK ($(CLI_CC)) ---"

demo-compile-ac: $(DEMO_ASM_AC)
	@echo "--- demo-compile-ac OK ---"

demo-compile-self: $(DEMO_ASM_SELF)
	@echo "--- demo-compile-self OK ---"

demo-compile-self2: $(DEMO_ASM_SELF2)
	@echo "--- demo-compile-self2 OK ---"

demo-fail-ac:
	-makedir demo-out
	-makedir $(DEMO_OUT_AC)
	-delete $(DEMO_OUT_AC)/test_static_assert_fail.s QUIET
	-$(ACC) $(ACFLAGS) -q -c -o $(DEMO_OUT_AC)/test_static_assert_fail.s $(DEMO_DIR)/test_static_assert_fail.c
	@echo "--- expect-fail $(ACC): confirm static_assert error above ---"

demo-fail-self:
	-makedir demo-out
	-makedir $(DEMO_OUT_SELF)
	-delete $(DEMO_OUT_SELF)/test_static_assert_fail.s QUIET
	-$(ACSELF_CC) $(ACFLAGS) -q -c -o $(DEMO_OUT_SELF)/test_static_assert_fail.s $(DEMO_DIR)/test_static_assert_fail.c
	@echo "--- expect-fail $(ACSELF_CC): confirm static_assert error above ---"

demo-fail-self2:
	-makedir demo-out
	-makedir $(DEMO_OUT_SELF2)
	-delete $(DEMO_OUT_SELF2)/test_static_assert_fail.s QUIET
	-$(BIN_SELF) $(ACFLAGS) -q -c -o $(DEMO_OUT_SELF2)/test_static_assert_fail.s $(DEMO_DIR)/test_static_assert_fail.c
	@echo "--- expect-fail $(BIN_SELF): confirm static_assert error above ---"

clean-demo:
	-delete demo-out ALL QUIET

report-ac: bootstrap-ac
	@echo "--- $(ACC) bootstrap report ($(OUTDIR_AC)) ---"
	-list $(OUTDIR_AC)/#?.s
	@echo "--- expected $(words $(CSOURCES)) modules ---"

report-self: bootstrap-self
	@echo "--- $(ACSELF_CC) bootstrap report ($(OUTDIR_SELF)) ---"
	-list $(OUTDIR_SELF)/#?.s
	@echo "--- expected $(words $(CSOURCES)) modules ---"

clean-hello-dos:
	-delete $(HELLO_S) QUIET
	-delete $(HELLO_O) QUIET
	-delete $(HELLO_WITH) QUIET
	-delete $(HELLO_LOG) QUIET
	-delete $(HELLO_CLOG) QUIET
	-delete $(HELLO_BIN) QUIET
	-delete hello_dos.lis QUIET

clean-ac:
	-delete $(OUTDIR_AC)/#?.s ALL QUIET
	-delete $(OUTDIR_AC)/#?.o ALL QUIET
	-delete $(LINKWITH_AC) QUIET
	-delete $(LINKLOG_AC) QUIET
	-delete $(BIN_AC) QUIET

clean-self:
	-delete $(OUTDIR_SELF)/#?.s ALL QUIET
	-delete $(OUTDIR_SELF)/#?.o ALL QUIET
	-delete $(LINKWITH_SELF) QUIET
	-delete $(LINKLOG_SELF) QUIET
	-delete $(BIN_SELF) QUIET

clean-cclib-crt:
	-delete $(CRT_DIR)/ac_start.o QUIET
	-delete $(CRT_DIR)/ac_crt.o QUIET
	-delete $(CRT_DIR)/ac_crt.s QUIET
	-delete $(CRT_DIR)/ac_lvos.o QUIET

clean: clean-ac clean-self clean-hello-dos clean-cclib-crt clean-demo
