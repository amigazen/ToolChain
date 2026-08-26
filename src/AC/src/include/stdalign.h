/* stdalign.h - alignment macros (C11 / C23) */

#ifndef __STDALIGN_H
#define __STDALIGN_H

/*
 * AC treats alignas / alignof / _Alignas / _Alignof as built-in keywords.
 * These macros exist for portable C11 code that includes <stdalign.h>.
 */
#define alignas _Alignas
#define alignof _Alignof
#define __alignas_is_defined 1
#define __alignof_is_defined 1

#endif /* __STDALIGN_H */
