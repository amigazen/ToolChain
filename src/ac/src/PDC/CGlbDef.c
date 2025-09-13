
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
 * $Log:	CGlbDef.c,v $
 * Revision 3.33  90/04/04  02:30:03  lionel
 * *** empty log message ***
 * 
 * Revision 3.32  90/02/03  16:23:06  lionel
 * None
 * 
 *------------------------------------------------------------------
 */

/*
 * CGlbDef.c global definitions
 */

#include    <stdio.h>
#include    "C.h"
#include    "Expr.h"
#include    "Gen.h"

static char *RCSid = "$Header: work:PDC/src/PDC/RCS/CGlbDef.c,v 3.33 90/04/04 02:30:03 lionel Exp Locker: lionel $";

FILE *input = NULL;
FILE *list = NULL;
FILE *output = NULL;

struct OptTab {
    int             Optimize;
    int             List;
    int             Quiet;
    int             Annote;
    int             Frame;
    int             Debug;
    int             MulDiv32;
    int             Builtin;
    int             PreComp;
    int             Stack;
};

/*
 *  Optimize    ON 
 *  List        OFF 
 *  Quiet       OFF 
 *  Annote      OFF 
 *  Frame       A5
 *  Debug       OFF 
 *  MulDiv32    ON
 *  Builtin     OFF
 *  PreComp     OFF
 *  Stack       OFF
 */

struct OptTab   Options = {1, 0, 0, 0, 5, 0, 1, 0, 0, 0};

int             lineno = 0;
int             nextlabel = 0;
int             lastch = 0;
enum e_sym      lastst = id;
char            lastid[MAX_IDP1] = "";
char            laststr[MAX_STLP1] = "";
char           *curfile;
long            ival = 0;
double          rval;

TABLE           gsyms = {NULL, NULL},
                lsyms = {NULL, NULL};

SYM            *lastfunc = NULL;
SYM            *lasthead = NULL;
struct slit    *strtab = NULL;

int             lc_static = 0;
int             lc_auto = 0;
int             lc2_auto = 0;
int             lc_base = 0;
int             float_auto = 0;
int             nest_level = 0;
int             maxparmsize = 0;

char            code_name[ 64 ] = "1";
char            data_name[ 64 ] = "2";
char            bss_name[ 64 ] = "3";

struct snode   *bodyptr = NULL;
struct snode   *autohead = NULL;
struct snode   *autotail = NULL;

struct libcall *libpragma = NULL;

int             global_flag = 1;
TABLE           defsyms = {NULL, NULL};
TABLE           cmd_defsyms = {NULL, NULL};
TABLE           cmd_undefsyms = {NULL, NULL};
TABLE           cmd_include = {NULL, NULL};
TABLE           cmd_local = {NULL, NULL};

int             save_mask = 0;  /* register save mask    */
int             PdcFlags = 0;   /* Flags to load libraries   */

