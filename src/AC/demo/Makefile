# Mac / POSIX host only — Amiga uses src/Makefile (make demo-ac / demo-self / demo-self2).
#
# From github/TCC/src/ac:
#   make test-demo-suite
#   make -f demo/Makefile check CC=./AC ACFLAGS= DEMODIR=demo OUTDIR=demo/out/host
#   make -f demo/Makefile check-cli CC=./AC DEMODIR=demo OUTDIR=demo/out/host

CC       ?= ./AC
ACFLAGS  ?=
DEMODIR  ?= demo
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
	test_cclib_syntax \
	test_static_assert \
	test_types_suffixes \
	ac_tests

FAIL_SRC = $(DEMODIR)/test_static_assert_fail.c
FAIL_OUT = $(OUTDIR)/test_static_assert_fail.s
PASS_ASM = $(PASS_TESTS:%=$(OUTDIR)/%.s)
BSS_ASM = $(OUTDIR)/test_bss_arrays.s

.PHONY: all check check-pass check-fail check-cli check-bss dirs clean help

all: check

help:
	@echo "Host demo suite: make check / check-cli CC=./AC"
	@echo "Amiga: cd src && make demo-self2 / demo-cli-self2"

dirs:
	mkdir -p $(OUTDIR)

check: check-pass check-bss check-fail check-cli
	@echo "--- demo suite OK with $(CC) -> $(OUTDIR) ---"

check-pass: dirs $(PASS_ASM)
	@echo "--- $(CC): pass tests compiled ---"

# Confirm global arrays emit count*elemsize BSS, not bare count.
check-bss: $(BSS_ASM)
	@echo "  check DS.b sizes in test_bss_arrays.s"
	@grep -q 'DS\.b[[:space:]]*40' $(BSS_ASM) || (echo "FAIL: expected DS.b 40 for int[10]/ptr[10]"; exit 1)
	@grep -q 'DS\.b[[:space:]]*20' $(BSS_ASM) || (echo "FAIL: expected DS.b 20 for long[5]"; exit 1)
	@grep -q 'DS\.b[[:space:]]*16' $(BSS_ASM) || (echo "FAIL: expected DS.b 16 for short[8]/char[16]/intptr[4]"; exit 1)
	@echo "  ok test_bss_arrays DS.b sizes"

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

clean:
	rm -rf $(OUTDIR)
