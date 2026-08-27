# Changelog

All notable changes to the PDC (Publicly Distributable C) compiler will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [4.00]

### Added
- **#warning directive** - Non-standard but useful warning directive for conditional compilation
- **#pragma once** - Single-include optimization to prevent multiple inclusion of header files
- **Enhanced #error directive** - Improved error message formatting (removed fixed-width format specifier)
- **Variadic macros** - Support for `__VA_ARGS__` in macro definitions with `...` parameter syntax
- **Stringification operator** - `#` operator support for converting macro parameters to strings
- **Token pasting operator** - `##` operator support for concatenating macro tokens
- **Macro redefinition** - Allow redefinition of identical macros (ANSI C compliant)
- **Standard predefined macros** - Added `__STDC__` and `__STDC_VERSION__` macros for ANSI C compliance
- **Modern error reporting** - GCC-style error format: `filename:line:column: error: message`
- **Column tracking** - Accurate column position tracking for better error reporting
- **Multiple output formats** - Support for GCC, SASC, and PDC output formats
- **Warning system** - Comprehensive warning reporting with `-W` options
- **POSIX exit codes** - Proper exit codes following POSIX specifications
- **Warning function** - New `warning()` function for non-fatal compiler messages
- **Error vs Warning separation** - Clear distinction between fatal errors and recoverable warnings
- **No input files error** - Proper error message when compiler is run without input files
- **Type qualifiers** - Complete `const` and `volatile` support with proper type system integration
- **Integer promotion** - ANSI C compliant integer promotion rules for char/short types
- **Arithmetic conversions** - Proper usual arithmetic conversions following ANSI C standards
- **Array decay** - Arrays properly decay to pointers in expressions and function calls
- **Function pointers** - Enhanced function pointer type handling and assignments
- **libcall pragma format** - Updated to use SAS/C format for compatibility with existing pragma files
- **Complete SAS/C pragma support** - Added support for all SAS/C pragma types: flibcall, syscall, tagcall, and msg with full push/pop functionality
- **Amiga-specific defines** - Added `__AMIGA__` and `__amigaos__` predefined macros for Amiga platform detection
- **SAS/C compiler define** - Added `__SASC` predefined macro for SAS/C compatibility mode
- **#undef directive verification** - Confirmed proper functionality for undefining macros including predefined ones
- **Preprocessor deficits analysis** - Identified missing features for compiler-specific.h compatibility
- **SAS/C keyword recognition** - Added all 10 SAS/C keywords to lexer for proper token recognition
- **Memory section attributes** - Added support for `__chip`, `__far`, `__near`, `__fast` memory placement keywords
- **Interrupt function attributes** - Added support for `__interrupt` keyword in function declarations
- **Deep nesting support** - Increased preprocessor nesting depth from 10 to 32 levels with bounds checking
- **Graceful macro redefinition** - Improved macro redefinition handling with value comparison and warnings
- **Long long support** - Added C99 long long and unsigned long long type support
- **64-bit integer constants** - Added support for LL/ll suffixes in integer constants
- **Long long type system** - Complete type checking and conversion rules for 64-bit integers
- **64-bit arithmetic operations** - Complete 68000 assembly code generation for long long operations
- **C99 _Bool support** - Added _Bool keyword and boolean type support
- **stdbool.h header** - Created standard boolean header with bool, true, false definitions
- **Complete pragma code generation** - All SAS/C pragma types now have full 68000 assembly code generation
- **flibcall code generation** - Added gen_flibcall() and gen_flibcall2() for floating-point library calls
- **syscall code generation** - Added gen_syscall() and gen_syscall2() for system function calls
- **Calling convention keywords** - Added __regargs and __stdargs keyword support with attribute storage
- **Pragma integration** - All pragma types integrated into function call flow with proper precedence

### Changed
- **Error message formatting** - Fixed format string in #error directive to use proper %s instead of %1024s
- **Macro system architecture** - Enhanced macro parameter handling and expansion logic
- **Preprocessor compliance** - Improved ANSI C standard compliance for macro definitions
- **Console output** - Modernized to match standard C compiler and POSIX specifications
- **Error reporting** - Replaced old format with modern GCC/Clang-style error messages
- **Summary output** - Improved compilation summary with error and warning counts
- **Command-line interface** - Added new options for output format and warning control
- **Output format names** - Updated format names from MSVC/Classic to SASC/PDC for better Amiga compatibility
- **Error handling architecture** - Enhanced `error()` function to use modern formatting while maintaining backward compatibility

### Fixed
- **Preprocessor directive handling** - Improved robustness of preprocessor directive parsing
- **Macro redefinition errors** - Fixed handling of identical macro redefinitions
- **Stringification escaping** - Proper handling of quotes and backslashes in stringified parameters
- **Duplicate case values** - Fixed duplicate `case 'f':` in command-line argument parsing
- **Structure member definitions** - Synchronized structure definitions between header and implementation files
- **Input file validation** - Fixed logic to properly detect when no input files are provided (handles options-only command lines)
- **Type system architecture** - Enhanced type system with proper qualifier support and ANSI C compliant conversions
- **Expression evaluation** - Improved type promotion and arithmetic conversion rules
- **Function parameter handling** - Enhanced array decay and function pointer support
- **libcall pragma parsing** - Modified to automatically reverse argument order to match SAS/C format used by existing pragma files
- **Pragma system architecture** - Extended to support all SAS/C pragma types with proper parsing, storage, and output functions
- **Token buffering system** - Implemented proper lookahead token buffering for complete #pragma msg parsing with push/pop support
- **Preprocessor nesting limits** - Increased conditional compilation nesting depth from 10 to 32 levels
- **Macro redefinition logic** - Enhanced to allow identical redefinitions and warn on different values
- **Type system extensions** - Added memory section and interrupt attributes to type system
- **Expression node types** - Added new expression nodes for long long operations and conversions
- **Code generation framework** - Extended code generation to support 64-bit integer types
- **64-bit arithmetic code generation** - Complete gen_llbinary, gen_llmul, gen_lldiv, gen_llmod functions with real 68000 assembly
- **64-bit conversion code generation** - Complete gen_llextend function with proper sign/zero extension
- **68000 assembly implementation** - Real ADDX/SUBX for carry/borrow, MULS/DIVS/MODS for arithmetic, EXT/CLR for conversions
- **Boolean type system** - Complete _Bool type support with 1-byte storage and proper conversions

## [3.33] - 1989-08-01

### Added
- **ANSI C Features**:
  - All ANSI preprocessor directives
  - Function prototyping
  - Structure passing and assignment
- **Lattice Compatibility**: Supports Lattice-compatible libcall #pragmas
- **Pre-compiled Headers**: Generate and use pre-compiled header files for faster compilation
- **Inline Builtins**: Allows inline code generation for `__BUILTIN_` functions
- **Stack Checking**: Capable of generating stack-checking code (function wrappers)
- **Debugging Support**: Generate debugging information in DBX-compatible format
- **Cross-Platform**: Can be compiled and run under UNIX (SunOS)

### Changed
- **Fixed moveq peephole optimization** - Eliminated needless sign extensions
- **Fixed sign problem** - With libcall pragma function offsets
- **Improved support** - For unsigned types
- **Changed acrt0.o** - So that entry stack pointer is available in _initialSP
- **Fixed semantics** - Of multidimensional arrays
- **Fixed external libcall names** - Conflict with user-defined globals of the same name
- **system() improvements**:
  - Maximum path of 255 characters
  - Dynamic allocation for parameters
  - Parameter list can be of any length
  - Removed recursion from path-building function
  - Fixed to release FileLocks correctly
- **Added checking** - For 0x80000000 special case to format()
- **Added missing formatting character** - In putword()
- **Fixed formatting** - Of padded strings
- **Added PDCTmpArea support** - Environment variable support in ccx.c
- **Corrected default make targets** - For PDC in rules.c
- **Fixed trailing-newline make bug** - In reader.c
- **Made getenv return null** - In accordance with ANSI spec when no variable found
- **Added function wrappers** - Generate .entry and .exit wrappers for stack checking, profiling, etc.

### Fixed
- **CCX improvements**:
  - Can now take options interspersed with operands (e.g., `ccx Pi.c -lmath`)
  - Added PDC-specific default rules to Make utility

### Known Issues
- **Function parameter addressing** - PDC cannot correctly take the address of function parameters that are of type `char` or `short`. Workaround: assign parameter to dummy variable and take address of that
- **Floating-point arithmetic** - The arithmetic assignment and (++/--) operators do not function correctly for floating-point values
- **Stack frame limitations** - PDC cannot generate correct code for automatic variables beyond 32K into the stack frame (common limitation due to 68000 address modes)
- **Buffer flushing** - Buffer flushing is not being performed as it should be on writes to interactive files (consoles)

## [3.32] - 1989-02-03

### Changed
- **Repackaging and maintenance release** - Did not see extensive distribution

## [3.31] - 1988-07-01

### Added
- **First release** - Of Paul Petersen's work distributed at AmiExpo/Midwest
- **Added crucial missing features**:
  - Floating-point support
  - Integrated preprocessor
- **Fixed many bugs** - From previous Fish disk versions
- **More robust implementation** - For experienced programmers

## [3.30] - 1988-01-01

### Added
- **Initial PDC port** - By Jeff Lydiatt
- **Charlie Gibb's assembler** - Version 2.6
- **Basic C compiler functionality** - Limited application due to missing features
