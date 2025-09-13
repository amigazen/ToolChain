#include <stdio.h>
#include <math.h>
#include "pml.h"

/*
 * Modf returns the positive fractional part of "value" and stores the
 * integer part indirectly through "iptr". Thus the argument "value" and the
 * returned values "modf" and "*iptr" would satisfy the relation
 * 
 *          (*iptr + modf) = value          and 
 *          0 <= modf < abs(value)
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

#define W1_FBITS  21        /* (NOTE HIDDEN BIT NORMALIZATION)      */
#define W2_FBITS  32        /* Number of fractional bits in word 2  */
#define WORD_MASK 0xFFFFFFFF    /* Mask for each long word of double    */

union dtol {
    double          dval;
    int             ival[2];
};

double
modf(value, dptr)
    double          value, *dptr;
{
    double          frac;
    union dtol      number;
    int            *iptr, cexp, fbits;

    if (value == 0.0) {
        *dptr = 0.0;
        return (0.0);
    }
    else {
        frac = frexp(value, &cexp);
        number.dval = value;
        iptr = &number.ival[0];

        if (cexp <= 0) {
            *iptr++ = 0;
            *iptr++ = 0;
        }
        else {
            if (cexp <= W1_FBITS) {
                *iptr++ &= (WORD_MASK << (W1_FBITS - cexp));
                *iptr++ = 0;
            }
            else if (cexp <= (fbits = W1_FBITS + W2_FBITS)) {
                iptr++;
                *iptr++ &= (WORD_MASK << (fbits - cexp));
            }
            else {
                /* pmlerr(DINT_2BIG); */
            }
        }
        *dptr = number.dval;
        frac = value - number.dval;
        return (frac);
    }
}
