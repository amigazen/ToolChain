/*
 * stdint.h - fixed-width integer limits and macros (C99 / C23).
 *
 * Exact-width and related type names are built-in keywords in AC:
 *   int8_t/uint8_t, int16_t/uint16_t, int32_t/uint32_t, int64_t/uint64_t
 *   intptr_t/uintptr_t  -> long / unsigned long
 *   intmax_t/uintmax_t  -> long long / unsigned long long
 */

#ifndef __STDINT_H
#define __STDINT_H

#define INT8_MIN   (-127-1)
#define INT8_MAX   127
#define UINT8_MAX  255

#define INT16_MIN  (-32767-1)
#define INT16_MAX  32767
#define UINT16_MAX 65535

#define INT32_MIN  (-2147483647L-1L)
#define INT32_MAX  2147483647L
#define UINT32_MAX 4294967295UL

#define INT64_MIN  (-9223372036854775807LL-1LL)
#define INT64_MAX  9223372036854775807LL
#define UINT64_MAX 18446744073709551615ULL

#define INTPTR_MIN  INT32_MIN
#define INTPTR_MAX  INT32_MAX
#define UINTPTR_MAX UINT32_MAX

#define INTMAX_MIN  INT64_MIN
#define INTMAX_MAX  INT64_MAX
#define UINTMAX_MAX UINT64_MAX

#define PTRDIFF_MIN INT32_MIN
#define PTRDIFF_MAX INT32_MAX
#define SIZE_MAX    UINT32_MAX

#define INT8_C(v)    (v)
#define UINT8_C(v)   (v)
#define INT16_C(v)   (v)
#define UINT16_C(v)  (v)
#define INT32_C(v)   (v##L)
#define UINT32_C(v)  (v##UL)
#define INT64_C(v)   (v##LL)
#define UINT64_C(v)  (v##ULL)
#define INTMAX_C(v)  INT64_C(v)
#define UINTMAX_C(v) UINT64_C(v)

#endif /* __STDINT_H */
