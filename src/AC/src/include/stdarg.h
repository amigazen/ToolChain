/*
 * stdarg.h - ANSI C variable argument macros (Amiga big-endian m68k)
 *
 * Arguments occupy 4-byte (or 8-byte for double/long double) stack slots.
 * Narrow types sit at the high-address end of the slot (SAS/C / AC ABI).
 * va_arg promotes: use int for char/short, double for float.
 */

#ifndef STDARG_H
#define STDARG_H

#include <stddef.h>

typedef char *va_list;

/* Round size up to a 4-byte stack slot. */
#define __VA_SIZE(type) \
    ((sizeof(type) + 3U) & ~3U)

/*
 * Point ap at the first anonymous argument: align past the end of last.
 * Works when last is a char/short sitting at +2/+3 in a 4-byte slot.
 */
#define va_start(ap, last) \
    ((void)((ap) = (char *)((((unsigned long)(char *)&(last)) \
        + sizeof(last) + 3UL) & ~(unsigned long)3)))

/*
 * Fetch type from the current slot, then advance.  Read at the high end
 * of the aligned slot so big-endian layout matches named parameters.
 */
#define va_arg(ap, type) \
    (*(type *)(((ap) += (int)__VA_SIZE(type)), \
        (ap) - (int)sizeof(type)))

#define va_end(ap)          ((void)0)

#endif /* STDARG_H */
