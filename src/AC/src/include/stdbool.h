/* stdbool.h - Boolean type and values (C99 / C23) */

#ifndef __STDBOOL_H
#define __STDBOOL_H

/*
 * AC treats bool, true, and false as built-in keywords.
 * Macros remain for sources that expect the C99 header spellings.
 */
#define true 1
#define false 0

#define __bool_true_false_are_defined 1

#endif /* __STDBOOL_H */
