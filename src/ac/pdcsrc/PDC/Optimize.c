
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
 * $Log:	Optimize.c,v $
 * Revision 3.33  90/04/05  22:42:41  lionel
 * Added cases for unsigned *, /, %, *=, /=, and %= expressions.
 * 
 * Revision 3.32  90/02/03  16:24:59  lionel
 * None
 * 
 *------------------------------------------------------------------
 */

/*
 * Optimize.c
 * 
 * Applies the various optimizations known by PDC.
 */

#include        <stdio.h>
#include        "C.h"
#include        "Expr.h"
#include        "Gen.h"
#include        "Cglbdec.h"

extern struct enode *makenode();
extern void    *remlit();
extern void    *getlit();
extern long     floatlit();

void            fold_const();

int
fetchdouble(node, result)
    struct enode   *node;
    double         *result;
{
    double         *ptr;

    if (result == NULL)
        return (0);

    *result = 0.0;

    if (node->nodetype == en_d_ref) {
        if (node->v.p[0]->nodetype == en_labcon) {
            if ((ptr = (double *) getlit(node->v.p[0]->v.i)) != NULL) {
                *result = *ptr;
                return (1);
            }
        }
    }
    return (0);
}

void
dooper(node)

/*
 * dooper will execute a constant operation in a node and modify the node to
 * be the result of the operation.
 */
    struct enode  **node;
{
    struct enode   *ep;
    double          rval;

    ep = *node;
    switch (ep->nodetype) {
    case en_uminus:
        ep->nodetype = en_icon;
        ep->v.i = -ep->v.p[0]->v.i;
        break;
    case en_not:
        ep->nodetype = en_icon;
        ep->v.i = !ep->v.p[0]->v.i;
        break;
    case en_compl:
        ep->nodetype = en_icon;
        ep->v.i = ~ep->v.p[0]->v.i;
        break;
    case en_add:
        ep->nodetype = en_icon;
        ep->v.i = ep->v.p[0]->v.i + ep->v.p[1]->v.i;
        break;
    case en_sub:
        ep->nodetype = en_icon;
        ep->v.i = ep->v.p[0]->v.i - ep->v.p[1]->v.i;
        break;
    case en_mul:
        ep->nodetype = en_icon;
        ep->v.i = ep->v.p[0]->v.i * ep->v.p[1]->v.i;
        break;
    case en_div:
        ep->nodetype = en_icon;
        ep->v.i = ep->v.p[0]->v.i / ep->v.p[1]->v.i;
        break;
    case en_lsh:
        ep->nodetype = en_icon;
        ep->v.i = ep->v.p[0]->v.i << ep->v.p[1]->v.i;
        break;
    case en_rsh:
        ep->nodetype = en_icon;
        ep->v.i = ep->v.p[0]->v.i >> ep->v.p[1]->v.i;
        break;
    case en_and:
        ep->nodetype = en_icon;
        ep->v.i = ep->v.p[0]->v.i & ep->v.p[1]->v.i;
        break;
    case en_or:
        ep->nodetype = en_icon;
        ep->v.i = ep->v.p[0]->v.i | ep->v.p[1]->v.i;
        break;
    case en_xor:
        ep->nodetype = en_icon;
        ep->v.i = ep->v.p[0]->v.i ^ ep->v.p[1]->v.i;
        break;
    case en_land:
        ep->nodetype = en_icon;
        ep->v.i = ep->v.p[0]->v.i && ep->v.p[1]->v.i;
        break;
    case en_lor:
        ep->nodetype = en_icon;
        ep->v.i = ep->v.p[0]->v.i || ep->v.p[1]->v.i;
        break;
    case en_lt:
    case en_ult:
        ep->nodetype = en_icon;
        ep->v.i = ep->v.p[0]->v.i < ep->v.p[1]->v.i;
        break;
    case en_le:
    case en_ule:
        ep->nodetype = en_icon;
        ep->v.i = ep->v.p[0]->v.i <= ep->v.p[1]->v.i;
        break;
    case en_gt:
    case en_ugt:
        ep->nodetype = en_icon;
        ep->v.i = ep->v.p[0]->v.i > ep->v.p[1]->v.i;
        break;
    case en_ge:
    case en_uge:
        ep->nodetype = en_icon;
        ep->v.i = ep->v.p[0]->v.i >= ep->v.p[1]->v.i;
        break;
    case en_eq:
        ep->nodetype = en_icon;
        ep->v.i = ep->v.p[0]->v.i == ep->v.p[1]->v.i;
        break;
    case en_ne:
        ep->nodetype = en_icon;
        ep->v.i = ep->v.p[0]->v.i != ep->v.p[1]->v.i;
        break;
    case en_cdl:
    case en_cfl:
        ep->nodetype = en_icon;
        ep->v.i = ep->v.p[0]->v.f;
        break;
    case en_cld:
        rval = ep->v.p[0]->v.i;
        ep = makenode(en_labcon, floatlit(rval), NULL);
        *node = makenode(en_d_ref, ep, NULL);
        break;
    case en_fnegd:
        rval = -(ep->v.p[0]->v.f);
        ep = makenode(en_labcon, floatlit(rval), NULL);
        *node = makenode(en_d_ref, ep, NULL);
        break;
    case en_faddd:
        rval = ep->v.p[0]->v.f + ep->v.p[1]->v.f;
        ep = makenode(en_labcon, floatlit(rval), NULL);
        *node = makenode(en_d_ref, ep, NULL);
        break;
    case en_fsubd:
        rval = ep->v.p[0]->v.f - ep->v.p[1]->v.f;
        ep = makenode(en_labcon, floatlit(rval), NULL);
        *node = makenode(en_d_ref, ep, NULL);
        break;
    case en_fmuld:
        rval = ep->v.p[0]->v.f * ep->v.p[1]->v.f;
        ep = makenode(en_labcon, floatlit(rval), NULL);
        *node = makenode(en_d_ref, ep, NULL);
        break;
    case en_fdivd:
        rval = ep->v.p[0]->v.f / ep->v.p[1]->v.f;
        ep = makenode(en_labcon, floatlit(rval), NULL);
        *node = makenode(en_d_ref, ep, NULL);
        break;
    }
}

int
pwrof2(i)

/*
 * return which power of two i is or -1.
 */
    int             i;
{
    int             p, q;

    q = 2;
    p = 1;
    while (q > 0) {
        if (q == i)
            return p;
        q <<= 1;
        ++p;
    }
    return -1;
}

int
mod_mask(i)

/*
 * make a mod mask for a power of two.
 */
    int             i;
{
    int             m;

    m = 0;
    while (i--)
        m = (m << 1) | 1;
    return m;
}

void
ref_double(node)
    struct enode   *node;
{
    struct enode   *ep;
    double         *dptr;

    ep = node->v.p[0];
    if (ep->nodetype != en_fcon) {
        if (ep->nodetype != en_d_ref)
            return;
        if (ep->v.p[0]->nodetype != en_labcon)
            return;
        if (getlit(ep->v.p[0]->v.i) == NULL) 
            return;
    }

    ep = node->v.p[0];
    if (ep->nodetype != en_fcon) {
        dptr = (double *) remlit(ep->v.p[0]->v.i);
        ep->v.f = *dptr;
        ep->nodetype = en_fcon;
    }
}


void
ref2_double(node)
    struct enode   *node;
{
    int             i;
    struct enode   *ep;
    double         *dptr;

    for (i = 0; i < 2; i++) {
        ep = node->v.p[i];
        if (ep->nodetype != en_fcon) {
            if (ep->nodetype != en_d_ref)
                return;
            if (ep->v.p[0]->nodetype != en_labcon)
                return;
            if (getlit(ep->v.p[0]->v.i) == NULL) 
                return;
        }
    }

    for (i = 0; i < 2; i++) {
        ep = node->v.p[i];
        if (ep->nodetype != en_fcon) {
            if ((dptr = (double *) remlit(ep->v.p[0]->v.i)) != NULL) {
                ep->v.f = *dptr;
                ep->nodetype = en_fcon;
            }
        }
    }
}

void
opt0(node)

/*
 * opt0 - delete useless expressions and combine constants.
 * 
 * opt0 will delete expressions such as x + 0, x - 0, x * 0, x * 1, 0 / x, x /
 * 1, x mod 0, etc from the tree pointed to by node and combine obvious
 * constant operations. It cannot combine name and label constants but will
 * combine icon type nodes.
 */
    struct enode  **node;
{
    struct enode   *ep;
    int             sc;
    long            val;
    double          dval;

    ep = *node;

    if (ep == NULL)
        return;

    switch ((*node)->nodetype) {
    case en_b_ref:
    case en_ub_ref:
    case en_w_ref:      /* optimize unary node */
    case en_uw_ref:
    case en_l_ref:
    case en_m_ref:
    case en_d_ref:
    case en_f_ref:
    case en_cbw:
    case en_cbl:
    case en_cwl:
    case en_cfd:
    case en_cdf:

    case en_info:
        opt0(&((*node)->v.p[0]));
        return;
        break;

    case en_ainc:
    case en_adec:
    case en_faincs:
    case en_fadecs:
    case en_faincd:
    case en_fadecd:
        opt0(&((*node)->v.p[0]));
        opt0(&((*node)->v.p[1]));
        return;

    case en_not:
    case en_compl:
    case en_uminus:
        opt0(&(ep->v.p[0]));
        if (ep->v.p[0]->nodetype == en_icon)
            dooper(node);
        return;
    case en_add:
    case en_sub:
        opt0(&(ep->v.p[0]));
        opt0(&(ep->v.p[1]));
        if (ep->v.p[0]->nodetype == en_icon) {
            if (ep->v.p[1]->nodetype == en_icon) {
                dooper(node);
                return;
            }
            if (ep->v.p[0]->v.i == 0) {
                if (ep->nodetype == en_sub) {
                    ep->v.p[0] = ep->v.p[1];
                    ep->nodetype = en_uminus;
                }
                else
                    *node = ep->v.p[1];
                return;
            }
        }
        else if (ep->v.p[1]->nodetype == en_icon) {
            if (ep->v.p[1]->v.i == 0) {
                *node = ep->v.p[0];
                return;
            }
        }
        return;
    case en_mul:
        opt0(&(ep->v.p[0]));
        opt0(&(ep->v.p[1]));
        if (ep->v.p[0]->nodetype == en_icon) {
            if (ep->v.p[1]->nodetype == en_icon) {
                dooper(node);
                return;
            }
            val = ep->v.p[0]->v.i;
            if (val == 0) {
                *node = ep->v.p[0];
                return;
            }
            if (val == 1) {
                *node = ep->v.p[1];
                return;
            }
            sc = pwrof2((int) val);
            if (sc != -1) {
                swap_nodes(ep);
                ep->v.p[1]->v.i = sc;
                ep->nodetype = en_lsh;
            }
        }
        else if (ep->v.p[1]->nodetype == en_icon) {
            val = ep->v.p[1]->v.i;
            if (val == 0) {
                *node = ep->v.p[1];
                return;
            }
            if (val == 1) {
                *node = ep->v.p[0];
                return;
            }
            sc = pwrof2((int) val);
            if (sc != -1) {
                ep->v.p[1]->v.i = sc;
                ep->nodetype = en_lsh;
            }
        }
        break;
    case en_div:
        opt0(&(ep->v.p[0]));
        opt0(&(ep->v.p[1]));
        if (ep->v.p[0]->nodetype == en_icon) {
            if (ep->v.p[1]->nodetype == en_icon) {
                dooper(node);
                return;
            }
            if (ep->v.p[0]->v.i == 0) { /* 0/x */
                *node = ep->v.p[0];
                return;
            }
        }
        else if (ep->v.p[1]->nodetype == en_icon) {
            val = ep->v.p[1]->v.i;
            if (val == 1) { /* x/1 */
                *node = ep->v.p[0];
                return;
            }
            if (! Options.MulDiv32) {
                sc = pwrof2((int) val);
                if (sc != -1) {
                    ep->v.p[1]->v.i = sc;
                    ep->nodetype = en_rsh;
                }
            }
        }
        break;
    case en_mod:
    case en_umod:
        opt0(&(ep->v.p[0]));
        opt0(&(ep->v.p[1]));
        if (ep->v.p[1]->nodetype == en_icon) {
            if (ep->v.p[0]->nodetype == en_icon) {
                dooper(node);
                return;
            }
            if (! Options.MulDiv32) {
                sc = pwrof2((int) (ep->v.p[1]->v.i));
                if (sc != -1) {
                    ep->v.p[1]->v.i = mod_mask(sc);
                    ep->nodetype = en_and;
                }
            }
        }
        break;
    case en_and:
    case en_or:
    case en_xor:
    case en_rsh:
    case en_lsh:
    case en_lt:
    case en_le:
    case en_gt:
    case en_ge:
    case en_eq:
    case en_ne:
    case en_land:
    case en_lor:
        opt0(&(ep->v.p[0]));
        opt0(&(ep->v.p[1]));
        if (ep->v.p[0]->nodetype == en_icon &&
            ep->v.p[1]->nodetype == en_icon)
            dooper(node);
        break;
    case en_asand:
    case en_asor:
    case en_asadd:
    case en_assub:
        opt0(&(ep->v.p[0]));
        opt0(&(ep->v.p[1]));
        break;
    case en_asmul:
    case en_asumul:
        opt0(&(ep->v.p[0]));
        opt0(&(ep->v.p[1]));
        if (ep->v.p[1]->nodetype == en_icon) {
            val = ep->v.p[1]->v.i;
            if (val == 0) {
                (*node)->nodetype = en_assign;
                return;
            }
            if (val == 1) {
                *node = ep->v.p[0];
                return;
            }
            sc = pwrof2((int) val);
            if (sc != -1) {
                ep->v.p[1]->v.i = sc;
                ep->nodetype = en_aslsh;
            }
        }
        break;
    case en_asdiv:
    case en_asudiv:
        opt0(&(ep->v.p[0]));
        opt0(&(ep->v.p[1]));
        if (ep->v.p[1]->nodetype == en_icon) {
            val = ep->v.p[1]->v.i;
            if (val == 1) {
                *node = ep->v.p[0];
                return;
            }
            if (! Options.MulDiv32) {
                sc = pwrof2((int) val);
                if (sc != -1) {
                    ep->v.p[1]->v.i = sc;
                    ep->nodetype = en_asrsh;
                }
            }
        }
        break;
    case en_asmod:
    case en_asumod:
        opt0(&(ep->v.p[0]));
        opt0(&(ep->v.p[1]));
        if (ep->v.p[1]->nodetype == en_icon) {
            if (! Options.MulDiv32) {
                val = ep->v.p[1]->v.i;
                sc = pwrof2((int) val);
                if (sc != -1) {
                    ep->v.p[1]->v.i = mod_mask(sc);
                    ep->nodetype = en_asand;
                }
            }
        }
        break;
    case en_cond:
        opt0(&(ep->v.p[0]));
        opt0(&(ep->v.p[1]->v.p[0]));
        opt0(&(ep->v.p[1]->v.p[1]));
        if (ep->v.p[0]->nodetype == en_icon) {
            if (ep->v.p[0]->v.i) 
                *node = ep->v.p[1]->v.p[0];
            else
                *node = ep->v.p[1]->v.p[1];
        }
        break;
    case en_asrsh:
    case en_aslsh:
    case en_fcall:
    case en_void:
    case en_assign:
        opt0(&(ep->v.p[0]));
        opt0(&(ep->v.p[1]));
        break;
    case en_fnegd:
    case en_fnegs:
        opt0(&(ep->v.p[0]));
        ref_double(ep);
        if (ep->v.p[0]->nodetype == en_fcon)
            dooper(node);
        break;
    case en_faddd:
        opt0(&(ep->v.p[0]));
        opt0(&(ep->v.p[1]));
        if (fetchdouble(ep->v.p[0], &dval)) {
            if (dval == 0.0) {  /* 0.0 + X */
                *node = ep->v.p[1];
                (void) remlit(ep->v.p[0]->v.p[0]->v.i);
                return;
            }
        }
        if (fetchdouble(ep->v.p[1], &dval)) {
            if (dval == 0.0) {  /* X + 0.0 */
                *node = ep->v.p[0];
                (void) remlit(ep->v.p[1]->v.p[0]->v.i);
                return;
            }
        }
        ref2_double(ep);
        if (ep->v.p[0]->nodetype == en_fcon && ep->v.p[1]->nodetype == en_fcon)
            dooper(node);
        break;
    case en_fsubd:
        opt0(&(ep->v.p[0]));
        opt0(&(ep->v.p[1]));
        if (fetchdouble(ep->v.p[0], &dval)) {
            if (dval == 0.0) {  /* 0.0 - X */
                (void) remlit(ep->v.p[0]->v.p[0]->v.i);
                (*node)->v.p[0] = (*node)->v.p[1];
                (*node)->v.p[1] = NULL;
                (*node)->nodetype = en_fnegd;
                return;
            }
        }
        if (fetchdouble(ep->v.p[1], &dval)) {
            if (dval == 0.0) {  /* X - 0.0 */
                *node = ep->v.p[0];
                (void) remlit(ep->v.p[1]->v.p[0]->v.i);
                return;
            }
        }
        ref2_double(ep);
        if (ep->v.p[0]->nodetype == en_fcon && ep->v.p[1]->nodetype == en_fcon)
            dooper(node);
        break;
    case en_fmuld:
        opt0(&(ep->v.p[0]));
        opt0(&(ep->v.p[1]));
        if (fetchdouble(ep->v.p[0], &dval)) {
            if (dval == 0.0) {  /* 0.0 * X */
                *node = ep->v.p[0];
                return;
            }
            if (dval == 1.0) {  /* 1.0 * X */
                *node = ep->v.p[1];
                (void) remlit(ep->v.p[0]->v.p[0]->v.i);
                return;
            }
        }
        if (fetchdouble(ep->v.p[1], &dval)) {
            if (dval == 0.0) {  /* X * 0.0 */
                *node = ep->v.p[1];
                return;
            }
            if (dval == 1.0) {  /* X * 1.0 */
                *node = ep->v.p[0];
                (void) remlit(ep->v.p[1]->v.p[0]->v.i);
                return;
            }
        }
        ref2_double(ep);
        if (ep->v.p[0]->nodetype == en_fcon && ep->v.p[1]->nodetype == en_fcon)
            dooper(node);
        break;
    case en_fdivd:
        opt0(&(ep->v.p[0]));
        opt0(&(ep->v.p[1]));
        if (fetchdouble(ep->v.p[0], &dval)) {
            if (dval == 0.0) {  /* 0.0 / X */
                *node = ep->v.p[0];
                return;
            }
        }
        if (fetchdouble(ep->v.p[1], &dval)) {
            if (dval == 1.0) {  /* X / 1.0 */
                *node = ep->v.p[0];
                (void) remlit(ep->v.p[1]->v.p[0]->v.i);
                return;
            }
        }
        ref2_double(ep);
        if (ep->v.p[0]->nodetype == en_fcon && ep->v.p[1]->nodetype == en_fcon)
            dooper(node);
        break;
    case en_fmodd:
    case en_fadds:
    case en_fsubs:
    case en_fmuls:
    case en_fdivs:
    case en_fmods:
        opt0(&(ep->v.p[0]));
        opt0(&(ep->v.p[1]));
        ref2_double(ep);
        if (ep->v.p[0]->nodetype == en_fcon && ep->v.p[1]->nodetype == en_fcon)
            dooper(node);
        break;
    case en_cld:
    case en_clf:
        opt0(&(ep->v.p[0]));
        if (ep->v.p[0]->nodetype == en_icon)
            dooper(node);
        break;
    case en_cdl:
    case en_cfl:
        opt0(&(ep->v.p[0]));
        ref_double(ep);
        if (ep->v.p[0]->nodetype == en_fcon)
            dooper(node);
        break;
    default:
        break;
    }
}

long
xfold(node)

/*
 * xfold will remove constant nodes and return the values to the calling
 * routines.
 */
    struct enode   *node;
{
    long            i;

    if (node == NULL)
        return 0;
    switch (node->nodetype) {
    case en_icon:
        i = node->v.i;
        node->v.i = 0;
        return i;
    case en_add:
        return xfold(node->v.p[0]) + xfold(node->v.p[1]);
    case en_sub:
        return xfold(node->v.p[0]) - xfold(node->v.p[1]);
    case en_mul:
        if (node->v.p[0]->nodetype == en_icon)
            return xfold(node->v.p[1]) * node->v.p[0]->v.i;
        else if (node->v.p[1]->nodetype == en_icon)
            return xfold(node->v.p[0]) * node->v.p[1]->v.i;
        else
            return 0;
    case en_uminus:
        return -xfold(node->v.p[0]);
    case en_lsh:
    case en_rsh:
    case en_div:
    case en_mod:
    case en_umod:
    case en_asadd:
    case en_assub:
    case en_asmul:
    case en_asdiv:
    case en_asmod:
    case en_asumul:
    case en_asudiv:
    case en_asumod:
    case en_and:
    case en_land:
    case en_or:
    case en_lor:
    case en_xor:
    case en_asand:
    case en_asor:
    case en_void:
    case en_fcall:
    case en_assign:
        fold_const(&node->v.p[0]);
        fold_const(&node->v.p[1]);
        return 0;
    case en_faddd:
    case en_fsubd:
    case en_fmuld:
    case en_fdivd:
    case en_fmodd:
    case en_fadds:
    case en_fsubs:
    case en_fmuls:
    case en_fdivs:
    case en_fmods:
        fold_const(&node->v.p[0]);
        fold_const(&node->v.p[1]);
        return 0;
    case en_fnegd:
    case en_fnegs:
    case en_d_ref:
    case en_f_ref:
        fold_const(&node->v.p[0]);
        return 0;
    case en_b_ref:
    case en_ub_ref:
    case en_w_ref:
    case en_uw_ref:
    case en_l_ref:
    case en_m_ref:
    case en_compl:
    case en_not:
        fold_const(&node->v.p[0]);
        return 0;
    }
    return 0;
}

void
fold_const(node)

/*
 * reorganize an expression for optimal constant grouping.
 */
    struct enode  **node;
{
    struct enode   *ep;
    long            i;

    ep = *node;
    if (ep == NULL)
        return;
    if (ep->nodetype == en_add) {
        if (ep->v.p[0]->nodetype == en_icon) {
            ep->v.p[0]->v.i += xfold(ep->v.p[1]);
            return;
        }
        else if (ep->v.p[1]->nodetype == en_icon) {
            ep->v.p[1]->v.i += xfold(ep->v.p[0]);
            return;
        }
    }
    else if (ep->nodetype == en_sub) {
        if (ep->v.p[0]->nodetype == en_icon) {
            ep->v.p[0]->v.i -= xfold(ep->v.p[1]);
            return;
        }
        else if (ep->v.p[1]->nodetype == en_icon) {
            ep->v.p[1]->v.i -= xfold(ep->v.p[0]);
            return;
        }
    }
    i = xfold(ep);
    if (i != 0) {
        ep = makenode(en_icon, i, NULL);
        ep = makenode(en_add, ep, *node);
        *node = ep;
    }
}

void
opt4(node)

/*
 * apply all constant optimizations.
 */
    struct enode  **node;
{
    opt0(node);
    fold_const(node);
    opt0(node);
}
