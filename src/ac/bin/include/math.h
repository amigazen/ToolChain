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

/* math.h - standard C math functions and constants */

extern double	modf();		/* Integer and remainder		*/
extern double	ldexp();	/* load the exponent			*/
extern double	frexp();	/* retrieve the exponent		*/

extern double	acos();		/* inverse trigonometric function	*/
extern double	asin();		/* inverse trigonometric function 	*/
extern double	atan();		/* inverse trigonometric function	*/
extern double	atan2();	/* inverse trigonometric function 	*/
extern double	cos();		/* trigonometric function		*/
extern double	sin();		/* trigonometric function		*/
extern double	tan();		/* trigonometric function		*/

extern double	acosh();	/* inverse hyperbolic function		*/
extern double	asinh();	/* inverse hyperbolic function		*/
extern double	atanh();	/* inverse hyperbolic function		*/

extern double	cosh();		/* hyperbolic function			*/
extern double	tanh();		/* hyperbolic function 			*/
extern double	sinh();		/* hyperbolic function			*/
extern double	cabs();		/* complex absolute value		*/

extern double	cbrt();		/* cube root				*/
extern double	sqrt();		/* square root				*/
extern double	hypot();	/* Euclidean distance			*/

extern double	fabs();		/* absolute value			*/
extern double	floor();	/* integer no greater than		*/
extern double	ceil();		/* integer no less than			*/
extern double	rint();		/* round to nearest integer		*/

extern double	drem();		/* remainder				*/
extern double	copysign();	/* copy sign bit			*/
extern double	logb();		/* exponent extraction			*/
extern double	scalb();	/* exponent adjustment			*/

extern double	exp();		/* exponential				*/
extern double	expm1();	/* exp(x)-1				*/
extern double	log();		/* natural logarithm			*/
extern double	log10();	/* logarithm to base 10			*/
extern double	log1p();	/* log(1+x)				*/
extern double	pow();		/* exponential x**y			*/

#ifndef MAXDOUBLE

#define BITS(type)	(8 * (int)sizeof(type))

#define MAXDOUBLE	1.79769313486231470e+308
#define MAXFLOAT	((float)3.40282346638528860e+38)
#define MINDOUBLE	4.94065645841246544e-324
#define MINFLOAT	((float)1.40129846432481707e-45)

#define DMINEXP	(-(DMAXEXP + DSIGNIF - 4))
#define FMINEXP	(-(FMAXEXP + FSIGNIF - 4))

#define DSIGNIF	(BITS(double)-11)
#define FSIGNIF	(BITS(float)-8)

#define DMAXEXP	(1 << 11 - 1)
#define FMAXEXP	(1 << 8 - 1)

#endif

