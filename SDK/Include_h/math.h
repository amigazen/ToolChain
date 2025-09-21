/*
 * Libraries and headers for PDC release 3.3 (C) 1989 Lionel Hummel.
 * PDC Software Distribution (C) 1989 Lionel Hummel and Paul Petersen.
 * PDC I/O Library (C) 1987 by J.A. Lydiatt.
 *
 * This code is freely redistributable upon the conditions that this 
 * notice remains intact and that modified versions of this file not
 * be included as part of the PDC Software Distribution without the
 * express consent of the copyright holders.  No warrantee of any
 * kind is provided with this code.  For further information, contact:
 *
 *  PDC Software Distribution    Internet:                     BIX:
 *  P.O. Box 4006             or hummel@cs.uiuc.edu            lhummel
 *  Urbana, IL  61801-8801       petersen@uicsrd.csrd.uiuc.edu
 */

/* math.h - ANSI C mathematical functions and constants */

#ifndef MATH_H
#define MATH_H

#include <stddef.h>

/* ANSI C required mathematical constants */
#ifndef HUGE_VAL
#define HUGE_VAL (1.7976931348623157e+308)
#endif

/* ANSI C required mathematical functions */
double acos(double x);
double asin(double x);
double atan(double x);
double atan2(double y, double x);
double cos(double x);
double sin(double x);
double tan(double x);

double cosh(double x);
double sinh(double x);
double tanh(double x);

double exp(double x);
double frexp(double value, int *exp);
double ldexp(double x, int exp);
double log(double x);
double log10(double x);
double modf(double value, double *iptr);
double pow(double x, double y);
double sqrt(double x);

double ceil(double x);
double fabs(double x);
double floor(double x);
double fmod(double x, double y);

/* Non-ANSI extensions for compatibility */
double acosh(double x);
double asinh(double x);
double atanh(double x);
double cabs(double x);
double cbrt(double x);
double copysign(double x, double y);
double drem(double x, double y);
double expm1(double x);
double hypot(double x, double y);
double log1p(double x);
double logb(double x);
double rint(double x);
double scalb(double x, double n);

/* Mathematical constants */
#ifndef MAXDOUBLE
#define BITS(type) (8 * (int)sizeof(type))

#define MAXDOUBLE 1.79769313486231470e+308
#define MAXFLOAT ((float)3.40282346638528860e+38)
#define MINDOUBLE 4.94065645841246544e-324
#define MINFLOAT ((float)1.40129846432481707e-45)

#define DMINEXP (-(DMAXEXP + DSIGNIF - 4))
#define FMINEXP (-(FMAXEXP + FSIGNIF - 4))

#define DSIGNIF (BITS(double)-11)
#define FSIGNIF (BITS(float)-8)

#define DMAXEXP (1 << 11 - 1)
#define FMAXEXP (1 << 8 - 1)
#endif

#endif /* MATH_H */

