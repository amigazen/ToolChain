/* stdbool.h - Boolean type and values (C99 / C23) */

#ifndef __STDBOOL_H
#define __STDBOOL_H

/*
 * AC treats bool, true, and false as built-in keywords (C23-style).
 * Do not #define true/false — the preprocessor would replace them with
 * "1"/"0" and the lexer would then reject the define operands.
 *
 * C99 programs that only #include <stdbool.h> still see true/false/bool
 * via the keywords.  __bool_true_false_are_defined remains for feature tests.
 */
#define __bool_true_false_are_defined 1

#endif /* __STDBOOL_H */
