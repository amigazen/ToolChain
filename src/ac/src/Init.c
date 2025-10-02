
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
 * $Log:	Init.c,v $
 * Revision 3.33  90/04/05  22:35:27  lionel
 * None.
 * 
 * Revision 3.32  90/02/03  16:24:40  lionel
 * None
 * 
 *------------------------------------------------------------------
 */

/*
 * Init.c
 * 
 * Initializes all sorts of nodes that go into the symbol table/parse tree.
 * Generate and initialize a node for an auto initialized variable.
 */

#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    "C.h"
#include    "Expr.h"
#include    "Gen.h"
#include    "Cglbdec.h"

extern SYM     *gsearch();
extern SYM     *gsearch();
extern TYP     *exprnc(), *asforcefit(), *deref();
extern long     intexpr(), stringlit();
extern char    *xalloc();
extern double   floatexpr();
extern struct enode *addrexpr();
extern struct enode *makenode();
extern struct amode *gen_expr();

void    endinit();

int
initstring(tp) 
    TYP            *tp;
{
    int nbytes;
    char    *p;

    nbytes = 1;
    while (lastst == sconst) {
        nbytes += strlen(laststr);
        for (p = laststr; *p; ++p)
            genbyte(*p);
        getsym();
    }
    genbyte(0);
    if (nbytes & 1) {
        nbytes++;
        genbyte(0);
    }

    return( nbytes );
}

int
initarray(tp)
    TYP            *tp;
{
    int             nbytes, num;

    nbytes = 0;
    if (lastst == begin) {
        getsym();   /* skip past the brace */
        if (lastst == sconst &&
            (tp->btp->type == bt_char || tp->btp->type == bt_uchar)) {
            nbytes = initstring(tp);
        }
        else {
            while (lastst != end) {
                nbytes += inittype(tp->btp);
                if (lastst == comma)
                    getsym();
                else if (lastst != end) {
                    error(ERR_PUNCT, NULL);
                    getsym();
                }
            }
        }
        getsym();   /* skip closing brace */
    }
    else if (lastst == cconst &&
         (tp->btp->type == bt_char || tp->btp->type == bt_uchar)) {
        nbytes = 1;
        genbyte( ival );
        getsym();
    }
    else if (lastst == sconst &&
         (tp->btp->type == bt_char || tp->btp->type == bt_uchar)) {
        nbytes = initstring(tp);
    }
    else if (lastst != semicolon)
        error(ERR_ILLINIT, NULL);
    if (nbytes < tp->size) {
        num = tp->size - nbytes;
        if (num & 1) {  /* Only generate even amounts of storage */
            genbyte(0);
            --num;
        }
        if (num > 0)
            genstorage((int) num);
        nbytes = tp->size;
    }
    else if (tp->size != 0 && nbytes > tp->size)
        error(ERR_INITSIZE, NULL);  /* too many initializers */
    nl();
    return nbytes;
}

int
initstruct(tp)
    TYP            *tp;
{
    SYM            *sp;
    int             nbytes;
    int             seen;

    if (seen = (lastst == begin))
        needpunc(begin);

    nbytes = 0;
    sp = tp->lst.head;  /* start at top of symbol table */

    if (sp != NULL) {
        if (nbytes < sp->value.i) {
            while (nbytes < sp->value.i) {  /* align properly */
                genbyte(0);
                ++nbytes;
            }
        }
        nbytes += inittype(sp->tp);
        sp = sp->next;

        while (sp != NULL) {
            if (lastst == end)
                goto done;
            if (lastst == comma)
                getsym();
            if (nbytes < sp->value.i) {
                while (nbytes < sp->value.i) {  /* align properly */
                    genbyte(0);
                    ++nbytes;
                }
            }
            nbytes += inittype(sp->tp);
            sp = sp->next;
        }
    }
done:
    if (nbytes < tp->size)
        genstorage((int) (tp->size - nbytes));
    if (seen) {
        if (lastst == comma)
            getsym();
        needpunc(end);
    }
    nl();
    return tp->size;
}

int
initchar()
{
    int             i;

    i = (int) intexpr();
    genbyte(i);
    return 1;
}

int
initshort()
{
    genword((long) intexpr());
    return 2;
}

int
initlong()
{
    genlong((long) intexpr());
    return 4;
}

int
initsingle()
{
    gensingle((double) floatexpr());
    return 4;
}

int
initdouble()
{
    gendouble((double) floatexpr());
    return 8;
}

int
initpointer()
{
    SYM            *sp;
    struct enode   *ep;
    int             offset;
    int             seen;

    if (seen = (lastst == begin))
        getsym();

    if ((ep = addrexpr()) == NULL)
        error(ERR_SYNTAX, NULL);
    else if (ep->nodetype == en_icon)
        genlong(ep->v.i);
    else if (ep->nodetype == en_labcon)
        gen_labref((int) ep->v.i, 0);
    else if (ep->nodetype == en_add || ep->nodetype == en_sub) {
        if (ep->v.p[0]->nodetype != en_nacon &&
            ep->v.p[0]->nodetype != en_labcon)
            swap_nodes(ep);
        if (ep->v.p[0]->nodetype != en_nacon &&
            ep->v.p[0]->nodetype != en_labcon)
            error(ERR_IDEXPECT, NULL);
        else {
            if (ep->v.p[0]->nodetype == en_labcon) {
                offset = ep->v.p[1]->v.i;
                if (ep->nodetype == en_add)
                    gen_labref((int) ep->v.p[0]->v.i, offset);
                else
                    gen_labref((int) ep->v.p[0]->v.i, -offset);
            }
            else if ((sp = gsearch(ep->v.p[0]->v.sp)) == NULL)
                error(ERR_UNDEFINED, ep->v.p[0]->v.sp);
            else {
                if (sp->storage_class == sc_auto)
                    error(ERR_NOINIT, NULL);
                else if (ep->v.p[1]->nodetype != en_icon)
                    error(ERR_SYNTAX, NULL);
                else {
                    offset = ep->v.p[1]->v.i;
                    if (ep->nodetype == en_add)
                        genref(sp, offset);
                    else
                        genref(sp, -offset);
                }
            }
        }
    }
    else if (ep->nodetype == en_nacon) {
        if ((sp = gsearch(ep->v.sp)) == NULL)
            error(ERR_UNDEFINED, ep->v.p[0]->v.sp);
        else {
            if (sp->storage_class == sc_auto)
                error(ERR_NOINIT, NULL);
            else
                genref(sp, 0);
        }
    }
    else
        error(ERR_SYNTAX, NULL);
    endinit();

    if (seen)
        needpunc(end);

    return 4;       /* pointers are 4 bytes long */
}

void
endinit()
{
    if (lastst != comma && lastst != semicolon && lastst != end) {
        error(ERR_PUNCT, NULL);
        while (lastst != comma && lastst != semicolon &&
               lastst != end && lastst != eof)
            getsym();
    }
}

int
inittype(tp)
    TYP            *tp;
{
    int             nbytes;

    if (tp == NULL)
        return(0);

    switch (tp->type) {

    case bt_char:
    case bt_uchar:
    case bt_bool:
        nbytes = initchar();
        break;
    case bt_short:
    case bt_ushort:
    case bt_enum:
        nbytes = initshort();
        break;
    case bt_float:
        nbytes = initsingle();
        break;
    case bt_double:
        nbytes = initdouble();
        break;
    case bt_pointer:
        if (tp->val_flag) {
            nbytes = initarray(tp);
            if (nbytes > tp->size) {
                tp->size = nbytes;
            }
        }
        else
            nbytes = initpointer();
        break;
    case bt_long:
    case bt_unsigned:
    case bt_longlong:
    case bt_ulonglong:
#ifdef OLD_tfunc
        nbytes = initlong();
#else
        nbytes = initpointer();
#endif
        break;
    case bt_struct:
        nbytes = initstruct(tp);
        break;
    default:
        error(ERR_NOINIT, NULL);
        nbytes = 0;
    }
    return nbytes;
}

void
doinit(sp)
    SYM            *sp;
{
    if (lastst == assign)
        dseg();     /* initialize into data segment */
    else
        bseg();     /* generate storage in the bss  */

    nl();           /* start a new line in object */
    if (sp->storage_class == sc_static)
        put_label((long) (sp->value.i));
    else
        gen_strlab(sp->name);
    if (lastst != assign)
        genstorage((int) (sp->tp->size));
    else {
        getsym();
        inittype(sp->tp);
    }
    nl();
    endinit();
}


void
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
        error(ERR_LVALUE, NULL);
    else {
        tp1 = asforcefit(&ep1, tp1, &ep2, tp2);
        ep1 = makenode(en_assign, ep1, ep2);
        snp->exp = ep1;
        snp->next = NULL;
        addauto(snp);
    }
}
