
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
 * $Log:	InitAuto.c,v $
 * Revision 3.33  90/04/05  23:05:21  lionel
 * None.
 * 
 * Revision 3.32  90/02/03  16:24:45  lionel
 * None
 * 
 *------------------------------------------------------------------
 */

/* InitAuto.c
 *
 * Generate and initialize a node for an auto initialized variable.
 */

#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        "C.h"
#include        "Expr.h"
#include        "Gen.h"
#include        "Cglbdec.h"

extern char    *xalloc();
extern SYM     *gsearch();
extern long     intexpr(), stringlit();
extern TYP     *exprnc(), *asforcefit(), *deref();
extern struct enode *makenode();
extern struct amode *gen_expr();

doinitauto(sp)
    SYM            *sp;
{
    TYP            *tp1, *tp2;
    struct enode   *ep1, *ep2;
    struct snode   *snp;

    if (lastst != assign || sp->storage_class != sc_auto)
        return;

    getsym();       /* We found an auto initilized variable */

    snp = (struct snode *) xalloc(sizeof(struct snode));
    snp->stype = st_expr;

    ep1 = makenode(en_autocon, sp->value.i, NULL);
    ep1->constflag = 0;

    tp1 = sp->tp;
    tp1 = deref(&ep1, tp1);

    tp2 = exprnc(&ep2);

    if (tp2 == 0 || !lvalue(ep1))
        error(ERR_LVALUE);
    else {
        tp1 = asforcefit(&ep1, tp1, &ep2, tp2);
        ep1 = makenode(en_assign, ep1, ep2);
        snp->exp = ep1;
        snp->next = NULL;
        addauto(snp);
    }
}
