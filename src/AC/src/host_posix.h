/*
 * host_posix.h - detect a POSIX-capable host when building or running AC.
 *
 * AC_HOST_POSIX is defined for macOS, Linux, and other UNIX hosts where the
 * compiler itself is built with a normal ISO C library.  It is not defined
 * when building the AC executable on AmigaOS with the Amiga toolchain.
 *
 * Target Amiga code generation (680x0 assembler, SAS/C pragmas) is unchanged.
 */

#ifndef AC_HOST_POSIX_H
#define AC_HOST_POSIX_H

#if !defined(AC_HOST_POSIX)
#if defined(__unix__) || defined(__unix) || defined(_POSIX_VERSION) || \
    defined(_XOPEN_VERSION) || defined(__APPLE__) || defined(__MACH__)
#define AC_HOST_POSIX 1
#endif
#endif

#if defined(AC_HOST_POSIX)
void open_stdio(void);
void close_stdio(void);
#endif

#endif /* AC_HOST_POSIX_H */
