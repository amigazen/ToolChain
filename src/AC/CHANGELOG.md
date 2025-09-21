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

### Changed
- **Error message formatting** - Fixed format string in #error directive to use proper %s instead of %1024s
- **Macro system architecture** - Enhanced macro parameter handling and expansion logic
- **Preprocessor compliance** - Improved ANSI C standard compliance for macro definitions

### Fixed
- **Preprocessor directive handling** - Improved robustness of preprocessor directive parsing
- **Macro redefinition errors** - Fixed handling of identical macro redefinitions
- **Stringification escaping** - Proper handling of quotes and backslashes in stringified parameters

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
