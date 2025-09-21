
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
 * $Log:	IntExpr.c,v $
 * Revision 3.33  90/04/05  22:35:44  lionel
 * None.
 * 
 * Revision 3.32  90/02/03  16:24:48  lionel
 * None
 * 
 *------------------------------------------------------------------
 */

/*
 * IntExpr.c
 * 
 * Builds parse tree for int/addr/float values.
 */

#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        "C.h"
#include        "Expr.h"
#include        "Gen.h"
#include        "Cglbdec.h"

extern TYP     *exprnc();
extern void    *remlit();

long
intexpr()
{
    TYP            *tp;
    struct enode   *node;

    tp = exprnc(&node);

    if (tp != NULL) {

        opt4(&node);
        if (node != NULL && node->nodetype == en_icon)
            return (node->v.i);
    }
    error(ERR_SYNTAX,NULL);
    return 0;
}

struct enode   *
addrexpr()
{
    TYP            *tp;
    struct enode   *node;

    tp = exprnc(&node);

    if (tp != NULL) {
        opt4(&node);
        if (node != NULL) {
            switch (node->nodetype) {
            case en_l_ref:
            case en_ul_ref:
            case en_w_ref:
            case en_uw_ref:
            case en_b_ref:
            case en_ub_ref:
                node = node->v.p[0];
            }

            switch (node->nodetype) {
            case en_icon:
            case en_nacon:
            case en_labcon:
            case en_add:
            case en_sub:
                return (node);
            }
        }
    }
    error(ERR_SYNTAX,NULL);
    return NULL;
}

double
floatexpr()
{
    TYP            *tp;
    int             lab;
    double         *ptr;
    struct enode   *node;

    tp = exprnc(&node);
    if (tp != NULL) {
        opt4(&node);

        if (node != NULL) {
            if (node->nodetype == en_icon)
                return((double) node->v.i);
            if (node->nodetype == en_fcon)
                return (node->v.f);
            if (node->nodetype == en_d_ref) {
                if (node->v.p[0]->nodetype == en_labcon) {
                    lab = node->v.p[0]->v.i;
                    ptr = (double *) remlit(lab);
                    if (ptr != NULL)
                        return (*ptr);
                }
            }
        }
    }
    error(ERR_SYNTAX, NULL);
    return 0;
}
