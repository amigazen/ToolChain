
/* PDC Compiler - A Freely Distributable C Compiler for the Amiga
 *                Based upon prior work by Matthew Brandt and Jeff Lydiatt.
 *
 * PDC Compiler release 3.3 Copyright (C) 1989 Paul Petersen and Lionel Hummel.
 * PDC Software Distribution (C) 1989 Lionel Hummel and Paul Petersen.
 *
 * This code is freely redistributable upon the conditions that this 
 * notice remains intact and that modified versions of this file not be 
 * distributed as part of the PDC Software Distribution without the express
 * consent of the copyright holders.
 *
 *------------------------------------------------------------------
 *
 * $Log:	Include.c,v $
 * Revision 3.33  90/04/05  22:35:12  lionel
 * None.
 * 
 * Revision 3.32  90/02/03  16:24:38  lionel
 * None
 * 
 *------------------------------------------------------------------
 */

/*
 * Include.c All the files included while building PDC, for use when
 * pre-compiling
 */

#include        <stdio.h>
#include        "C.h"
#include        "Expr.h"
#include        "Gen.h"
#include        "Cglbdec.h"
#include        "Version.h"

