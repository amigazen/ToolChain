
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
 * $Log:	Version.h,v $
 * Revision 3.33  90/04/05  22:51:41  lionel
 * *** empty log message ***
 * 
 * Revision 3.32  90/02/03  16:25:57  lionel
 * None
 * 
 *------------------------------------------------------------------
 */

/*  Version.h
 *
 *  This is the current patch level for this version of PDC.
 *
 *  No responsibility is taken for any errors or inaccuracies inherent
 *  either to the comments or the code of this program, but if
 *  reported, we can attempt to fix them.
 */

#ifdef AZTEC_C
#define VERSION "AC Compiler version 4.0 (21/9/25)"
#else
#define VERSION "AC version 4.0 ("__DATE__")"
static const char *version_tag = "$VER: AC 4.0 ("__DATE__")";
static const char *stack_cookie = "$STACK: 65536";
#endif