
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

/*
 * Safety caps against corrupt type sizes during self-host bootstrap.
 * BearSSL T0 bytecode tables are several thousand bytes; keep headroom.
 */
#define MAX_INIT_PAD 65536
#define MAX_INIT_ELEMS 16384

extern SYM     *gsearch();
extern TYP     *exprnc(), *asforcefit(), *deref();
extern void     opt4();
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
            int ninit;

            ninit = 0;
            while (lastst != end) {
                if (++ninit > MAX_INIT_ELEMS) {
                    error(ERR_SYNTAX, NULL);
                    break;
                }
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
    {
        int tsz;

        /* Prefer type_size(): recovers count*elemsize if tp->size is bare. */
        tsz = type_size(tp);
        if (tsz <= 0)
            tsz = tp->size;
        if (nbytes < tsz) {
            num = tsz - nbytes;
            if (num > MAX_INIT_PAD || tsz > MAX_INIT_PAD)
                num = 0;
            if (num & 1) {  /* Only generate even amounts of storage */
                genbyte(0);
                --num;
            }
            if (num > 0)
                genstorage((int) num);
            nbytes = tsz;
        }
        else if (tsz != 0 && nbytes > tsz)
            error(ERR_INITSIZE, NULL);  /* too many initializers */
    }
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
    int             nmembers;

    if (seen = (lastst == begin))
        needpunc(begin);

    nbytes = 0;
    nmembers = 0;
    sp = tp->lst.head;  /* start at top of symbol table */

    if (sp != NULL) {
        if (nbytes < sp->value.i) {
            /*
             * Reject corrupt member offsets before genbyte loops explode.
             */
            if (sp->value.i > tp->size
                || sp->value.i - nbytes > tp->size
                || sp->value.i - nbytes > MAX_INIT_PAD)
                sp->value.i = nbytes;
            while (nbytes < sp->value.i) {  /* align properly */
                genbyte(0);
                ++nbytes;
            }
        }
        nbytes += inittype(sp->tp);
        sp = sp->next;
        nmembers = 1;

        while (sp != NULL) {
            if (++nmembers > MAX_INIT_ELEMS)
                break;
            if (lastst == end)
                goto done;
            if (lastst == comma)
                getsym();
            if (nbytes < sp->value.i) {
                if (sp->value.i > tp->size
                    || sp->value.i - nbytes > tp->size
                    || sp->value.i - nbytes > MAX_INIT_PAD)
                    sp->value.i = nbytes;
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
    if (nbytes < tp->size) {
        int pad;

        pad = tp->size - nbytes;
        if (pad > MAX_INIT_PAD || tp->size > MAX_INIT_PAD)
            pad = 0;
        if (pad > 0)
            genstorage(pad);
    }
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

/*
 * Emit an 8-byte long long initializer (m68k big-endian: high then low).
 */
int
initlonglong()
{
    TYP            *tp;
    struct enode   *node;
    long            hi;
    long            lo;

    tp = exprnc(&node);
    if (tp != NULL && node != NULL) {
        opt4(&node);
        if (node->nodetype == en_icon) {
            lo = node->v.i;
            if (node->size == 8 && node->v.p[1] != NULL
                && node->v.p[1]->nodetype == en_icon)
                hi = node->v.p[1]->v.i;
            else if (node->signedflag && lo < 0)
                hi = -1L;
            else
                hi = 0L;
            genlong(hi);
            genlong(lo);
            return 8;
        }
    }
    error(ERR_SYNTAX, NULL);
    genlong(0L);
    genlong(0L);
    return 8;
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
                offset = (int)ICON16L((long)ep->v.p[1]->v.i);
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
                    offset = (int)ICON16L((long)ep->v.p[1]->v.i);
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
        nbytes = initshort();
        break;
    case bt_enum:
        /* Same width as int (type_size / F.3.9); not a 16-bit short. */
        nbytes = initlong();
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
#ifdef OLD_tfunc
        nbytes = initlong();
#else
        nbytes = initpointer();
#endif
        break;
    case bt_longlong:
    case bt_ulonglong:
        nbytes = initlonglong();
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
    int             align;
    int             sz;

    if (lastst == assign)
        dseg();     /* initialize into data segment */
    else
        bseg();     /* generate storage in the bss  */

    nl();           /* start a new line in object */
    /*
     * Word-align before the label in this section.  After a 1-byte
     * BSS object (e.g. optsign), the next long/pointer (optarg) must
     * be even or A68k reports "Alignment error" on move.l.
     * Fall back to size: if alignment() wrongly returns 1 for a
     * multi-byte object, still emit CNOP 0,2.
     */
    align = 1;
    if (sp->tp != NULL) {
        align = alignment(sp->tp);
        if (align <= 1) {
            sz = type_size(sp->tp);
            if (sz >= 2)
                align = 2;
        }
    }
    if (align > 1)
        genalignment(align);
    if (sp->storage_class == sc_static)
        put_label((long) (sp->value.i));
    else
        gen_strlab(sp->name);
    if (lastst != assign) {
        /* Use type_size(): arrays are bt_pointer+val_flag, not 4 bytes. */
        sz = type_size(sp->tp);
        if (sz > MAX_INIT_PAD || sz < 0)
            sz = 0;
        /*
         * Never emit a BSS/data label with no DS.b - A68k/Blink then
         * produce load files LoadSeg rejects ("not executable").
         */
        if (sz <= 0 && sp->tp != NULL && sp->tp->type != bt_func
            && sp->tp->type != bt_ifunc && sp->tp->type != bt_void)
            sz = 4;
        if (sz > 0)
            genstorage(sz);
    }
    else {
        getsym();
        inittype(sp->tp);
    }
    nl();
    endinit();
}


/*
 * Emit seed = rhs as an auto init statement (st_expr assignment).
 * base_off is the frame offset of the object (like sp->value.i).
 */
static void
auto_assign_scalar(base_off, tp, ep_rhs, tp_rhs)
    long            base_off;
    TYP            *tp;
    struct enode   *ep_rhs;
    TYP            *tp_rhs;
{
    struct enode   *ep1;
    struct snode   *snp;

    if (tp == NULL || ep_rhs == NULL)
        return;
    snp = (struct snode *) xalloc(sizeof(struct snode));
    snp->stype = st_expr;
    ep1 = makenode(en_autocon, icon_unpoison(base_off), NULL);
    ep1->constflag = 0;
    tp = deref(&ep1, tp);
    if (tp_rhs == NULL || !lvalue(ep1))
        error(ERR_LVALUE, NULL);
    else {
        asforcefit(&ep1, tp, &ep_rhs, tp_rhs);
        snp->exp = makenode(en_assign, ep1, ep_rhs);
        snp->next = NULL;
        addauto(snp);
    }
}

/*
 * C99: brace initializer for an automatic aggregate (array/struct).
 * Walks '{' ... '}' and emits per-member/element assignments.
 */
static void
doinitauto_brace(base_off, tp)
    long            base_off;
    TYP            *tp;
{
    int             seen;
    int             n;
    long            elsz;
    SYM            *msp;
    TYP            *tp2;
    struct enode   *ep2;

    if (tp == NULL)
        return;
    seen = 0;
    if (lastst == begin) {
        seen = 1;
        getsym();
    }

    if (tp->type == bt_pointer && tp->val_flag != 0) {
        /* array */
        elsz = (long) type_size(tp->btp);
        if (elsz <= 0)
            elsz = 4;
        n = 0;
        while (lastst != end && lastst != eof) {
            if (n > MAX_INIT_ELEMS) {
                error(ERR_SYNTAX, NULL);
                break;
            }
            if (lastst == begin
                || (tp->btp != NULL
                    && (tp->btp->type == bt_struct
                        || tp->btp->type == bt_union
                        || (tp->btp->type == bt_pointer
                            && tp->btp->val_flag != 0))))
                doinitauto_brace(base_off + n * elsz, tp->btp);
            else {
                tp2 = exprnc(&ep2);
                auto_assign_scalar(base_off + n * elsz, tp->btp, ep2, tp2);
            }
            n++;
            if (lastst == comma)
                getsym();
            else if (lastst != end)
                break;
        }
    }
    else if (tp->type == bt_struct || tp->type == bt_union) {
        msp = tp->lst.head;
        n = 0;
        while (msp != NULL && lastst != end && lastst != eof) {
            if (++n > MAX_INIT_ELEMS)
                break;
            if (lastst == begin
                || (msp->tp != NULL
                    && (msp->tp->type == bt_struct
                        || msp->tp->type == bt_union
                        || (msp->tp->type == bt_pointer
                            && msp->tp->val_flag != 0))))
                doinitauto_brace(base_off + (long) msp->value.i, msp->tp);
            else {
                tp2 = exprnc(&ep2);
                auto_assign_scalar(base_off + (long) msp->value.i,
                    msp->tp, ep2, tp2);
            }
            msp = msp->next;
            if (lastst == comma)
                getsym();
            else if (lastst != end)
                break;
        }
    }
    else {
        tp2 = exprnc(&ep2);
        auto_assign_scalar(base_off, tp, ep2, tp2);
    }

    if (seen) {
        if (lastst == comma)
            getsym();
        needpunc(end);
    }
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

    getsym();       /* We found an auto initialized variable */

    /*
     * C99: local aggregate brace initializers (BearSSL PRF seed chunks).
     */
    if (lastst == begin && sp->tp != NULL
        && (sp->tp->type == bt_struct || sp->tp->type == bt_union
            || (sp->tp->type == bt_pointer && sp->tp->val_flag != 0))) {
        doinitauto_brace((long) sp->value.i, sp->tp);
        return;
    }

    snp = (struct snode *) xalloc(sizeof(struct snode));
    snp->stype = st_expr;

    ep1 = makenode(en_autocon, icon_unpoison(sp->value.i), NULL);
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
