# AC 4.0 TODO — Roadmap

## MVP (active focus): SAS/C ANSI C language parity

**Goal:** Match SAS/C 6.x **ANSI C** (ISO C89/C90) language behavior as documented in
[SASC6V1.txt](../../../../SASC6V1.txt) Appendix 3 (F.3) and the compiler’s
`ansi` / `nooldpp` defaults — **not** K&R or pre-ANSI compatibility modes, and
**not** Chapter 11 Amiga extensions.

**Dialect target (SAS/C equivalent):** `nooldpp` + `nocommentnest` + `notrigraph`
(trigraphs optional later). Prefer prototypes; accept C89 old-style definitions
only as required by the standard, not as a product focus.

**Sources:** SASC6V1 Appendix 3 (~p.267 / ~line 13676); Ch.11 lists *non*-ANSI
extensions (~p.153 / ~line 8117) — those are **post-MVP**.

### MVP in scope

| Area | SAS/C reference | AC status / work |
|------|-----------------|------------------|
| C89 types, decls, stmts, exprs | ANSI + F.3.3–F.3.12 | Largely done; see checklist |
| `const` / `volatile` | F.3.10 | Done |
| Prototypes + prototype checking | V6 expects prototypes | Present (`ERR_PROTO`); deepen diagnostics later |
| ANSI preprocessor `#` / `##` / `defined` / `#if` | `nooldpp` | Done — heap token PP (`PpToken.c`); GetSym text pushback |
| `#include ""` vs `<>` search | F.3.13 | Done (SAS order) |
| Bitfields (MSB-first, no cross-unit) | F.3.9 | Done |
| Struct alignment (non-`char` even) | F.3.9 | Done (`AL_*` = 2; pad test) |
| Plain `char` signed; sizes; IEEE float | F.3.4–F.3.6 | Done / documented |
| Signed `>>` / unsigned `>>` / rem sign | F.3.5 | Done |
| Enums as `int` (4 bytes) | F.3.9 | Done |
| `stdarg.h` / varargs ABI | C89 | Done (BE slots) |
| Implementation-defined doc | Appendix 3 | Done — [IMPLEMENTATION_DEFINED.md](IMPLEMENTATION_DEFINED.md) |

### MVP checklist (ordered)

1. [x] **Audit C89 completeness** against unittest + Appendix 3
2. [ ] **Preprocessor correctness** — `#` / `##` / argument prescan; token-based PP (§1.1.2a) still open (defer; not blocking doc/gate)
3. [x] **`#include` search order** — SAS F.3.13
4. [x] **Bitfields vs F.3.9** — MSB-first + layout test
5. [x] **Integer shifts / rem** — ASR / LSR + tests
6. [x] **`stdarg.h`** — BE slot-aware
7. [x] **Document** AC F.3.1–F.3.13 — [IMPLEMENTATION_DEFINED.md](IMPLEMENTATION_DEFINED.md)
8. [x] **Enum size** — `sizeof(enum)` = 4 (was 2)
9. [ ] **Gate:** Amiga `make demo-self2` + run `unittest-out/ac-self2/ac_tests` (confirm new tests; self-host)

**Still open after C89 language pass:** richer prototype diagnostics; Amiga gate run. Then **C99 round-out**.

### MVP out of scope (parked)

- **Pre-ANSI / K&R:** `oldpp` (comment paste, string formal substitution), multi-character constants as a feature, “Aztec-era” option mapping
- **Non-ANSI comments:** nested `/* */` (`commentnest`), `//` under `ansi` (SAS warns; we already have `//` as C99 — keep but not MVP gate)
- **Trigraphs:** off by default; optional `-f trigraph` later
- **Chapter 11 Amiga extensions:** `__chip`/`__far`/`__asm`/`__saveds`/…, `#pragma libcall`, unnamed unions, implicit member refs, structure equivalence, zero-length arrays, sized enums, `sizeof`/`,` in `#if`, national chars in identifiers, common-model linkage — **post-MVP** (many already partially parsed)
- **C99 / C23:** `long long` runtime polish, designated init, VLAs, `_Generic`, etc. — **post-MVP** (keep what already works; do not block MVP)
- **Console diagnostic redesign** (`[AC E n] …`) — **parked** (see §1.5.0 / §1.5.1a); fix only blockers like `(null)` if they impede MVP testing
- **Full libc / F.3.14–F.3.15** — library & locale, not language MVP

#### Audit findings (2026-08-28, from `out.txt` + sources)

**Already green (C89 runtime in `ac_tests`):** enums, structs/unions, bitfield RMW, switch/goto/loops, compound assigns, pointers/func ptrs, casts, string concat/escapes/sizeof, initializers, file/func `static`, K&R call + old-style def, `stdarg` int sum, `#`/`##` value checks, sizes (`char`1/`short`2/`int`4/ptr4), param array→pointer `sizeof`, unary `+`, fp call sugar.

**Only FAILs in last suite:** `c99/ll/*` arithmetic (post-MVP; leave on hold).

**Remediated:** include order, bitfield MSB packing, `stdarg.h` BE ABI, signed/unsigned `>>` codegen + tests, F.3 doc, enum `sizeof` = 4, struct even-pad test.

**Still open for MVP:** richer prototype diagnostics (later); Amiga gate run (`demo-self2` + new tests).

### Recent Improvements (Latest Update)
- ✅ **`#if` / NDK `types.h`** - rewrite `defined(X)` to 0/1 before `intexpr`; `intexpr` always `opt0`; no `L` on `__STDC_VERSION__`
- ✅ **Enum size = 4** - F.3.9 `int`-sized enumerations (`Decl.c`); `c89/enum/sizeof_int` + struct pad offset test
- ✅ **Enum deref = long** - `deref()` used `en_w_ref` for `bt_enum` while params/slots are 4 bytes; big-endian `needpunc` read 0 → NDK typedef Punctuation under ac-self (`Expr.c` / `Init.c` / `Force.c`)
- ✅ **F.3 language doc** - [IMPLEMENTATION_DEFINED.md](IMPLEMENTATION_DEFINED.md)
- ✅ **Quoted `#include "..."`** - SAS F.3.13 order: cwd, then directory of `curfile`, then `-I`, then `INCLUDE:`
- ✅ **Bitfields MSB-first** - F.3.9 left-to-right packing; `c89/bitfield/msb_word` layout check
- ✅ **Signed/unsigned `>>`** - ASR vs LSR via `signedflag`; rem sign-of-dividend tests
- ✅ **`stdarg.h` BE slots** - align `va_start`/`va_arg` to 4-byte stack slots
- ✅ **Array parameters → pointers** - K&R and prototype array params clear `val_flag` and set `size = 4` so `sizeof(a)` is pointer width (`c89/edge/sizeof_param_arr`)
- ✅ **`sizeof` string literals** - String literal types are `char[N]` with `N = len+1` (`Expr.c`)
- ✅ **Post-`*` qualifiers** - `int *const` / `restrict` after `*` parsed in declarators (`Decl.c`)
- ✅ **Unary `+`** - Accepted in expressions (`Expr.c`)
- ✅ **Function-pointer call sugar** - `fp(args)` when the callee is a pointer-to-function (`Expr.c`)
- ✅ **Variadic `#define` ellipsis** - `get_macro_param()` recognizes `...` (bare `(...)` and `(a, ...)` → `__VA_ARGS__`); `##` before `#`; trim spaces in `getparm`
- ✅ **SAS/C `__asm` / `__REG__` parse** - `__asm`, `__d0`…`__a6`, `register __dN`; `__aligned` as alignas(4)-style; call/define codegen still stack/`#pragma libcall` (see §1.5.1.2)
- ✅ **CRT `acdbg` off by default** - `AC_CRT_DEBUG 0` in `crt/ac_crt.c`; real errors still print
- ✅ **`selfhost-self` incremental** - Gen-2 `.s` no longer depends on `$(BIN_AC)` (avoids refreshing gen-1 then rebuilding all gen-2); `force-bootstrap-self` added
- ✅ **C89/C99 demo coverage** - Expanded `ac_tests` + compile-only `test_c89_*` / `test_c99_*` / `test_sasc_compiler_specific.c`; long long runtime arith still **on hold** (see §2.1.1)
- ✅ **Unit test layout** - Merged former `demo/` into `src/unittest/`; Amiga output under `unittest-out/`; `make demo-self2` still works
- ✅ **Amiga soft-float self-host (gen-0 → ac-self → ac-self2)** - Soft-float codegen and frame spills work through gen-2; `getfrac`/`getexp` emit `.Fl2d` / `.FD*` correctly
- ✅ **`make_autocon` always allocates** - Fixed `#if AC_DEBUG` dangling-`if` that skipped `xalloc` in framed functions and smashed `&call_library` via a stale A2 (empty `jsr` after first `.Fl2d`)
- ✅ **`#if AC_DEBUG` dangling-if audit** - Remaining sources use braced ifs or `#if` around diag only; rule documented in `C.h`; `unittest/test_ac_debug_brace.c` smoke
- ✅ **Global array BSS sizing** - `type_size()` recomputes count×elemsize when `tp->size` is a bare count; `typesize_mul` no longer returns count-only on poisoned elemsize; PreProc restored to real `FILE *inclfile[10]` etc.; `GetSym.c`/`Cmain.c` externs updated (no more `incl*_buf`); `unittest/test_bss_arrays.c` + `bss/sizeof_*` in `ac_tests`
- ✅ **char/short parameter addressing** - Keep declared type; BE 4-byte slot at +3/+2 in `Func.c`; `unittest/test_param_addr.c` + `param/&*` in `ac_tests`
- ✅ **Floating-point arithmetic assignment** - `gen_fsaincdec` stores updated float and returns old value for postfix `++`/`--`; `+=`/`-=`/`*=`/`/=` and prefix via assign+soft-float; `unittest/test_fp_assign.c` + `fp/*` in `ac_tests`
- ✅ **Stack frames larger than 32K** - Full 32-bit auto offsets (`icon_unpoison` instead of truncating `ICON16L`); `link #0` + `suba.l #N,A7` when `lc_auto > 32760`; deep locals via `make_frame_ref` (A-temp + adda); `unittest/test_large_frame.c` + `frame/large_touch_ends`
- ✅ **Soft-float results in fresh frame slots** - `float_result_mem()` parks D0:D1; `make_legal(F_FREG)` no longer reuses one `float_auto` cell
- ✅ **`link A5,#-N` frame size** - Placeholder immediates patched from final `lc_auto` (no empty `link A5,#` on float-return functions)
- ✅ **Double returns reload D0:D1** - `genreturn` reloads from memory when the result is not already in registers
- ✅ **Float const-fold restored** - `Optimize.c` `dooper`/`opt0` fold `v.f` again after soft-float ABI was fixed
- ✅ **Lexer avoids float literals in GetSym** - `getfrac`/`getexp` use casts only so ac-self does not re-enter float lexing while compiling itself
- ✅ **#pragma once** - Implemented single-include optimization
- ✅ **#warning directive** - Added non-standard warning directive
- ✅ **#error directive** - Enhanced error message formatting
- ✅ **#undef directive** - Verified proper functionality
- ✅ **#line directive** - Confirmed existing implementation
- ✅ **CHANGELOG.md** - Created comprehensive changelog
- ✅ **Variadic macros** - Implemented `__VA_ARGS__` support with `...` parameter
- ✅ **Stringification** - Added `#` operator support for macro parameters
- ✅ **Token pasting** - Added `##` operator support for macro tokens
- ✅ **Macro redefinition** - Allow redefinition of identical macros
- ✅ **Predefined macros** - Added `__STDC__` and `__STDC_VERSION__` macros
- ✅ **Type qualifiers** - Complete `const` and `volatile` support with proper type system integration
- ✅ **Integer promotion** - ANSI C compliant integer promotion rules for char/short types
- ✅ **Arithmetic conversions** - Proper usual arithmetic conversions following ANSI C standards
- ✅ **Array decay** - Arrays properly decay to pointers in expressions and function calls
- ✅ **Function pointers** - Enhanced function pointer type handling and assignments
- ✅ **libcall pragma format** - Updated to use SAS/C format for compatibility with existing pragma files
- ✅ **Complete SAS/C pragma support** - Added support for all SAS/C pragma types: flibcall, syscall, tagcall, and msg with full push/pop functionality
- ✅ **Amiga-specific defines** - Added `__AMIGA__` and `__amigaos__` predefined macros
- ✅ **SAS/C compiler define** - Added `__SASC` predefined macro for SAS/C compatibility
- ✅ **#undef directive** - Verified proper functionality for undefining macros
- ✅ **SAS/C keyword recognition** - Added all 10 SAS/C keywords to lexer for proper token recognition
- ✅ **Memory section attributes** - Added support for `__chip`, `__far`, `__near`, `__fast` memory placement keywords
- ✅ **Interrupt function attributes** - Added support for `__interrupt` keyword in function declarations
- ✅ **Deep nesting support** - Increased preprocessor nesting depth from 10 to 32 levels with bounds checking
- ✅ **Graceful macro redefinition** - Improved macro redefinition handling with value comparison and warnings
- ✅ **Long long type system** - Added C99 long long and unsigned long long support with complete type checking
- ✅ **64-bit constants** - Added support for LL/ll integer constant suffixes
- ✅ **Unsigned / long constant suffixes** - Accept `U`/`u`, `L`/`l`, and `UL`/`ul`/`LU`/`lu`/`ULL`/`LLU` on integer literals (C89/C99)
- ✅ **Floating constant suffixes** - Accept `f`/`F`/`l`/`L` via `getfloatsuffix`; `f`/`F` typed as float via `floatlits()` (integer IEEE d→s into the single pool, no runtime `.Fd2s`); `l`/`L` → long double≡double; `gen_fconvert` kept for real double→float casts
- ✅ **C99 `//` line comments** - GCC-compatible; also strip `//` from `#define` bodies; unterminated `/*` errors at EOF
- ✅ **SAS/C nested block comments** - opt-in via `-Wcommentnest` (default off so globs like `*.s` in comments still work)
- ✅ **Parsing `unsigned long int`** - Accept any-order type-specifier lists (`unsigned long int`, `long unsigned`, `unsigned long long int`, …); see §1.4.2
- ✅ **64-bit arithmetic operations** - Complete 68000 assembly code generation for long long operations
- ✅ **64-bit conversion operations** - Complete 68000 assembly code generation for long long conversions
- ✅ **Real 68000 assembly implementation** - Removed all TODO placeholders with proper assembly code
- ✅ **C99 _Bool support** - Added _Bool keyword and boolean type with complete type system integration
- ✅ **stdbool.h header** - Keyword-based `bool`/`true`/`false` (no conflicting `#define`s)
- ✅ **C23 type/constant aliases** - `bool`/`true`/`false`/`nullptr`, `intN_t`/`uintN_t`, `intptr_t`/`intmax_t`, no-op `restrict`/`inline`/`_Noreturn`
- ✅ **C23 freestanding headers** - `<stdint.h>`, `<iso646.h>`, `<stdnoreturn.h>` (plus `<stdbool.h>`)
- ✅ **long long on m68k** - L-value/assign, hi/lo storage, D0:D1 add/sub, 64-bit constant halves, full 32-bit immediates
- ✅ **POSIX host struct sizes** - `SZ_SYM`/`SZ_TYP`/`SZ_ENODE` use `sizeof` on Mac/Linux hosts
- ✅ **Complete pragma code generation** - All SAS/C pragma types now have full 68000 assembly code generation
- ✅ **flibcall code generation** - Added gen_flibcall() and gen_flibcall2() for floating-point library calls
- ✅ **syscall code generation** - Added gen_syscall() and gen_syscall2() for system function calls
- ✅ **Calling convention keywords** - Added __regargs and __stdargs keyword support with attribute storage
- ✅ **Pragma integration** - All pragma types integrated into function call flow with proper precedence

## Phase 1.5: SAS/C Amiga-Specific Features (POST-MVP)

> **Parked for MVP.** Chapter 11 of SASC6V1 — extensions *not* defined by ANSI.
> Resume after C89 language gate. Items already implemented stay checked.

### 1.5.0 Pragma Code Generation - COMPLETED

#### 1.5.0.1 Complete Pragma Implementation
- [x] **libcall code generation** - gen_libcall() and gen_libcall2() for library function calls
- [x] **flibcall code generation** - gen_flibcall() and gen_flibcall2() for floating-point library calls
- [x] **syscall code generation** - gen_syscall() and gen_syscall2() for system function calls
- [x] **tagcall code generation** - gen_tagcall() and gen_tagcall2() for tag-based function calls
- [x] **Function call integration** - All pragma types integrated into gen_fcall() with proper precedence
- [x] **68000 assembly generation** - Complete assembly code generation for all pragma types
- [x] **Register management** - Proper register saving/restoring for all pragma calls
- [x] **Stack management** - Correct stack cleanup for all pragma types
- [x] **Library integration** - Proper library calls with correct offsets and parameters

#### 1.5.0.2 Calling Convention Keywords - COMPLETED
- [x] **__regargs keyword** - Added keyword recognition and attribute storage
- [x] **__stdargs keyword** - Added keyword recognition and attribute storage
- [x] **Type qualifier integration** - Added QUAL_REGARGS and QUAL_STDARGS constants
- [x] **Declaration parsing** - Added support in Decl.c and PDC/Decl.c
- [x] **Test files** - Created comprehensive test files for calling conventions

### 1.5.1 Special Keywords

#### 1.5.1.0 Keyword Recognition Setup
- [x] **Add SAS/C keywords to lexer** - Add all SAS/C keywords to keyword recognition
  - **Keywords from compiler-specific.h**: `__asm`, `__stdargs`, `__saveds`, `__far`, `__chip`, `__fast`, `__interrupt`
  - **Keywords from SAS/C manual**: `__regargs`, `__near`, `__aligned` (not in compiler-specific.h)
  - **Implementation**: Add to `e_sym` enum in `C.h`, add recognition in `GetSym.c`
  - **Syntax**: All keywords start with `__` and end with `__` (except `__asm`, `__regargs`, `__stdargs`, `__saveds`)
  - **Files to modify**: `C.h` (add to enum), `GetSym.c` (add keyword recognition)

#### 1.5.1.1 Memory Management Keywords
- [x] **`__aligned`** - Force 4-byte alignment on data (SAS/C) — **PARSE COMPLETE**
  - **Implementation**: Keyword in `SearchKW.c` / `C.h`; `Decl.c` treats like `alignas(4)`
  - **Remaining**: dedicated function-prolog align codegen if SAS needs more than type align
  - **Files**: `SearchKW.c`, `Decl.c`, `C.h`

- [x] **`__chip`, `__far`, `__near`, `__fast`** - Memory section placement (SAS/C specific) - **BASIC ATTRIBUTE STORAGE COMPLETE**
  - **Implementation**: Add section attributes to symbol table, modify code generation
  - **`__chip`**: Place data in chip memory (lowest 512K-2M), generate 32-bit references
  - **`__far`**: Place in far data section, generate 32-bit references
  - **`__near`**: Place in near data section, generate 16-bit relative references via A4
  - **`__fast`**: Place data in fast memory (see exec/memory.h), generate 32-bit references
  - **Function calls**: `__near` forces 16-bit branch, `__far` forces 32-bit branch
  - **Syntax**: `__chip int data;` - matches `__CHIP__` from compiler-specific.h
  - **Syntax**: `__fast int data;` - matches `__FAST__` from compiler-specific.h
  - **Files to modify**: `Decl.c`, `GenCode.c`, `C.h`, `CGlbDef.c`, `GetSym.c` (add to keywords)
  - **Status**: Keywords recognized and attributes stored in type system

- [x] **`__interrupt`** - Interrupt routine support (SAS/C specific) - **BASIC ATTRIBUTE STORAGE COMPLETE**
  - **Implementation**: Disable stack checking, set condition codes on return
  - **Code generation**: Skip stack overflow checks, modify return sequence
  - **Syntax**: `__interrupt void handler()` - matches `__INTERRUPT__` from compiler-specific.h
  - **Files to modify**: `GenCode.c`, `Func.c`, `C.h`, `GetSym.c` (add to keywords)
  - **Status**: Keyword recognized and interrupt attribute stored in function qualifiers

#### 1.5.1.2 Parameter Passing Keywords
- [x] **`__asm`** - Specify register usage for parameters (SAS/C specific) — **PARSE COMPLETE**
  - **Implementation**: Accept `__asm` as declaration specifier and after `*` in
    declarators (`LONG (* __asm f)(...)`); accept `register __d0`…`__a6` from
    `compiler-specific.h` `__REG__(r,p)` → `register __##r p`.  Sets `QUAL_ASM`.
  - **Syntax**: matches Hyperion `clib/compiler-specific.h` under `__SASC`
  - **Remaining**: call/define codegen still uses stack (or `#pragma libcall`);
    register placement at call sites not yet driven from `__REG__` formals
  - **Files**: `Decl.c`, `SearchKW.c`, `C.h`, `Expr.c` (castbegin), demo test
  - **Demo**: `unittest/test_sasc_compiler_specific.c`

- [x] **`__regargs`** - Force register parameter passing (SAS/C specific)
  - **Implementation**: First 2 pointers in A0/A1, first 2 integers in D0/D1
  - **Floating-point**: First 2 doubles in FP0/FP1 (with math=68881)
  - **Linker symbol**: Add @ prefix to function name (replaces underscore)
  - **Syntax**: `__regargs int func(int a, char *b)` - matches `__STDARGS__` from compiler-specific.h
  - **Files to modify**: `Decl.c`, `GenCode.c`, `C.h`, `GetSym.c` (add to keywords)

- [x] **`__stdargs`** - Force stack parameter passing (SAS/C specific)
  - **Implementation**: Override register passing, force all parameters on stack
  - **Syntax**: `__stdargs int func(int a, char *b)` - matches `__STDARGS__` from compiler-specific.h
  - **Files to modify**: `Decl.c`, `GenCode.c`, `C.h`, `GetSym.c` (add to keywords)

#### 1.5.1.3 Function Control Keywords
- [ ] **`__saveds`** - Load global data register A4 (SAS/C specific)
  - **Implementation**: Generate `LEA _LinkerDB,A4` or `LEA _LinkerDB(A6),A4`
  - **Library code**: Use A6-relative addressing when compiled with libcode
  - **Syntax**: `__saveds int func()` - matches `__SAVE_DS__` from compiler-specific.h
  - **Files to modify**: `GenCode.c`, `Func.c`, `C.h`, `GetSym.c` (add to keywords)

- [ ] **`__inline`** - Function inlining support
  - **Implementation**: Mark functions for inlining, expand at call sites
  - **Global optimizer**: Only works with optimize option enabled
  - **Files to modify**: `Optimize.c`, `GenCode.c`, `C.h`

- [ ] **`__stackext`** - Stack extension support
  - **Implementation**: Check `__STKNEED`, allocate new stack if needed
  - **Stack management**: Compare available vs required stack space
  - **Files to modify**: `GenCode.c`, `Func.c`, `C.h`

### 1.5.2 Advanced Language Features

#### 1.5.2.1 Structure and Union Extensions
- [ ] **Unnamed unions** - Union members as structure members
  - **Implementation**: Allow union declarations without names in structures
  - **Member access**: Treat union members as direct structure members
  - **Syntax**: `struct { union { int x; double d; }; long l; } foo;`
  - **Files to modify**: `Decl.c`, `Expr.c`, `C.h`

- [ ] **Implicit structure references** - Direct member access without substructure name
  - **Implementation**: Allow `bar.x` instead of `bar.foo.x` when member names are unique
  - **Warning**: Generate warning 193 for implicit references
  - **Error handling**: Generate error 123 for ambiguous member names
  - **Files to modify**: `Expr.c`, `Decl.c`

- [ ] **Structure equivalence** - Pass equivalent structure types
  - **Implementation**: Compare structure layouts, allow compatible types
  - **Compatibility**: Longer structures can substitute shorter ones
  - **Files to modify**: `Decl.c`, `Expr.c`, add `StructureEquivalence` option

- [ ] **Zero-length arrays** - Variable-sized data structures
  - **Implementation**: Allow `long mem[0];` as last structure member
  - **Size calculation**: `sizeof` returns base structure size
  - **Allocation**: `malloc(sizeof(struct) + count * sizeof(type))`
  - **Files to modify**: `Decl.c`, `Expr.c`, `C.h`

#### 1.5.2.2 Type System Extensions
- [ ] **Enum size control** - Control enumerated type sizes
  - **Implementation**: Support `char enum`, `short enum`, `long enum`
  - **Syntax**: `char enum flintstones {Fred, Barney, Wilma};`
  - **Size**: 1 byte for char, 2 bytes for short, 4 bytes for long
  - **Files to modify**: `Decl.c`, `C.h`, `GetSym.c`

#### 1.5.2.3 Preprocessor Extensions
- [ ] **`sizeof` in `#if`** - Use sizeof operator in preprocessor directives
  - **Implementation**: Evaluate sizeof expressions in preprocessor
  - **Syntax**: `#if (sizeof(int) == 2)`
  - **Warning**: Generate warning with ansi/strict options
  - **Files to modify**: `PreProc.c`, `Expr.c`

- [ ] **Comma operator in `#if`** - Use comma operator in preprocessor
  - **Implementation**: Support comma operator in preprocessor expressions
  - **Syntax**: `#if (a, b)`
  - **Files to modify**: `PreProc.c`, `Expr.c`

- [x] **C++ style comments** - Support `//` comments (C99 / GCC); `GetSym` + `#define` body strip; unterminated block comment diagnosed at EOF
  - **Implementation**: Recognize `//` as comment start, extend to end of line
  - **Files**: `GetSym.c`, `PreProc.c`

- [x] **Nested comments** - SAS/C `COMMENTNEST` via `-Wcommentnest` (default off = C89 first-closer-wins)
  - **Implementation**: Depth counter when `comment_nesting`; default preserves `/*.s` globs in comments
  - **Files**: `GetSym.c`, `PreProc.c`, `Cmain.c`

- [ ] **National characters** - Accented characters in variable names
  - **Implementation**: Allow accented characters in identifiers
  - **Restriction**: Disabled with ansi option
  - **Files to modify**: `GetSym.c`, `C.h`

### 1.5.3 Stack Management

#### 1.5.3.1 Stack Control Variables
- [ ] **`__stack` variable** - Minimum stack size specification
  - **Implementation**: External long integer controlling minimum stack size
  - **Startup code**: Allocate new stack if default is smaller than `__stack`
  - **Files to modify**: `Init.c`, `Cmain.c`, add to predefined symbols

- [ ] **`__STKNEED` variable** - Minimum free stack per function
  - **Implementation**: External long integer specifying minimum free stack
  - **Stack extension**: Used by `__stackext` functions to check available space
  - **Default**: 400 bytes minimum
  - **Files to modify**: `Init.c`, `Cmain.c`, add to predefined symbols

#### 1.5.3.2 Stack Extension System
- [ ] **Stack extension code** - Dynamic stack allocation
  - **Implementation**: Check available stack, allocate new extent if needed
  - **Overhead**: Additional address register usage, slower execution
  - **Files to modify**: `GenCode.c`, `Func.c`

### 1.5.4 External Data Models

#### 1.5.4.1 Common Model Support
- [ ] **Common model external data** - Multiple external variable definitions
  - **Implementation**: Allow multiple definitions of external variables
  - **Linker**: Merge independent definitions into single symbol
  - **Option**: Controlled by `common` compiler option
  - **Files to modify**: `Decl.c`, `Cmain.c`, add option handling

#### 1.5.4.2 Strict Reference-Definition Model
- [ ] **Strict ref-def model** - Single definition, multiple declarations
  - **Implementation**: Require single definition, multiple extern declarations
  - **Default**: Current behavior (strict reference-definition)
  - **Files to modify**: `Decl.c`, `Cmain.c`

### 1.5.5 Preprocessor Deficits (compiler-specific.h compatibility)

#### 1.5.5.1 Complex Boolean Expressions in #if
- [ ] **Complex #if defined() expressions** - Support boolean logic in preprocessor
  - **Current limitation**: `#if defined(__SASC) || defined(__MAXON__) || defined(__STORM__)` fails
  - **Missing operators**: `&&` (logical AND), `||` (logical OR) in preprocessor expressions
  - **Implementation**: Extend `intexpr()` to support boolean operators
  - **Syntax**: `#if defined(A) && defined(B)`, `#if defined(A) || defined(B)`
  - **Files to modify**: `PreProc.c` (intexpr function), `Expr.c` (expression evaluation)

#### 1.5.5.2 Preprocessor Expression Evaluation
- [ ] **Enhanced preprocessor expressions** - Full ANSI C preprocessor expression support
  - **Current limitation**: Limited expression evaluation in `#if` directives
  - **Missing features**: Parentheses grouping, operator precedence, complex comparisons
  - **Implementation**: Implement full preprocessor expression parser
  - **Syntax**: `#if (defined(__GNUC__) && defined(AMIGA)) || defined(__SASC)`
  - **Files to modify**: `PreProc.c`, `Expr.c`

#### 1.5.5.3 Nested Conditional Compilation
- [x] **Deep nesting support** - Support multiple levels of conditional compilation
  - **Current limitation**: May hit nesting depth limits with complex headers
  - **Implementation**: Increase nesting depth limits, improve stack management
  - **Syntax**: `#ifndef A #ifdef B #if defined(C) #endif #endif #endif`
  - **Files to modify**: `PreProc.c` (nesting depth variables), `CGlbDef.c` (increase limits)
  - **Status**: Increased nesting depth from 10 to 32 levels with bounds checking

#### 1.5.5.4 Macro Redefinition Handling
- [x] **Graceful macro redefinition** - Handle macro redefinition without errors
  - **Current limitation**: May error on legitimate macro redefinitions
  - **Implementation**: Allow redefinition of identical macros, warn on different definitions
  - **Syntax**: Multiple `#define MACRO value` statements with same value
  - **Files to modify**: `PreProc.c` (dodefine function), improve redefinition logic
  - **Status**: Enhanced redefinition logic with value comparison and warnings

### 1.5.6 Implementation Priority

#### High Priority (Essential for Amiga Development)
1. ✅ **Keyword recognition setup** - Add all SAS/C keywords to lexer (foundation for everything) - **COMPLETED**
2. **Complex #if defined() expressions** - Critical for compiler-specific.h compatibility
3. ✅ **`__regargs` and `__stdargs`** - Critical for library compatibility - **COMPLETED**
4. **`__saveds`** - Required for callback functions and shared libraries
5. ✅ **`__chip`, `__far`, `__near`, `__fast`** - Essential for memory management - **BASIC ATTRIBUTE STORAGE COMPLETED**
6. **Stack management** - Required for large programs

#### Medium Priority (Important for Compatibility)
1. **Enhanced preprocessor expressions** - Full ANSI C preprocessor support
2. ✅ **`__interrupt`** - Needed for interrupt handlers - **BASIC ATTRIBUTE STORAGE COMPLETED**
3. **`__aligned`** - Important for performance
4. **`__asm`** - Advanced register specification
5. **Unnamed unions** - Common in Amiga system headers
6. **Structure equivalence** - Useful for type compatibility
7. ✅ **Deep nesting support** - Required for complex headers - **COMPLETED**

#### Low Priority (Nice to Have)
1. **`__inline`** - Performance optimization
2. **Zero-length arrays** - Advanced data structures
3. ✅ **Graceful macro redefinition** - Developer convenience - **COMPLETED**
4. **Preprocessor extensions** - Additional convenience features
5. **National characters** - Internationalization

### 1.5.6 Testing Strategy

#### 1.5.6.1 SAS/C Compatibility Testing
- [ ] **compiler-specific.h testing** - Test compilation of compiler-specific.h header
- [x] **Pragma file testing** - Test with existing SAS/C pragma files
- [ ] **Library header testing** - Compile Amiga system headers
- [ ] **Real-world programs** - Test with existing Amiga C programs

#### 1.5.6.2 Feature Testing
- [ ] **Keyword functionality** - Test each special keyword
- [ ] **Memory model testing** - Verify chip/far/near placement
- [ ] **Stack management** - Test stack extension and overflow handling

## Phase 1: C89/C90 (ANSI C) Compliance

### 1.1 Preprocessor Improvements

#### 1.1.1 Missing Preprocessor Directives
- [x] **#pragma once** - Implement single-include optimization
- [x] **#error** - Implement error directive for conditional compilation
- [x] **#warning** - Implement warning directive (non-standard but useful)
- [x] **#line** - Improve line directive handling for better debugging
- [x] **#undef** - Verified proper undefine functionality including predefined macros

#### 1.1.2 Macro System Enhancements
- [x] **Variadic macros** - Support for `__VA_ARGS__` in macro definitions
- [x] **Stringification** - Improve `#` operator handling in macros
- [x] **Token pasting** - Improve `##` operator handling in macros
- [x] **Macro redefinition** - Allow redefinition of identical macros
- [x] **Predefined macros** - Add missing standard macros:
  - [x] `__STDC__` - Standard C compliance indicator
  - [x] `__STDC_VERSION__` - C standard version
  - [x] `__DATE__` - Compilation date
  - [x] `__TIME__` - Compilation time
  - [x] `__FILE__` - Current source file
  - [x] `__LINE__` - Current line number

#### 1.1.2a Preprocessor architecture refactor
- [x] **Token-based preprocessor** - Heap PPT tokens in `PpToken.c`; `#if`/`#elif`/`defined` on tokens; `#` / `##` / placemarkers in `prepdefine`; stringify only at GetSym pushback (`$` sentinel). Nested disable-set / full rescan edge cases can still be hardened later.
  - **Files**: `PpToken.c` / `PpToken.h`, `PreProc.c`, Makefile / smakefile

#### 1.1.3 Include System
- [x] **Quoted includes** - `#include "file"`: cwd, then dir of `curfile`, then `-I`, then `INCLUDE:` (SAS F.3.13)
- [x] **System includes** - `#include <file>` skips cwd / beside-caller (`-I` then `INCLUDE:`)
- [ ] **NDK `pragmas/`** - ToolKit `os-include` has no `pragmas/`; `<proto/*.h>` under `__SASC` needs pragmas on the include path or `-D_NO_INLINE`
- [ ] **Include path management** - Improve `-I` option handling
- [ ] **Include guards** - Better support for include guard patterns

### 1.2 Language Features

#### 1.2.1 Type System
- [x] **Complete type qualifiers** - Ensure `const` and `volatile` are fully implemented
- [x] **Post-`*` qualifiers** - `int *const` / `restrict` after `*` in declarators
- [x] **Type promotion rules** - Fix integer promotion in expressions
- [x] **Usual arithmetic conversions** - Implement proper type conversion rules
- [x] **Function pointer types** - Improve function pointer handling; `fp(args)` call sugar
- [x] **Array decay** - Ensure arrays properly decay to pointers in expressions
- [x] **Array parameters** - Function params of array type become pointers (`sizeof` = 4)
- [x] **`sizeof` string literals** - Typed as `char[N]` including NUL

#### 1.2.2 Declarations and Definitions
- [x] **Function prototypes** - Parsing + basic `ERR_PROTO` arg checking (`Expr.c`); richer diagnostics later
- [x] **Variable argument lists** - `stdarg.h` BE 4-byte slots (`va_start` / `va_arg`)
- [ ] **External linkage** - Fix external symbol handling
- [x] **Static linkage** - File- and function-scope `static` covered in `ac_tests`
- [x] **Register storage class** - Accepted as a hint (F.3.8)

#### 1.2.3 Expressions and Operators
- [x] **Unary `+`** - Accepted in expressions
- [x] **Comma operator** - Present; covered by existing expression tests
- [x] **Conditional operator** - Present; covered by existing expression tests
- [x] **Assignment operators** - Compound assigns covered in `ac_tests`
- [x] **Increment/decrement** - Covered (incl. FP postfix assign fix)
- [x] **Bitwise operators** - Covered; signed/unsigned `>>` match F.3.5

#### 1.2.4 Statements
- [x] **Switch statements** - Covered in `ac_tests`
- [x] **Do-while loops** - Covered in `ac_tests`
- [x] **For loops** - Covered in `ac_tests`
- [x] **Goto statements** - Covered in `ac_tests`
- [x] **Compound statements** - Block scoping in use for self-host / tests

### 1.3 Standard Library Headers

#### 1.3.1 Core Headers
- [ ] **assert.h** - Implement assertion macros
- [ ] **ctype.h** - Character classification functions
- [ ] **errno.h** - Error number definitions
- [ ] **float.h** - Floating-point characteristics
- [ ] **limits.h** - Integer limits and characteristics
- [ ] **locale.h** - Locale-specific information
- [ ] **math.h** - Mathematical functions
- [ ] **setjmp.h** - Non-local jumps
- [ ] **signal.h** - Signal handling
- [x] **stdarg.h** - Variable argument lists (BE slot ABI)- [ ] **stddef.h** - Common definitions
- [ ] **stdio.h** - Input/output functions
- [ ] **stdlib.h** - General utilities
- [ ] **string.h** - String handling functions
- [ ] **time.h** - Time and date functions

#### 1.3.2 Library Implementation Issues
- [ ] **Buffer overflow protection** - Add bounds checking to string functions
- [ ] **Null pointer handling** - Ensure proper null pointer checks
- [ ] **Error handling** - Implement proper error reporting
- [ ] **Memory management** - Fix malloc/free implementation
- [ ] **File I/O** - Improve file handling and buffering

### 1.4 Code Generation

#### 1.4.1 Assembly Generation
- [ ] **Optimization improvements** - Better peephole optimization
- [ ] **Register allocation** - Improve register usage
- [x] **Stack management** - Soft-float/long-long frame spills and `link A5,#-N` sizing work on Amiga self-host; push-safe frameless omit still restricted
- [ ] **Function calls** - Ensure proper calling conventions
- [x] **Return value handling** - Double returns reload D0:D1 from memory when needed; long long already uses D0:D1

#### 1.4.2 Known Bug Fixes
- [x] **Function parameter addressing** - char/short params keep declared type; `Func.c` places them at +3/+2 in a 4-byte big-endian slot so `&param` works; float still widens to double for the 8-byte push
- [x] **Soft-float library codegen (self-host)** - Repeated `.Fl2d` / `.FD*` in one function (e.g. `GetSym` `getfrac`) emit correctly; root cause was `make_autocon` skipping `xalloc` when `AC_DEBUG` was off
- [x] **Floating-point arithmetic assignment** - float postfix `++`/`--` (`gen_fsaincdec`) now stores the updated value and returns the old one (soft-float `.FSadd`/`.FSsub`); compound assign and prefix already used `assign` + `fadd`/`fmul`/…; covered by `unittest/test_fp_assign.c` and `ac_tests` `fp/*`
- [x] **Stack frame limits** - Frames >32K use `link #0` + `suba.l #N,A7`; auto offsets keep full 32-bit values; `make_frame_ref` materializes deep locals outside `(d16,An)`
- [ ] **Buffer flushing** - Fix interactive file buffering
- [x] **Parsing `unsigned long int`** - C89 allows optional `int` after `unsigned`/`signed`/`short`/`long` (and combinations like `unsigned long int`). Decl accepts any-order type-specifier lists via `decl_int_specs()`.
- [x] **Global array BSS sizing** - `type_size()` recovers count×elemsize when `tp->size` looks like a bare count; `typesize_mul` uses 4-byte fallback instead of count-only; PreProc uses real pointer arrays again
- [x] **`#if AC_DEBUG` dangling-if audit** - Pattern documented in `C.h`; no remaining dangerous cases after `make_autocon` / `initstack` fixes

### 1.5 Error Handling and Diagnostics

> **Parked for MVP** (format redesign). Only fix MVP blockers (e.g. `(null)` text) if needed.

#### 1.5.0 Console output review (from Amiga `file.txt` / selfhost + unittest) — PARKED

Current AC-mode output mixes three styles and is hard to skim on a real console:

1. **Immediate “GCC-ish” line** — `file:line:col: error: (null) (Punctuation)`
2. **Legacy line block** — `Errors in line N of file:` + source line + `*** error 6 Punctuation`
3. **End summary** — `Compilation successful.` / `N errors, M warnings generated.` (skipped under `-q`)

Observations from a real gen-2 + unittest run:

- **`(null)` in messages** — `error(n, NULL)` prints `(null)` via `%s`; looks broken. Prefer errmsg-only or a short canned phrase.
- **Duplicate diagnostics** — every fault is printed twice (immediate + line block). Pick one primary form for AC mode.
- **Warning flood under `-q`** — each bootstrap `.c` still dumps many `macro redefinition with different type (Syntax)` lines from cclib `stdio.h` / `stdlib.h` / `ctype.h`. Quiet should mean quiet, or system-header noise should be once / suppressible.
- **Cascade spam** — one bad `__REG__(…)` produces many identical `Punctuation` / `Expression expected` lines; recover and stop stacking duplicates on the same column.
- **Category-only text** — `Punctuation`, `Undefined Symbol` without “expected …” / “near …” is thin for interactive Amiga use.
- **Banner vs make** — full version banner on every non-`-q` compile is fine interactively; Makefile should keep `-q` (or a single banner) so logs stay readable.
- **Paths** — `///SDK/...` and long absolute-ish include paths dominate the line; shorten for display when possible.
- **Column without caret** — column is reported but the line block has no `^` under the token (SAS/C-friendly and easy to read on 80-col consoles).

Goal for **AC mode** (`frontend_mode != FE_CC`, default `-f` / native): friendly Amiga console diagnostics — not GCC parity. Keep `-f gcc` / `cc` front-end for tools that want the colon form.

#### 1.5.1 Error Reporting
- [ ] **Error message improvements** - Better error descriptions
- [ ] **Warning system** - Implement warning levels
- [ ] **Source location** - Improve error location reporting
- [ ] **Error recovery** - Better error recovery in parser

#### 1.5.1a AC-mode console formatting (friendly / Amiga-native)
- [ ] **Single diagnostic stream in AC mode** - Do not emit both `file:line:col:` and the `Errors in line N` block for the same fault; choose one shape (prefer the line+caret block for AC, keep GCC shape for `-f gcc` / `cc`)
- [ ] **Never print `(null)`** - If `msg` is NULL, print only `errmsg[n]` (or a fixed “see message above”); never `%s` on a null pointer
- [ ] **Readable one-liner** - Prefer e.g. `Decl.c 120: error 6: Punctuation near '__'` or SAS-like `Decl.c(120) : Error 6: …` over `error: (null) (Punctuation)`
- [ ] **Source snippet + caret** - After the message, show the source line and a `^` (or `---^`) under the column; wrap long lines for 80-column Amiga consoles
- [ ] **Deduplicate cascades** - Cap identical error-code+column repeats on one line; after recovery, skip further reports until the next semicolon / declaration
- [ ] **Human errmsg text** - Expand terse codes (“Punctuation” → “unexpected token” / “expected ‘)’ or ‘,’”) where cheap; keep numeric code for manuals (`*** error 6`)
- [ ] **`-q` really quiet** - Under `-q`, suppress version banner and per-file success line; still print errors/warnings (or add `-qq` for errors-only)
- [ ] **System-header warning policy** - Downgrade or once-only macro-redefinition warnings from `-I` system paths (cclib / NDK); optional `-Wsystem` to re-enable; do not reprint the same warning on every TU in a make log
- [ ] **Path display** - Strip redundant `///` / show basename+assign when the full path wraps past ~60 chars (full path still OK in listing file)
- [ ] **End-of-file summary always useful** - Even with `-q`, if there were errors, print one line: `Analyze.c: 4 errors, 12 warnings` (omit “Compilation successful.” under `-q`)
- [ ] **Default AC format** - Make native AC default `-f pdc` or a new `-f ac` (line block + caret), not GCC columns, unless `cc` / `-f gcc`
- [ ] **Align warning/error vocabulary** - Same layout for warnings as errors (`Warning` vs `Error`, same caret rules)
- [ ] **Optional color** - Off by default on Amiga; if ever added, gate behind `-f color` and detect non-console

#### 1.5.2 Debugging Support
- [ ] **Debug information** - Improve DBX debug info generation
- [ ] **Symbol table** - Better symbol table management
- [ ] **Line number mapping** - Accurate line number tracking

## Phase 1.6: Long Long Implementation Completion

### 1.6.1 Long Long Work - COMPLETED
- ✅ **68000 assembly code generation** - Complete 64-bit arithmetic operations with real assembly
  - **Addition/Subtraction**: Complete gen_llbinary function with ADDX/SUBX for carry/borrow propagation
  - **Multiplication**: Complete gen_llmul function with MULS and cross-product handling
  - **Division/Modulo**: Complete gen_lldiv and gen_llmod functions with DIVS/MODS
  - **Conversion**: Complete gen_llextend function with EXT/CLR for sign/zero extension
  - **Reference**: Uses existing gen_deref function for 64-bit references
- ✅ **Arithmetic operations** - Complete implementation of en_lladd, en_llsub, en_llmul, en_lldiv, en_llmod
- ✅ **Conversion operations** - Complete implementation of en_cbll, en_cwll, en_clll, en_cull
- ✅ **Reference operations** - Complete implementation of en_ll_ref, en_ull_ref
- ✅ **Testing** - Created comprehensive test suite for 64-bit operations
- ✅ **Real assembly implementation** - Removed all TODO placeholders with proper 68000 assembly code

## Phase 2: C99 Compliance

### 2.1 New Language Features

#### 2.1.1 Type System Extensions
- [x] **long long** - 64-bit integer type on 32-bit m68k (hi/lo words, not a user-visible struct)
  - **Status**: Parsing, typing, assign/init, sizeof, and constants work; Amiga `ac_tests` `c99/ll/*` **runtime arith/suffix FAILs on hold**
  - **L-value fix**: `lvalue()` accepts `en_ll_ref` / `en_ull_ref` (auto-init and `=` work)
  - **Casts**: `asforcefit` allows long long → narrower integer types
  - **Storage / ABI**: 8-byte memory (big-endian hi then lo); args as two stack longs; return in D0:D1; CSE does not park values in a single D-reg
  - **Constants**: `ival` + `ival_hi` pair (C89-safe); `LL`/`ULL` nodes carry both halves; global init emits two `DC.l`
  - **Codegen**: `add.l`/`addx.l` (and sub/subx) on D0:D1; `putconst` emits full 32-bit immediates (no `ICON16L` truncate); `addx`/`subx` opcode names fixed
  - **On hold / still open**: long long mul/div/mod and remaining runtime arith; some narrow casts may still emit `move.f`
- [ ] **Complex types** - `_Complex` and `_Imaginary` types (or define `__STDC_NO_COMPLEX__`)
- [x] **Boolean type** - `_Bool` / `bool` keywords and type system
  - [x] **`stdbool.h` vs keywords** - header no longer `#define`s `true`/`false` (keywords provide them)
- [ ] **Flexible array members** - Variable-length array members in structs
- [ ] **Variable-length arrays** - Arrays with runtime-determined size

#### 2.1.2 New Keywords
- [x] **inline** - Accepted as no-op declaration qualifier
- [x] **restrict** - Accepted as no-op declaration qualifier
- [x] **_Bool** - Boolean type keyword (also C23 `bool`)

#### 2.1.3 Compound Literals
- [ ] **Array literals** - `(int[]){1, 2, 3}` syntax
- [ ] **Struct literals** - `(struct point){1, 2}` syntax

#### 2.1.4 Designated Initializers
- [ ] **Array designators** - `int a[10] = {[5] = 1, [7] = 2}`
- [ ] **Struct designators** - `struct s = {.x = 1, .y = 2}`

### 2.2 New Preprocessor Features

#### 2.2.1 Variadic Macros
- [x] **__VA_ARGS__** - Variable argument macros; `...` recognized in `#define` param lists
- [ ] **##__VA_ARGS__** - GNU/C23 empty-variadic paste (placemarker) still incomplete vs full token PP

#### 2.2.2 Pragmas
- [ ] **#pragma STDC** - Standard pragma support
- [x] **#pragma once** - Single-include optimization

### 2.3 New Standard Library Features

#### 2.3.1 New Headers
- [x] **stdbool.h** - Keyword-based `bool`/`true`/`false`; `__bool_true_false_are_defined` only
- [x] **stdint.h** - Fixed-width integer typedef aliases (freestanding)
- [ ] **inttypes.h** - Integer format conversion
- [ ] **complex.h** - Complex number support
- [ ] **tgmath.h** - Type-generic math functions
- [ ] **fenv.h** - Floating-point environment

#### 2.3.2 Enhanced Headers
- [ ] **math.h** - New mathematical functions
- [ ] **stdio.h** - Enhanced I/O functions
- [ ] **stdlib.h** - Additional utility functions
- [ ] **string.h** - New string functions

### 2.4 New Language Constructs

#### 2.4.1 Mixed Declarations
- [ ] **C99-style declarations** - Declarations anywhere in block
- [ ] **Loop variable scoping** - Variables declared in for loops

#### 2.4.2 New Operators
- [ ] **Compound literals** - Runtime object creation
- [ ] **Designated initializers** - Named field initialization

## Phase 3: Modern C Features (Future)

### 3.1 C11 Features (Optional)
- [ ] **Generic selections** - `_Generic` keyword
- [x] **No-return functions** - `_Noreturn` accepted as no-op qualifier; `<stdnoreturn.h>` provided
- [ ] **Anonymous structs/unions** - Unnamed struct members
- [ ] **Thread support** - `_Thread_local` / `thread_local` and `threads.h`
- [ ] **Bounds checking** - `_Bounds_safe` annotations

### 3.2 C17/C18 Features (Optional)
- [ ] **Attribute syntax** - `[[attribute]]` syntax

### 3.3 C23 alias work already done
- [x] **`bool` / `_Bool`** - built-in boolean type
- [x] **`true` / `false`** - keywords for integer constants 1 / 0 (header macros must not redefine them)
- [x] **`nullptr`** - null pointer constant (`void*` today; distinct `nullptr_t` still open)
- [x] **`intN_t` / `uintN_t`** - aliases of char/short/long/long long
- [x] **`intptr_t` / `uintptr_t` / `intmax_t` / `uintmax_t`** - aliases of long / unsigned long / long long
- [x] **`restrict` / `inline` / `_Noreturn`** - accepted as no-op declaration qualifiers
- [x] **`<stdint.h>` / `<iso646.h>` / `<stdnoreturn.h>` / `<stdbool.h>`** - freestanding headers (`stdbool.h` does not redefine keyword `true`/`false`)
- [x] **`<stdbool.h>` keyword clash** - fixed: no `#define true`/`false`
- [x] **iso646 operator keywords** - `and`/`or`/`not`/… as aliases of `&&`/`||`/`!`/…
- [x] **POSIX host struct sizes** - `SZ_SYM`/`SZ_TYP`/`SZ_ENODE` use `sizeof` on Mac/Linux; `xalloc` 8-byte-aligns on LP64
- [x] **`long long` on m68k** - hi/lo memory + D0:D1 ops; see §2.1.1

### 3.4 C23 / modern C still to do

Near-term product goal: compile a real SSH stack on Amiga (prefer **Dropbear**
as the first target — largely C89/C99-portable; **OpenSSH** as the stretch
target). `__STDC_NO_*` macros are temporary honesty, **not** feature completion.
Full C23 remains the long-term goal; order work by what SSH code actually uses.

#### SSH-facing priority (do these before exotic C23)
- [ ] **Solid `long long` compares/casts** - no more mistaken `.FDcmp` on `ull != 0`
- [ ] **`__func__`** - logging / assert paths
- [ ] **Flexible array members** - `T name[];` at end of struct (packet buffers)
- [ ] **Designated initializers** - `.field =` / `[index] =`
- [ ] **Compound literals** - `(type){...}` (OpenSSH ML-KEM / modern crypto)
- [ ] **Anonymous structs/unions** - unnamed members (very common in C trees)
- [ ] **Declarations after statements** - C99 mixed decls (OpenSSH sntrup / newer files)
- [ ] **VLAs** - needed for some OpenSSH crypto paths; Dropbear can disable those
- [ ] **Preprocessor robustness** - complex `#if` / `defined()` (configure output)
- [ ] **`u8` string prefixes** - treat as ordinary strings initially

OpenSSH *requires* C99 variadic macros (`__VA_ARGS__`) — already present.
Dropbear can build closer to C89 if post-quantum options are off.

#### Remaining type-system work (full C23; not “done” via NO_* macros)
- [ ] **`_BitInt(N)`** - bit-precise integers (widths 1..64 on m68k) (+ `wb`/`uwb` suffixes)
- [ ] **`typeof` / `typeof_unqual`** - type of expression / unqualified type
- [x] **`alignof` / `_Alignof`**, **`alignas` / `_Alignas`** - + `<stdalign.h>`; forced align stored in TYP `_pad_typ` (CNOP 2/4/8)
- [ ] **`auto` type inference** - deduce type from initializer
- [ ] **`constexpr`** - compile-time constant objects
- [ ] **`_Atomic`** - real atomics (today only `__STDC_NO_ATOMICS__=1`)
- [ ] **`_Complex` / `_Imaginary`** - real complex types (today only `__STDC_NO_COMPLEX__=1`)
- [ ] **`nullptr_t`** - distinct null-pointer type (today `nullptr` is `void*`)

#### Often grouped with C23/C99 types (not keywords alone)
- [ ] **VLAs / flexible array members**
- [ ] **Designated initializers** - `.field` / `[index]`
- [ ] **Compound literals** - `(type){...}`
- [ ] **Empty braced initialization** - `= {}` (C23)

#### Lexer / constants
- [x] **`U` / `L` / `UL` integer suffixes** - C89/C99 unsigned/long/long-long suffixes in `getnumber()` (`10U`, `10L`, `10UL` / `10LU`, `10ULL` / `10LLU`)
- [x] **`f` / `F` / `l` / `L` floating suffixes** - C99/C23 in `getfloatsuffix()`; `f`/`F` → `floatlits()` IEEE single pool (integer d→s, no literal `.Fd2s`); `l`/`L` → long double (= double on Amiga)
- [x] **Binary integer literals** - `0b` / `0B` prefixes in `getnumber()`
- [x] **Digit separators** - skip `'` inside numeric tokens (`1'000'000`)
- [ ] **`wb` / `uwb` suffixes** - `_BitInt` constant suffixes (needs `_BitInt` first)
- [ ] **UTF-8 / `u8` string prefixes** - treat `u8"..."` as ordinary strings initially; full Unicode later
- [x] **UTF-8 source tolerance** - warn once per line if UTF-8 (or BOM) is detected; skip multi-byte sequences in the lexer so Amiga Latin-1 builds are not aborted. Keep identifiers ASCII; Latin-1 OK in comments/strings. No UTF-8 identifiers / console Unicode (OS limitation)

#### Keywords / other
- [x] **`static_assert` / `_Static_assert`** - compile-time assert (const-expr [, message]); file and block scope
- [ ] **`thread_local` / `_Thread_local`** - real TLS (today only `__STDC_NO_THREADS__=1`)

#### Preprocessor / macros
- [ ] **`__func__`** - string of current function name (`lastfunc`)
- [ ] **`__has_include`** - include-existence probe
- [ ] **`__has_c_attribute` / `__has_embed`** - feature probes
- [ ] **`__VA_OPT__(...)`** - optional variadic macro tokens
- [ ] **`#embed`** - binary resource inclusion
- [ ] **Bump `__STDC_VERSION__`** toward `202311L` once a documented subset is claimed
- [x] **Feature-absence macros (interim)** - `__STDC_NO_ATOMICS__`, `__STDC_NO_THREADS__`, `__STDC_NO_COMPLEX__`, `__STDC_NO_VLA__` as `1` until real support lands

#### Larger language features
- [ ] **`_Generic`** - type-generic selection
- [ ] **Anonymous structs/unions** - unnamed members
- [ ] **Labels before `}`** - allow label at end of block

## Implementation Priority

### MVP (now) — ANSI C89 / SAS/C `ansi`+`nooldpp` language
1. ✅ C89 language vs F.3 (layout, shifts, stdarg, include order, enums) — **gate run remains**
2. Preprocessor token refactor (§1.1.2a) — deferred; string `#`/`##` accepted for gate
3. ✅ Integer/FP implementation-defined behavior (F.3.5–F.3.6) — documented + tested
4. ✅ Document AC F.3.1–F.3.13 — [IMPLEMENTATION_DEFINED.md](IMPLEMENTATION_DEFINED.md)
5. ✅ Soft-float self-host / frames / param addressing / FP assign — **COMPLETED**

### Post-MVP — Chapter 11 Amiga / C99 / product polish
1. SAS/C keywords codegen (`__asm` registers, `__saveds`, memory sections)
2. `#pragma` libcall path for NDK `proto/`
3. Console diagnostic format (`[AC E n]…` plan)
4. **C99 round-out** (after C89 Amiga gate): `long long` runtime, designated init, VLAs, …
5. Buffer flushing / libc completeness

### Historical High Priority (superseded by MVP list above)
1. ✅ **Function parameter addressing bug** - **COMPLETED**
2. ✅ **Soft-float self-host codegen** - **COMPLETED**
3. ✅ **Floating-point arithmetic assignment** - **COMPLETED**
4. ✅ **Stack frame limitations** - **COMPLETED**
5. **Buffer flushing** - post-MVP / CRT
6. **Complete preprocessor** - **MVP** (see above)
7. ✅ **SAS/C `__regargs`/`__stdargs` parse** - parse done; full codegen post-MVP
8. **SAS/C memory / `__saveds`** - **post-MVP**

## Testing Strategy

### 1. Compliance Testing
- [ ] **GCC test suite** - Use GCC's C compliance tests
- [ ] **Plum Hall test suite** - Commercial C compliance tests
- [ ] **Custom test cases** - Specific PDC bug regression tests

### 2. Performance Testing
- [ ] **Benchmark suite** - Compile and run performance tests
- [ ] **Memory usage** - Monitor compiler memory consumption
- [ ] **Compilation speed** - Measure compilation time improvements

### 3. Compatibility Testing
- [ ] **Real-world programs** - Test with existing C codebases
- [ ] **Cross-platform** - Ensure Amiga and Unix compatibility
- [ ] **Library integration** - Test with Amiga system libraries
- [x] **SAS/C compatibility** - Test with existing SAS/C pragma files and headers
- [ ] **Amiga system headers** - Compile official Amiga system include files
- [ ] **Legacy Amiga programs** - Test with existing Amiga C programs

## Documentation Updates

### 1. User Documentation
- [x] **Implementation-defined (F.3)** - [IMPLEMENTATION_DEFINED.md](IMPLEMENTATION_DEFINED.md)
- [ ] **Updated README** - Reflect new features and capabilities
- [ ] **User manual** - Comprehensive usage guide
- [ ] **Migration guide** - Help users upgrade from older versions

### 2. Developer Documentation
- [ ] **Internal documentation** - Code structure and design
- [ ] **API documentation** - Compiler internals for contributors
- [ ] **Build instructions** - Updated build and development setup

## Estimated Timeline

- **Phase 1 (C89/C90)**: 6-12 months
- **Phase 2 (C99)**: 12-18 months
- **Phase 3 (Modern C)**: 18+ months

## Success Criteria

### C89/C90 Compliance
- [ ] Passes 95% of GCC C89 compliance tests
- [ ] Compiles standard C library implementations
- [ ] No critical bugs in basic language features
- [ ] Complete standard library implementation
- [x] Full SAS/C pragma compatibility
- [ ] Compiles Amiga system headers without errors

### C99 Compliance
- [ ] Passes 90% of GCC C99 compliance tests
- [ ] Supports all C99 language features
- [ ] Complete C99 standard library
- [ ] Maintains Amiga compatibility
- [ ] Full SAS/C Amiga-specific feature support
- [ ] Complete Amiga development environment compatibility

---

*This TODO list is a living document and should be updated as progress is made and new issues are discovered.*
