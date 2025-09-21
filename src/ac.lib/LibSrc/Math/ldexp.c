#include <stdio.h>
#include <math.h>
#include "pml.h"

/*
 * ldexp returns the quanity "value" * 2 ^ "exp"
 * 
 * For the mc68000 using IEEE format the double precision word format is:
 * 
 * WORD N   =>    SEEEEEEEEEEEMMMM 
 * WORD N+1 =>    MMMMMMMMMMMMMMMM 
 * WORD N+2 =>    MMMMMMMMMMMMMMMM 
 * WORD N+3 =>    MMMMMMMMMMMMMMMM
 * 
 * Where:          S  =>   Sign bit 
 *                 E  =>   Exponent 
 *                 X  =>   Ignored (set to 0)
 *                 M  =>   Mantissa bit
 * 
 * NOTE:  Beware of 0.0; on some machines which use excess 128 notation for the
 * exponent, if the mantissa is zero the exponent is also.
 * 
 */

#define MANT_MASK 0x800FFFFF    /* Mantissa extraction mask     */
#define ZPOS_MASK 0x3FF00000    /* Positive # mask for exp = 0  */
#define ZNEG_MASK 0x3FF00000    /* Negative # mask for exp = 0  */

#define EXP_MASK 0x7FF00000 /* Mask for exponent            */
#define EXP_SHIFTS 20       /* Shifts to get into LSB's     */
#define EXP_BIAS 1023       /* Exponent bias                */


union dtol {
    double          dval;
    int             ival[2];
};

double
ldexp(value, exp)
    double          value;
    int             exp;
{
    union dtol      number;
    int            *iptr, cexp;

    if (value == 0.0)
        return (0.0);
    else {
        number.dval = value;
        iptr = &number.ival[0];
        cexp = (((*iptr) & EXP_MASK) >> EXP_SHIFTS) - EXP_BIAS;
        *iptr &= ~EXP_MASK;
        exp += EXP_BIAS;
        *iptr |= ((exp + cexp) << EXP_SHIFTS) & EXP_MASK;
        return (number.dval);
    }
}
