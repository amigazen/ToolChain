# AC unit tests
#
# Host (Mac/Linux) — from github/TCC/src/ac:
#   make test
#   make -f src/unittest/Makefile check CC=./AC \
#     DEMODIR=src/unittest OUTDIR=src/unittest/out/host
#
# Amiga — from src/:
#   make demo-self2          (unittest-self2 alias)
#   unittest-out/ac-self2/ac_tests
#
# Classic Amiga ccx samples (append, hello, …): make classic

CC       ?= ./AC
ACFLAGS  ?=
DEMODIR  ?= .
OUTDIR   ?= $(DEMODIR)/out/host
QUIET    ?= -q
CLI_DIR  ?= $(OUTDIR)/cli
CLI_PROBE ?= $(DEMODIR)/cli_probe.c
CLI_MIN ?= $(DEMODIR)/cli_min.c
CLI_INCDIR ?= $(DEMODIR)/cli_inc

PASS_TESTS = \
	test_align \
	test_binlit \
	test_bss_arrays \
	test_ac_debug_brace \
	test_param_addr \
	test_fp_assign \
	test_large_frame \
	test_line_comments \
	test_cclib_syntax \
	test_static_assert \
	test_types_suffixes \
	test_c89_pp_cond \
	test_c89_stringize \
	test_c89_token_paste \
	test_c89_typedefs \
	test_c89_qualifiers \
	test_calling_conventions \
	test_sasc_pragmas \
	test_memory_sections \
	test_interrupt \
	test_c23_types \
	test_saveds \
	ac_tests

FAIL_SRC = $(DEMODIR)/test_static_assert_fail.c
FAIL_OUT = $(OUTDIR)/test_static_assert_fail.s
PASS_ASM = $(PASS_TESTS:%=$(OUTDIR)/%.s)
BSS_ASM = $(OUTDIR)/test_bss_arrays.s
FRAME_ASM = $(OUTDIR)/test_large_frame.s

CLASSIC_EXECS = append args date fahr hello null sizeofs Pi Savage Sieve

.PHONY: all check check-pass check-fail check-cli check-bss check-frame \
	dirs clean help classic classic-clean

all: check

help:
	@echo "Host suite: make check / check-cli CC=./AC"
	@echo "Amiga: cd src && make demo-self2 / unittest-self2"
	@echo "Classic Amiga ccx samples: make classic"

dirs:
	mkdir -p $(OUTDIR)

check: check-pass check-bss check-frame check-fail check-cli
	@echo "--- unittest OK with $(CC) -> $(OUTDIR) ---"

check-pass: dirs $(PASS_ASM)
	@echo "--- $(CC): pass tests compiled ---"

# Confirm global arrays emit count*elemsize BSS, not bare count.
check-bss: $(BSS_ASM)
	@echo "  check DS.b sizes in test_bss_arrays.s"
	@grep -q 'DS\.b[[:space:]]*40' $(BSS_ASM) || (echo "FAIL: expected DS.b 40 for int[10]/ptr[10]"; exit 1)
	@grep -q 'DS\.b[[:space:]]*20' $(BSS_ASM) || (echo "FAIL: expected DS.b 20 for long[5]"; exit 1)
	@grep -q 'DS\.b[[:space:]]*16' $(BSS_ASM) || (echo "FAIL: expected DS.b 16 for short[8]/char[16]/intptr[4]"; exit 1)
	@echo "  ok test_bss_arrays DS.b sizes"

# Frames >32K: link #0 plus suba.l #size,A7 (not a truncated link #d16).
check-frame: $(FRAME_ASM)
	@echo "  check large-frame prologue in test_large_frame.s"
	@grep -qi 'suba' $(FRAME_ASM) || (echo "FAIL: expected suba for frame >32K"; exit 1)
	@grep -Eiq 'link[[:space:]]+A[0-7],#0([^0-9]|$)' $(FRAME_ASM) \
		|| grep -Eiq 'link[[:space:]]+A[0-7],#\+?0([^0-9]|$)' $(FRAME_ASM) \
		|| (echo "FAIL: expected link An,#0 for frame >32K"; exit 1)
	@echo "  ok test_large_frame prologue"

$(OUTDIR)/%.s: $(DEMODIR)/%.c
	@echo "  $(CC) $<"
	$(CC) $(ACFLAGS) $(QUIET) -c -o $@ $<

check-fail: dirs
	@echo "  $(CC) expect-fail test_static_assert_fail"
	@rm -f $(FAIL_OUT)
	@if $(CC) $(ACFLAGS) $(QUIET) -c -o $(FAIL_OUT) $(FAIL_SRC); then \
	  echo "  FAIL test_static_assert_fail (should have been rejected)"; \
	  exit 1; \
	else \
	  echo "  ok test_static_assert_fail (rejected as expected)"; \
	fi

check-cli: dirs
	@echo "--- CLI option smoke with $(CC) ---"
	mkdir -p $(CLI_DIR)
	$(CC) $(QUIET) -c -o $(CLI_DIR)/probe.s -DCLI_MUST_BE_SET=42 -I$(CLI_INCDIR) $(CLI_PROBE)
	$(CC) $(QUIET) -c -o $(CLI_DIR)/probe_u.s -DCLI_MUST_BE_SET=42 -DCLI_MUST_BE_UNDEF=1 -UCLI_MUST_BE_UNDEF -I$(CLI_INCDIR) $(CLI_PROBE)
	$(CC) $(QUIET) -c -a -b -g -l -r -S -o $(CLI_DIR)/probe_flags.s -DCLI_MUST_BE_SET=42 -I$(CLI_INCDIR) $(CLI_PROBE)
	$(CC) $(QUIET) -c -n -o $(CLI_DIR)/min_n.s $(CLI_MIN)
	$(CC) $(QUIET) -c -f gcc -o $(CLI_DIR)/probe_fgcc.s -DCLI_MUST_BE_SET=42 -I$(CLI_INCDIR) $(CLI_PROBE)
	$(CC) $(QUIET) -c -f sasc -o $(CLI_DIR)/probe_fsasc.s -DCLI_MUST_BE_SET=42 -I$(CLI_INCDIR) $(CLI_PROBE)
	$(CC) $(QUIET) -c -f pdc -o $(CLI_DIR)/probe_fpdc.s -DCLI_MUST_BE_SET=42 -I$(CLI_INCDIR) $(CLI_PROBE)
	$(CC) $(QUIET) -c -f 5 -o $(CLI_DIR)/probe_fp.s -DCLI_MUST_BE_SET=42 -I$(CLI_INCDIR) $(CLI_PROBE)
	$(CC) $(QUIET) -c -W column -o $(CLI_DIR)/probe_wcol.s -DCLI_MUST_BE_SET=42 -I$(CLI_INCDIR) $(CLI_PROBE)
	$(CC) $(QUIET) -c -W no-column -o $(CLI_DIR)/probe_wnocol.s -DCLI_MUST_BE_SET=42 -I$(CLI_INCDIR) $(CLI_PROBE)
	$(CC) $(QUIET) -c -W error -o $(CLI_DIR)/probe_werr.s -DCLI_MUST_BE_SET=42 -I$(CLI_INCDIR) $(CLI_PROBE)
	$(CC) $(QUIET) -c -W no-error -o $(CLI_DIR)/probe_wnoerr.s -DCLI_MUST_BE_SET=42 -I$(CLI_INCDIR) $(CLI_PROBE)
	$(CC) $(QUIET) -E -c -o $(CLI_DIR)/probe_E.s -DCLI_MUST_BE_SET=42 -I$(CLI_INCDIR) $(CLI_PROBE)
	$(CC) $(QUIET) -c -P 0 -o $(CLI_DIR)/probe_P.s -DCLI_MUST_BE_SET=42 -I$(CLI_INCDIR) $(CLI_PROBE)
	$(CC) $(QUIET) -c -L ignored -o $(CLI_DIR)/probe_L.s -DCLI_MUST_BE_SET=42 -I$(CLI_INCDIR) $(CLI_PROBE)
	$(CC) $(QUIET) -c -s -o $(CLI_DIR)/probe_s.s -DCLI_MUST_BE_SET=42 -I$(CLI_INCDIR) $(CLI_PROBE)
	- $(CC) -?
	@echo "--- CLI option smoke OK ---"

# Legacy Amiga ccx sample programs (run on Amiga with ccx in PATH).
classic: $(CLASSIC_EXECS)

append: append.c
	ccx append.c

args: args.c
	ccx args.c

date: date.c
	ccx date.c

fahr: fahr.c
	ccx -lmath fahr.c

hello: hello.c
	ccx hello.c

null: null.c
	ccx null.c

sizeofs: sizeofs.c
	ccx sizeofs.c

Pi: Pi.c
	ccx -lmath Pi.c

Savage: Savage.c
	ccx -lmath Savage.c

Sieve: Sieve.c
	ccx Sieve.c

classic-clean:
	-delete (append|args|date|fahr|hello|null|sizeofs|Pi|Savage|Sieve)

clean:
	rm -rf $(OUTDIR)
	-rm -f args.o
