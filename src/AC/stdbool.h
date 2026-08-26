/* stdbool.h - Boolean type and values (C99 / C23) */

#ifndef __STDBOOL_H
#define __STDBOOL_H

/*
 * AC treats bool, true, and false as built-in keywords (C23-style).
 * Do not #define true/false — they clash with the lexer keywords.
 */
#define __bool_true_false_are_defined 1

#endif /* __STDBOOL_H */
