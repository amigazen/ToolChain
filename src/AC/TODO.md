# PDC Compiler TODO - ANSI C Compliance Roadmap

This document outlines the comprehensive roadmap for bringing the PDC (Publicly Distributable C) compiler to full ANSI C compliance, targeting C89/C90 first, then C99.

## Current Status

The PDC compiler (version 3.33) has basic ANSI C features but lacks full compliance with modern C standards. This TODO prioritizes C89/C90 compliance first, then C99 features.

### Recent Improvements (Latest Update)
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

## Phase 1.5: SAS/C Amiga-Specific Features

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
- [ ] **`__aligned`** - Force 4-byte alignment on data and functions
  - **Implementation**: Add keyword recognition in `GetSym.c`, modify `Decl.c` to handle alignment attributes
  - **Data alignment**: Update `alignment()` function to respect `__aligned` modifier
  - **Function alignment**: Modify code generation to align stack in function prolog
  - **Files to modify**: `GetSym.c`, `Decl.c`, `GenCode.c`, `C.h` (add to keyword enum)

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
- [ ] **`__asm`** - Specify register usage for parameters (SAS/C specific)
  - **Implementation**: Parse register specifications, modify calling convention
  - **Syntax**: `__asm void func(d0, d1, a0, a1)` - matches `__ASM__` from compiler-specific.h
  - **Register mapping**: Support D0-D7, A0-A7, FP0-FP7 for floating-point
  - **Files to modify**: `Decl.c`, `GenCode.c`, `C.h`, `GetSym.c` (add to keywords)

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

- [ ] **C++ style comments** - Support `//` comments
  - **Implementation**: Recognize `//` as comment start, extend to end of line
  - **Warning**: Generate warning with ansi/strict options
  - **Files to modify**: `PreProc.c`, `GetSym.c`

- [ ] **Nested comments** - Support `/* /* */ */` comments
  - **Implementation**: Track comment nesting level, allow nested comments
  - **Option**: Controlled by `commentnest` compiler option
  - **Files to modify**: `PreProc.c`, `GetSym.c`

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

#### 1.1.3 Include System
- [ ] **System includes** - Proper handling of `#include <file>` vs `#include "file"`
- [ ] **Include path management** - Improve `-I` option handling
- [ ] **Include guards** - Better support for include guard patterns

### 1.2 Language Features

#### 1.2.1 Type System
- [x] **Complete type qualifiers** - Ensure `const` and `volatile` are fully implemented
- [x] **Type promotion rules** - Fix integer promotion in expressions
- [x] **Usual arithmetic conversions** - Implement proper type conversion rules
- [x] **Function pointer types** - Improve function pointer handling
- [x] **Array decay** - Ensure arrays properly decay to pointers in expressions

#### 1.2.2 Declarations and Definitions
- [ ] **Function prototypes** - Ensure complete prototype support
- [ ] **Variable argument lists** - Implement `stdarg.h` support
- [ ] **External linkage** - Fix external symbol handling
- [ ] **Static linkage** - Ensure proper static variable handling
- [ ] **Register storage class** - Implement register keyword (hint only)

#### 1.2.3 Expressions and Operators
- [ ] **Comma operator** - Ensure proper precedence and evaluation
- [ ] **Conditional operator** - Fix `?:` operator implementation
- [ ] **Assignment operators** - Ensure all compound assignments work correctly
- [ ] **Increment/decrement** - Fix `++` and `--` operator issues
- [ ] **Bitwise operators** - Ensure proper bitwise operation handling

#### 1.2.4 Statements
- [ ] **Switch statements** - Improve switch case handling
- [ ] **Do-while loops** - Ensure proper do-while implementation
- [ ] **For loops** - Fix for loop scoping and initialization
- [ ] **Goto statements** - Ensure proper label and goto handling
- [ ] **Compound statements** - Fix block scoping issues

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
- [ ] **stdarg.h** - Variable argument lists
- [ ] **stddef.h** - Common definitions
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
- [ ] **Stack management** - Fix stack frame handling
- [ ] **Function calls** - Ensure proper calling conventions
- [ ] **Return value handling** - Fix return value passing

#### 1.4.2 Known Bug Fixes
- [ ] **Function parameter addressing** - Fix address-of parameter bug
- [ ] **Floating-point operations** - Fix arithmetic assignment operators
- [ ] **Stack frame limits** - Fix 32K stack frame limitation
- [ ] **Buffer flushing** - Fix interactive file buffering
- [x] **Parsing `unsigned long int`** - C89 allows optional `int` after `unsigned`/`signed`/`short`/`long` (and combinations like `unsigned long int`). Decl accepts any-order type-specifier lists via `decl_int_specs()`.
- [ ] **Global array BSS sizing** - `T *foo[10]` / `int foo[10]` can emit `DS.b 10` under ac-self when element size is poisoned `(e_sc<<16)|n` and `typesize_mul` falls back to the count. Include-path tables use `char foo_buf[40]` instead. Still fix `typesize_mul` / TYP.size poison for general arrays.

### 1.5 Error Handling and Diagnostics

#### 1.5.1 Error Reporting
- [ ] **Error message improvements** - Better error descriptions
- [ ] **Warning system** - Implement warning levels
- [ ] **Source location** - Improve error location reporting
- [ ] **Error recovery** - Better error recovery in parser

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
  - **Status**: Parsing, typing, assign/init, add/sub, constants, and Mac-host smoke tests pass
  - **L-value fix**: `lvalue()` accepts `en_ll_ref` / `en_ull_ref` (auto-init and `=` work)
  - **Casts**: `asforcefit` allows long long → narrower integer types
  - **Storage / ABI**: 8-byte memory (big-endian hi then lo); args as two stack longs; return in D0:D1; CSE does not park values in a single D-reg
  - **Constants**: `ival` + `ival_hi` pair (C89-safe); `LL`/`ULL` nodes carry both halves; global init emits two `DC.l`
  - **Codegen**: `add.l`/`addx.l` (and sub/subx) on D0:D1; `putconst` emits full 32-bit immediates (no `ICON16L` truncate); `addx`/`subx` opcode names fixed
  - **Still open**: long long mul/div/mod quality; some narrow casts may still emit `move.f`; soft-float `.f` length mnemonic for size 8 is legacy FP naming
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
- [ ] **__VA_ARGS__** - Variable argument macro support
- [ ] **##__VA_ARGS__** - Token pasting with variadic args

#### 2.2.2 Pragmas
- [ ] **#pragma STDC** - Standard pragma support
- [ ] **#pragma once** - Single-include optimization

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

### 3.4 C23 still to do (not simple aliases)

#### Remaining type-system work
- [ ] **`_BitInt(N)`** - bit-precise integers (widths 1..64 on m68k) (+ `wb`/`uwb` suffixes)
- [ ] **`typeof` / `typeof_unqual`** - type of expression / unqualified type
- [x] **`alignof` / `_Alignof`**, **`alignas` / `_Alignas`** - + `<stdalign.h>`; forced align stored in TYP `_pad_typ` (CNOP 2/4/8)
- [ ] **`auto` type inference** - deduce type from initializer
- [ ] **`constexpr`** - compile-time constant objects
- [ ] **`_Atomic`** - or define `__STDC_NO_ATOMICS__` as `1`
- [ ] **`_Complex` / `_Imaginary`** - or define `__STDC_NO_COMPLEX__` as `1`
- [ ] **`nullptr_t`** - distinct null-pointer type (today `nullptr` is `void*`)

#### Often grouped with C23/C99 types (not keywords alone)
- [ ] **VLAs / flexible array members**
- [ ] **Designated initializers** - `.field` / `[index]`
- [ ] **Compound literals** - `(type){...}`
- [ ] **Empty braced initialization** - `= {}` (C23)

#### Lexer / constants
- [x] **`U` / `L` / `UL` integer suffixes** - C89/C99 unsigned/long/long-long suffixes in `getnumber()` (`10U`, `10L`, `10UL` / `10LU`, `10ULL` / `10LLU`)
- [x] **Binary integer literals** - `0b` / `0B` prefixes in `getnumber()`
- [x] **Digit separators** - skip `'` inside numeric tokens (`1'000'000`)
- [ ] **`wb` / `uwb` suffixes** - `_BitInt` constant suffixes (needs `_BitInt` first)
- [ ] **UTF-8 / `u8` string prefixes** - treat as ordinary strings initially; full Unicode later

#### Keywords / other
- [x] **`static_assert` / `_Static_assert`** - compile-time assert (const-expr [, message]); file and block scope
- [ ] **`thread_local` / `_Thread_local`** - or document as unsupported + `__STDC_NO_THREADS__`

#### Preprocessor / macros
- [ ] **`__func__`** - string of current function name (`lastfunc`)
- [ ] **`__has_include`** - include-existence probe
- [ ] **`__has_c_attribute` / `__has_embed`** - feature probes
- [ ] **`__VA_OPT__(...)`** - optional variadic macro tokens
- [ ] **`#embed`** - binary resource inclusion
- [ ] **Bump `__STDC_VERSION__`** toward `202311L` once a documented subset is claimed
- [x] **Feature-absence macros** - `__STDC_NO_ATOMICS__`, `__STDC_NO_THREADS__`, `__STDC_NO_COMPLEX__`, `__STDC_NO_VLA__` as `1`

#### Larger language features
- [ ] **`_Generic`** - type-generic selection
- [ ] **Anonymous structs/unions** - unnamed members
- [ ] **Labels before `}`** - allow label at end of block

## Implementation Priority

### High Priority (Must Fix)
1. **Function parameter addressing bug** - Critical for basic functionality
2. **Floating-point arithmetic** - Essential for math operations
3. **Stack frame limitations** - Prevents large programs from compiling
4. **Buffer flushing** - Affects I/O reliability
5. **Complete preprocessor** - Required for most C programs
6. ✅ **SAS/C parameter passing** - `__regargs` and `__stdargs` for library compatibility - **COMPLETED**
7. **SAS/C memory management** - `__chip`, `__far`, `__near` for Amiga development

### Medium Priority (Should Fix)
1. **Standard library completeness** - Needed for portability
2. **Type system improvements** - Better type safety
3. **Error handling** - Better user experience
4. **Code generation** - Better performance
5. **SAS/C function control** - `__saveds`, `__interrupt` for system programming
6. **SAS/C advanced features** - Unnamed unions, structure equivalence

### Low Priority (Nice to Have)
1. **C99 features** - Modern language features
2. **Advanced optimizations** - Performance improvements
3. **Debugging enhancements** - Developer experience
4. **SAS/C convenience features** - `__inline`, zero-length arrays, preprocessor extensions
5. **SAS/C internationalization** - National characters in identifiers

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
