
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
 * $Log:	Expr.c,v $
 * Revision 3.33  90/04/04  23:19:19  lionel
 * Added recognition of unsigned *, /, and % operations.
 * 
 * Revision 3.32  90/02/03  16:23:29  lionel
 * None
 * 
 *------------------------------------------------------------------
 */

/*
 * Expr.c
 * 
 * expression evaluation
 * 
 * This set of routines builds a parse tree for an expression. no code is
 * generated for the expressions during the build, this is the job of the
 * codegen module. for most purposes expression() is the routine to call. it
 * will allow all of the C operators. for the case where the comma operator
 * is not valid (function parameters for instance) call exprnc().
 * 
 * Each of the routines returns a pointer to a describing type structure. each
 * routine also takes one parameter which is a pointer to an expression node
 * by reference (address of pointer). the completed expression is returned in
 * this pointer. all routines return either a pointer to a valid type or NULL
 * if the hierarchy of the next operator is too low or the next symbol is not
 * part of an expression.
 */

/* 890926 (LDH) Removed forcefit() for && and || ops.  It generates irrelevant
 *              code.
 */

#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    "C.h"
#include    "Expr.h"
#include    "Gen.h"
#include    "Cglbdec.h"

TYP             stdint = {bt_long, 0, 4, {0, 0}, 0, "int"};
TYP             stdunsigned = {bt_unsigned, 0, 4, {0, 0}, 0, "unsigned"};
TYP             stdchar = {bt_char, 0, 1, {0, 0}, 0, "char"};
TYP             stdshort = {bt_short, 0, 2, {0, 0}, 0, "short"};
TYP             stdstring = {bt_pointer, 1, 4, {0, 0}, &stdchar, "string"};
TYP             stdfunc = {bt_func, 1, 0, {0, 0}, &stdint, "func"};
TYP             stdfloat = {bt_float, 0, 4, {0, 0}, 0, "float"};
TYP             stddouble = {bt_double, 0, 8, {0, 0}, 0, "double"};

extern TYP     *head;       /* shared with decl */
extern TYP     *istypedef();
extern TYP     *maketype();
extern SYM     *gsearch();
extern SYM     *search();
extern char    *litlate();
extern long     stringlit();
extern long     floatlit();
extern int      dodefined();
extern char    *xalloc();
extern int      oneline;

TYP            *asforcefit();   /* Forward declaration */
TYP            *forcefit();

TYP            *expression();   /* forward declaration */
TYP            *exprnc();   /* forward declaration */
TYP            *unary();    /* forward declaration */

struct enode   *
makenode(nt, v1, v2)

/*
 * build an expression node with a node type of nt and values v1 and v2.
 */
    enum e_node     nt;
    struct enode   *v1, *v2;
{
    struct enode   *ep;

    ep = (struct enode *) xalloc(sizeof(struct enode));
    ep->nodetype = nt;
    ep->size = 0;
    ep->constflag = 0;
    ep->signedflag = 1;
    ep->v.p[0] = v1;
    ep->v.p[1] = v2;
    return ep;
}

int
equal_types(tp1, tp2)
    TYP            *tp1, *tp2;
{
    if (tp1 == tp2)
        return (TRUE);

    if (tp1 != NULL && tp2 != NULL) 
        if (tp1->type == tp2->type) 
            if (equal_types(tp1->btp, tp2->btp)) 
                return (TRUE);
    return (FALSE);
}


TYP            *
deref(node, tp)

/*
 * build the proper dereference operation for a node using the type pointer
 * tp.
 */
    struct enode  **node;
    TYP            *tp;
{
    struct enode   *ep1;

    if (node == NULL || tp == NULL) {
        fprintf( stderr, "DIAG -- NULL argument to deref.\n" );
        return (NULL);
    }

    switch (tp->type) {
    case bt_char:
        *node = makenode(en_b_ref, *node, NULL);
        tp = &stdchar;
        break;
    case bt_uchar:
        (*node)->signedflag = 0;
        *node = makenode(en_ub_ref, *node, NULL);
        (*node)->signedflag = 0;
        break;
    case bt_short:
    case bt_enum:
        *node = makenode(en_w_ref, *node, NULL);
        tp = &stdshort;
        break;
    case bt_ushort:
        (*node)->signedflag = 0;
        *node = makenode(en_uw_ref, *node, NULL);
        (*node)->signedflag = 0;
        break;
    case bt_long:
    case bt_pointer:
        *node = makenode(en_l_ref, *node, NULL);
        break;
    case bt_unsigned:
        (*node)->signedflag = 0;
        *node = makenode(en_ul_ref, *node, NULL);
        (*node)->signedflag = 0;
        tp = &stdunsigned;
        break;
    case bt_struct:
    case bt_union:
        ep1 = makenode(en_icon, (long) tp->size, NULL);
        *node = makenode(en_m_ref, *node, ep1);
        break;
    case bt_float:
        *node = makenode(en_f_ref, *node, NULL);
        tp = &stdfloat;
        break;
    case bt_double:
        *node = makenode(en_d_ref, *node, NULL);
        tp = &stddouble;
        break;
    default:
        error(ERR_DEREF, NULL);
        break;
    }
    return tp;
}

TYP            *
nameref(node)

/*
 * nameref will build an expression tree that references an identifier. if
 * the identifier is not in the global or local symbol table then a
 * look-ahead to the next character is done and if it indicates a function
 * call the identifier is coerced to an external function name. non-value
 * references generate an additional level of indirection.
 */
    struct enode  **node;
{
    SYM            *sp;
    TYP            *tp;

    sp = gsearch(lastid);

    if (sp == NULL) {
        ++global_flag;
        sp = (SYM *) xalloc(sizeof(SYM));
        sp->tp = maketype ( bt_func, 0 );
        sp->tp->btp = &stdint;
        sp->tp->val_flag = 1;
        sp->name = litlate(lastid);
        sp->storage_class = sc_external;
        sp->value.i = 1;                    /* External is referenced */
        --global_flag;

        getsym();
        /*
         * Declare unknown functions as extern int func()
         */

        if (lastst == openpa) {
            insert(sp, &gsyms);
            tp = &stdfunc;
            *node = makenode(en_nacon, sp->name, NULL);
            (*node)->constflag = 1;
        }
        else {
            if (oneline) {      /* Preprocessor kludge */
                tp = &stdint;
                *node = makenode(en_icon, 0, NULL );
                (*node)->constflag = 1;
            }
            else {
                tp = &stdint;
                ++global_flag;
                sp = (SYM *) xalloc(sizeof(SYM));
                sp->tp = tp;
                sp->name = litlate(lastid);
                sp->storage_class = sc_external;
                sp->value.i = 1;                    /* External is referenced */
                --global_flag;
                error(ERR_UNDEFINED, lastid );
                insert(sp, &gsyms);
                *node = makenode(en_nacon, sp->name, NULL);
                (*node)->constflag = 1;
            }
        }
    }
    else {
        if ((tp = sp->tp) == NULL) {
            error(ERR_UNDEFINED, "untyped identifier" );
            return NULL;    /* guard against untyped entries */
        }
        switch (sp->storage_class) {
        case sc_static:
            if (tp->type != bt_func && tp->type != bt_ifunc) 
                *node = makenode(en_labcon, sp->value.i, NULL);
            else 
                *node = makenode(en_nacon, sp->name, NULL);
            (*node)->constflag = 1;
            break;
        case sc_external:
            sp->value.i = 1;    /* Referenced    */
            /* FALL-THROUGH */
        case sc_global:
            *node = makenode(en_nacon, sp->name, NULL);
            (*node)->constflag = 1;
            break;
        case sc_const:
            *node = makenode(en_icon, sp->value.i, NULL);
            (*node)->constflag = 1;
            break;
        default:    /* auto and any errors */
            if (sp->storage_class != sc_auto)
                error(ERR_ILLCLASS, NULL);
            *node = makenode(en_autocon, sp->value.i, NULL);
            break;
        }
        if (tp->val_flag == 0)
            tp = deref(node, tp);
        getsym();
    }
    return tp;
}

struct enode   *
parmlist(sp)
    SYM            *sp;

/*
 * parmlist will build a list of parameter expressions in a function call and
 * return a pointer to the last expression parsed. since parameters are
 * generally pushed from right to left we get just what we asked for...
 */
{
    int             parmsize;
    struct enode   *ep1, *ep2, *ep3;
    enum e_node     etp;
    TYP            *tp1, *tp2;
    SYM            *sp1;

    ep1 = ep2 = ep3 = NULL;
    sp1 = NULL;
    tp1 = tp2 = NULL;

    parmsize = 0;

    if (sp != NULL && sp->tp != NULL)
        sp1 = sp->tp->lst.head;

    while (lastst != closepa) {
        tp1 = exprnc(&ep2); /* evaluate a parameter                         */

        if (sp1 != NULL)/* Get the prototype description        */
            tp2 = sp1->tp;

        if (tp1 != NULL) {
            if (tp2 != NULL) {
                if (!equal_types(tp1, tp2))
                    error(ERR_PROTO, NULL);
            }
/* TODO
If assigning prototypes based on prior usage, duplicate this type and assign 
it to sp1->tp.
*/
            switch (ep2->nodetype) {
            case en_b_ref:
                ep2 = makenode(en_cbl, ep2, NULL);
                parmsize += 4;
                break;
            case en_ub_ref:
                ep2 = makenode(en_cbl, ep2, NULL);
                ep2->signedflag = 0;
                parmsize += 4;
                break;
            case en_w_ref:
                ep2 = makenode(en_cwl, ep2, NULL);
                parmsize += 4;
                break;
            case en_uw_ref:
                ep2 = makenode(en_cwl, ep2, NULL);
                ep2->signedflag = 0;
                parmsize += 4;
                break;
            case en_f_ref:
                ep2 = makenode(en_cfd, ep2, NULL);
                parmsize += 8;
                break;
            case en_d_ref:
                parmsize += 8;
                break;
            default:
                parmsize += 4;
                break;
            }
        }
        ep1 = makenode(en_void, ep2, ep1);
        if (ep2)
            etp = ep2->nodetype;
        else
            etp = en_void;
        ep3 = makenode(etp, NULL, ep3);

        if (lastst != comma)
            break;

        getsym();

        if (sp1 != NULL)/* Advance to the next prototype variable */
            sp1 = sp1->next;
    }
    ep1 = makenode(en_info, ep1, ep3);

    if (parmsize > maxparmsize)
        maxparmsize = parmsize;

    return ep1;
}

int
castbegin(st)

/*
 * return 1 if st in set of [ kw_char, kw_short, kw_long, kw_int, kw_float,
 * kw_double, kw_struct, kw_union, kw_enum ]
 */
    enum e_sym      st;
{
    TYP            *tp;

    tp = istypedef(NULL);

    return (tp != NULL) ||
        st == kw_char || st == kw_short || st == kw_int ||
        st == kw_long || st == kw_float || st == kw_double ||
        st == kw_enum || st == kw_struct || st == kw_union ||
        st == kw_void || st == kw_unsigned;
}

TYP            *
primary(node)

/*
 * primary will parse a primary expression and set the node pointer returning
 * the type of the expression parsed. primary expressions are any of: id
 * constant string ( expression ) primary[ expression ] primary.id
 * primary->id primary( parameter list )
 */
    struct enode  **node;
{
    int             index;
    struct enode   *pnode, *qnode, *rnode;
    SYM            *sp, *sp2;
    TYP            *tptr, *tp, *tp2;

    switch (lastst) {
    case id:
        tptr = nameref(&pnode);
        break;
    case iconst:
        tptr = &stdint;
        pnode = makenode(en_icon, (long) ival, NULL);
        pnode->constflag = 1;
        getsym();
        break;
    case cconst:
        tptr = &stdint;
        pnode = makenode(en_icon, (long) ival, NULL);
        pnode->constflag = 1;
        getsym();
        break;
    case sconst:
        tptr = &stdstring;
        index = stringlit(laststr);
        pnode = makenode(en_labcon, index, NULL);
        pnode->constflag = 1;
        getsym();
        while (lastst == sconst) {
            stringconcat(index, laststr);
            getsym();
        }
        break;
    case rconst:
        tptr = &stddouble;
        pnode = makenode(en_labcon, floatlit(rval), NULL);
        tptr = deref(&pnode, tptr);
        getsym();
        break;
    case openpa:
        getsym();
        if (!castbegin(lastst)) {
            tptr = expression(&pnode);
            needpunc(closepa);
        }
        else {      /* cast operator */
            decl(NULL); /* do cast declaration */
            decl1();
            tptr = head;
            needpunc(closepa);
            if ((tp = unary(&pnode)) != NULL)
                tptr = asforcefit(NULL, tptr, &pnode, tp);
            else {
                error(ERR_IDEXPECT, NULL);
                tptr = NULL;
            }
        }
        break;
    default:
        return NULL;
    }

    if (tptr == NULL) {
        return (NULL);
    }

    for (;;) {
        switch (lastst) {
        case openbr:    /* build a subscript reference */
            getsym();
            tp = expression(&rnode);

            if (tptr->type != bt_pointer) {
                if (tp->type != bt_pointer) {
                    error(ERR_NOPOINTER, NULL);
                    break;
                }
                qnode = pnode;
                pnode = rnode;
                rnode = qnode;

                tp2 = tptr;
                tptr = tp;
                tp = tp2;
            }

            tptr = tptr->btp;

            if (rnode == NULL) {
                error(ERR_SYNTAX, NULL);
                break;
            }

            /*
             * we could check the type of the expression here...
             */

            if (tptr->size == 0)
                qnode = rnode;
            else {
                qnode = makenode(en_icon, (long) (tptr->size), NULL);
                qnode->constflag = 1;
                qnode = makenode(en_mul, qnode, rnode);
                qnode->constflag = rnode->constflag && qnode->v.p[0]->constflag;
            }
            pnode = makenode(en_add, pnode, qnode);
            pnode->constflag = qnode->constflag && pnode->v.p[1]->constflag;
            if (tptr->val_flag == 0)
                tptr = deref(&pnode, tptr);
            needpunc(closebr);
            break;
        case pointsto:
            if (tptr->type != bt_pointer)
                error(ERR_NOPOINTER, NULL);
            else
                tptr = tptr->btp;
            if (tptr->val_flag == 0)
                pnode = makenode(en_l_ref, pnode, NULL);

            /*
             * fall through to dot operation
             */
        case dot:
            getsym();   /* past -> or . */
            if (lastst != id)
                error(ERR_IDEXPECT, NULL);
            else {
                sp = search(lastid, tptr->lst.head);
                if (sp == NULL)
                    error(ERR_NOMEMBER, NULL);
                else {
                    tptr = sp->tp;
                    qnode = makenode(en_icon, (long) (sp->value.i), NULL);
                    qnode->constflag = 1;
                    pnode = makenode(en_add, pnode, qnode);
                    pnode->constflag = pnode->v.p[0]->constflag;
                    if (tptr->val_flag == 0)
                        tptr = deref(&pnode, tptr);
                }
                getsym();   /* past id */
            }
            break;
        case openpa:    /* function reference */
            getsym();
            if (tptr->type != bt_func && tptr->type != bt_ifunc)
                error(ERR_NOFUNC, NULL);
            else
                tptr = tptr->btp;

            if (pnode->nodetype != en_nacon)
                sp = NULL;
            else {
                sp = gsearch(pnode->v.sp);
                if (strncmp( pnode->v.sp, "__LIBCALL_", 10) == 0) {
                    sp2 = gsearch(&pnode->v.sp[10]);
                    if (sp2 != NULL && sp2->tp->lst.head != NULL)
                        sp->tp->lst = sp2->tp->lst;
                }
            }

            pnode = makenode(en_fcall, pnode, parmlist(sp));
            pnode->size = tptr->size;
            needpunc(closepa);
            break;
        default:
            goto fini;
        }
    }
fini:
    *node = pnode;
    return tptr;
}

int
lvalue(node)

/*
 * this function returns true if the node passed is an lvalue. this can be
 * qualified by the fact that an lvalue must have one of the dereference
 * operators as it's top node.
 */
    struct enode   *node;
{
    switch (node->nodetype) {
    case en_b_ref:
    case en_ub_ref:
    case en_w_ref:
    case en_uw_ref:
    case en_l_ref:
    case en_ul_ref:
    case en_m_ref:
    case en_f_ref:
    case en_d_ref:
        return TRUE;
    case en_cbl:
    case en_cwl:
    case en_cbw:
    case en_clf:
    case en_cld:
    case en_cfl:
    case en_cdl:
    case en_cfd:
        return lvalue(node->v.p[0]);
    }
    return FALSE;
}

TYP            *
unary(node)

/*
 * unary evaluates unary expressions and returns the type of the expression
 * evaluated. unary expressions are any of:
 * 
 * primary primary++ primary-- !unary ~unary ++unary --unary -unary *unary
 * &unary (typecast)unary sizeof(typecast)
 * 
 */
    struct enode  **node;
{
    TYP            *tp, *tp1;
    struct enode   *ep1, *ep2, *ep3;
    int             flag, seen;
    long            i;

    flag = 0;
    switch (lastst) {
    case autodec:
        flag = 1;
        /* fall through to common increment */
    case autoinc:
        getsym();
        tp = unary(&ep1);
        if (tp == NULL) {
            error(ERR_IDEXPECT, NULL);
            return NULL;
        }
        if (! lvalue(ep1)) 
            error(ERR_LVALUE, NULL);
        else {
            if (tp->type == bt_pointer)
                ep2 = makenode(en_icon, (long) (tp->btp->size), NULL);
            else
                ep2 = makenode(en_icon, 1L, NULL);
            ep2->constflag = 1;

            switch (tp->type) {
            case bt_float:
                ep3 = ep1;
                ep2 = makenode( en_clf, ep2, NULL );
                ep1 = makenode(flag ? en_fsubs : en_fadds, ep1, ep2);
                ep1 = makenode( en_assign, ep3, ep1 );
                break;
            case bt_double:
                ep3 = ep1;
                ep2 = makenode( en_cld, ep2, NULL );
                ep1 = makenode(flag ? en_fsubd : en_faddd, ep1, ep2);
                ep1 = makenode( en_assign, ep3, ep1 );
                break;
            default:
                ep1 = makenode(flag ? en_assub : en_asadd, ep1, ep2);
                break;
            }
        }
        break;
    case minus:
        getsym();
        tp = unary(&ep1);
        if (tp == NULL) {
            error(ERR_IDEXPECT, NULL);
            return NULL;
        }
        switch (tp->type) {
        case bt_float:
            ep1 = makenode(en_fnegs, ep1, NULL);
            break;
        case bt_double:
            ep1 = makenode(en_fnegd, ep1, NULL);
            break;
        default:
            ep1 = makenode(en_uminus, ep1, NULL);
            break;
        }
        ep1->constflag = ep1->v.p[0]->constflag;
        break;
    case not:
        getsym();
        tp = unary(&ep1);
        if (tp == NULL) {
            error(ERR_IDEXPECT, NULL);
            return NULL;
        }
        ep1 = makenode(en_not, ep1, NULL);
        ep1->constflag = ep1->v.p[0]->constflag;
        break;
    case compl:
        getsym();
        tp = unary(&ep1);
        if (tp == NULL) {
            error(ERR_IDEXPECT, NULL);
            return NULL;
        }
        ep1 = makenode(en_compl, ep1, NULL);
        ep1->constflag = ep1->v.p[0]->constflag;
        break;
    case star:
        getsym();
        tp = unary(&ep1);
        if (tp == NULL) {
            error(ERR_IDEXPECT, NULL);
            return NULL;
        }
        if (tp->btp == NULL)
            error(ERR_DEREF, NULL);
        else
            tp = tp->btp;
        if (tp->val_flag == 0)
            tp = deref(&ep1, tp);
        break;
    case and:
        getsym();
        tp = unary(&ep1);
        if (tp == NULL) {
            error(ERR_IDEXPECT, NULL);
            return NULL;
        }
        if (lvalue(ep1))
            ep1 = ep1->v.p[0];
        tp1 = (TYP *) xalloc(sizeof(TYP));
        tp1->size = 4;
        tp1->type = bt_pointer;
        tp1->btp = tp;
        tp1->val_flag = 0;
        tp1->lst.head = NULL;
        tp1->sname = NULL;
        tp = tp1;
        break;
    case kw_defined:
        ep1 = makenode(en_icon, (long) dodefined(), NULL);
        ep1->constflag = 1;
        tp = &stdint;
        getsym();
        break;
    case kw_sizeof:
        getsym();
        if (seen = (lastst == openpa))
            needpunc(openpa);
        if (castbegin(lastst)) {
            decl(NULL);
            decl1();
            if (head != NULL)
                ep1 = makenode(en_icon, (long) (head->size), NULL);
            else {
                error(ERR_IDEXPECT, NULL);
                ep1 = makenode(en_icon, 1L, NULL);
            }
        }
        else {
            head = exprnc(&ep2);
            if (head != NULL)
                ep1 = makenode(en_icon, (long) (head->size), NULL);
            else {
                error(ERR_IDEXPECT, NULL);
                ep1 = makenode(en_icon, 1L, NULL);
            }
        }
        ep1->constflag = 1;
        tp = &stdint;
        if (seen)
            needpunc(closepa);
        break;

    default:
        tp = primary(&ep1);
        if (tp != NULL) {
            if (tp->type == bt_pointer)
                i = tp->btp->size;
            else
                i = 1;
            if (lastst == autoinc) {
                if (lvalue(ep1)) {
                    ep2 = makenode( en_icon, (long) i, NULL );
                    switch (tp->type) {
                    case bt_float:
                        ep2 = makenode( en_clf, ep2, NULL );
                        ep1 = makenode( en_faincs, ep1, ep2 );
                        break;
                    case bt_double:
                        ep2 = makenode( en_cld, ep2, NULL );
                        ep1 = makenode( en_faincd, ep1, ep2 );
                        break;
                    default:
                        ep1 = makenode( en_ainc, ep1, ep2 );
                        break;
                    }
                }
                else
                    error(ERR_LVALUE, NULL);
                getsym();
            }
            else if (lastst == autodec) {
                if (lvalue(ep1)) {
                    ep2 = makenode( en_icon, (long) i, NULL );
                    switch (tp->type) {
                    case bt_float:
                        ep2 = makenode( en_uminus, ep2, NULL );
                        ep2 = makenode( en_clf, ep2, NULL );
                        ep1 = makenode( en_faincs, ep1, ep2 );
                        break;
                    case bt_double:
                        ep2 = makenode( en_uminus, ep2, NULL );
                        ep2 = makenode( en_cld, ep2, NULL );
                        ep1 = makenode( en_faincd, ep1, ep2 );
                        break;
                    default:
                        ep1 = makenode( en_adec, ep1, ep2 );
                        break;
                    }
                }
                else
                    error(ERR_LVALUE, NULL);
                getsym();
            }
        }
        break;
    }
    *node = ep1;
    return tp;
}

int
isscalar(tp)

/*
 * this function returns true when the type of the argument is one of char,
 * short, unsigned, or long.
 */
    TYP            *tp;
{
    return
        (tp->type == bt_char || tp->type == bt_uchar ||
         tp->type == bt_short || tp->type == bt_ushort ||
         tp->type == bt_long ||
         tp->type == bt_unsigned);
}

TYP            *
multops(node)

/*
 * multops parses the multiply priority operators. the syntax of this group
 * is:
 * 
 * unary multop * unary multop / unary multop % unary
 */
    struct enode  **node;
{
    struct enode   *ep1, *ep2;
    TYP            *tp1, *tp2;
    enum e_sym      oper;

    tp1 = unary(&ep1);
    if (tp1 == NULL)
        return NULL;
    while (lastst == star || lastst == divide || lastst == modop) {
        oper = lastst;
        getsym();   /* move on to next unary op */
        tp2 = unary(&ep2);
        if (tp2 == NULL) {
            error(ERR_IDEXPECT, NULL);
            *node = ep1;
            return tp1;
        }
        tp1 = forcefit(&ep1, tp1, &ep2, tp2);
        switch (oper) {
        case star:
            switch (tp1->type) {
            case bt_float:
                ep1 = makenode(en_fmuls, ep1, ep2);
                break;
            case bt_double:
                ep1 = makenode(en_fmuld, ep1, ep2);
                break;
            case bt_unsigned:
            case bt_uchar:
            case bt_ushort:
                ep1 = makenode(en_umul, ep1, ep2);
                ep1->signedflag = 0;
                break;
            default:
                ep1 = makenode(en_mul, ep1, ep2);
                break;
            }
            break;
        case divide:
            switch (tp1->type) {
            case bt_float:
                ep1 = makenode(en_fdivs, ep1, ep2);
                break;
            case bt_double:
                ep1 = makenode(en_fdivd, ep1, ep2);
                break;
            case bt_unsigned:
            case bt_uchar:
            case bt_ushort:
                ep1 = makenode(en_udiv, ep1, ep2);
                ep1->signedflag = 0;
                break;
            default:
                ep1 = makenode(en_div, ep1, ep2);
                break;
            }
            break;
        case modop:
            switch (tp1->type) {
            case bt_float:
                ep1 = makenode(en_fmods, ep1, ep2);
                break;
            case bt_double:
                ep1 = makenode(en_fmodd, ep1, ep2);
                break;
            case bt_unsigned:
            case bt_uchar:
            case bt_ushort:
                ep1 = makenode(en_umod, ep1, ep2);
                ep1->signedflag = 0;
                break;
            default:
                ep1 = makenode(en_mod, ep1, ep2);
                break;
            }
            break;
        }
        ep1->constflag = ep1->v.p[0]->constflag &&
            ep1->v.p[1]->constflag;
    }
    *node = ep1;
    return tp1;
}

TYP            *
addops(node)

/*
 * addops handles the addition and subtraction operators.
 */
    struct enode  **node;
{
    struct enode   *ep1, *ep2, *ep3;
    TYP            *tp1, *tp2;
    int             oper;

    tp1 = multops(&ep1);
    if (tp1 == NULL)
        return NULL;
    while (lastst == plus || lastst == minus) {
        oper = (lastst == plus ? 1 : 0);
        getsym();
        tp2 = multops(&ep2);
        if (tp2 == NULL) {
            error(ERR_IDEXPECT, NULL);
            *node = ep1;
            return tp1;
        }
        if (tp1->type == bt_pointer) {
            if (tp2->type != bt_pointer) {
                tp2 = forcefit(NULL, &stdint, &ep2, tp2);
                ep3 = makenode(en_icon, (long) (tp1->btp->size), NULL);
                ep3->constflag = 1;
                ep2 = makenode(en_mul, ep3, ep2);
                ep2->constflag = ep2->v.p[1]->constflag;
            }
        }
        else if (tp2->type == bt_pointer) {
            if (tp1->type != bt_pointer) {
                tp1 = forcefit(NULL, &stdint, &ep1, tp1);
                ep3 = makenode(en_icon, (long) (tp2->btp->size), NULL);
                ep3->constflag = 1;
                ep1 = makenode(en_mul, ep3, ep1);
                ep1->constflag = ep1->v.p[1]->constflag;
            }
        }
        tp1 = forcefit(&ep1, tp1, &ep2, tp2);
        switch (tp1->type) {
        case bt_pointer:
            ep1 = makenode(oper ? en_add : en_sub, ep1, ep2);
            if (tp2->type == bt_pointer && !oper) {
                ep3 = makenode(en_icon, (long) (tp1->btp->size), NULL);
                ep3->constflag = 1;
                ep1 = makenode(en_div, ep1, ep3);
                tp1 = &stdint;
            }
            break;
        case bt_float:
            ep1 = makenode(oper ? en_fadds : en_fsubs, ep1, ep2);
            break;
        case bt_double:
            ep1 = makenode(oper ? en_faddd : en_fsubd, ep1, ep2);
            break;
        default:
            ep1 = makenode(oper ? en_add : en_sub, ep1, ep2);
            break;
        }
        ep1->constflag = ep1->v.p[0]->constflag &&
            ep1->v.p[1]->constflag;
    }
    *node = ep1;
    return tp1;
}

TYP            *
shiftop(node)

/*
 * shiftop handles the shift operators << and >>.
 */
    struct enode  **node;
{
    struct enode   *ep1, *ep2;
    TYP            *tp1, *tp2;
    int             oper;

    tp1 = addops(&ep1);
    if (tp1 == NULL)
        return NULL;
    while (lastst == lshift || lastst == rshift) {
        oper = (lastst == lshift);
        getsym();
        tp2 = addops(&ep2);
        if (tp2 == NULL)
            error(ERR_IDEXPECT, NULL);
        else {
            tp1 = forcefit(&ep1, tp1, &ep2, tp2);
            ep1 = makenode(oper ? en_lsh : en_rsh, ep1, ep2);
            ep1->constflag = ep1->v.p[0]->constflag &&
                ep1->v.p[1]->constflag;
        }
    }
    *node = ep1;
    return tp1;
}

TYP            *
relation(node)

/*
 * relation handles the relational operators < <= > and >=.
 */
    struct enode  **node;
{
    struct enode   *ep1, *ep2;
    TYP            *tp1, *tp2;
    enum e_node     nt;

    tp1 = shiftop(&ep1);
    if (tp1 == NULL)
        return NULL;
    for (;;) {
        switch (lastst) {

        case lt:
            switch (tp1->type) {
            case bt_unsigned:
            case bt_uchar:
            case bt_ushort:
                nt = en_ult;
                break;
            default:
                nt = en_lt;
            }
            break;
        case gt:
            switch (tp1->type) {
            case bt_unsigned:
            case bt_uchar:
            case bt_ushort:
                nt = en_ugt;
                break;
            default:
                nt = en_gt;
            }
            break;
        case leq:
            switch (tp1->type) {
            case bt_unsigned:
            case bt_uchar:
            case bt_ushort:
                nt = en_ule;
                break;
            default:
                nt = en_le;
            }
            break;
        case geq:
            switch (tp1->type) {
            case bt_unsigned:
            case bt_uchar:
            case bt_ushort:
                nt = en_uge;
                break;
            default:
                nt = en_ge;
            }
            break;
        default:
            goto fini;
        }
        getsym();
        tp2 = shiftop(&ep2);
        if (tp2 == NULL)
            error(ERR_IDEXPECT, NULL);
        else {
            tp1 = forcefit(&ep1, tp1, &ep2, tp2);
            ep1 = makenode(nt, ep1, ep2);
            ep1->constflag = ep1->v.p[0]->constflag &&
                ep1->v.p[1]->constflag;
            if (tp1->type == bt_double || tp1->type == bt_float)
                tp1 = &stdint;
        }
    }
fini:
    *node = ep1;
    return tp1;
}

TYP            *
equalops(node)

/*
 * equalops handles the equality and inequality operators.
 */
    struct enode  **node;
{
    struct enode   *ep1, *ep2;
    TYP            *tp1, *tp2;
    enum e_sym      lastoper;

    tp1 = relation(&ep1);
    if (tp1 == NULL)
        return NULL;
    while (lastst == eq || lastst == neq) {
        lastoper = lastst;
        getsym();
        tp2 = relation(&ep2);
        if (tp2 == NULL)
            error(ERR_IDEXPECT, NULL);
        else {
            tp1 = forcefit(&ep1, tp1, &ep2, tp2);
            ep1 = makenode(lastoper == eq ? en_eq : en_ne, ep1, ep2);
            ep1->constflag = ep1->v.p[0]->constflag &&
                ep1->v.p[1]->constflag;
            tp1 = &stdint;
        }
    }
    *node = ep1;
    return tp1;
}

TYP *
binop(node, xfunc, nt, sy)

/*
 * binop is a common routine to handle all of the legwork and error checking
 * for bitand, bitor, bitxor, andop, and orop.
 */
struct enode  **node;
TYP            *(*xfunc)(struct enode **);
enum e_node     nt;
enum e_sym      sy;

{
    struct enode   *ep1, *ep2;
    TYP            *tp1, *tp2;

    tp1 = (*xfunc) (&ep1);
    if (tp1 == NULL)
        return NULL;
    while (lastst == sy) {
        getsym();
        tp2 = (*xfunc) (&ep2);
        if (tp2 == NULL)
            error(ERR_IDEXPECT, NULL);
        else {
            if ( sy != land && sy != lor )
                tp1 = forcefit(&ep1, tp1, &ep2, tp2);
            ep1 = makenode(nt, ep1, ep2);
            ep1->constflag = ep1->v.p[0]->constflag &&
                ep1->v.p[1]->constflag;
        }
    }
    *node = ep1;
    return tp1;
}

TYP            *
bitand(node)

/*
 * the bitwise and operator...
 */
    struct enode  **node;
{
    return binop(node, equalops, en_and, and);
}

TYP            *
bitxor(node)
    struct enode  **node;
{
    return binop(node, bitand, en_xor, uparrow);
}

TYP            *
bitor(node)
    struct enode  **node;
{
    return binop(node, bitxor, en_or, or);
}

TYP            *
andop(node)
    struct enode  **node;
{
    return binop(node, bitor, en_land, land);
}

TYP            *
orop(node)
    struct enode  **node;
{
    return binop(node, andop, en_lor, lor);
}

TYP            *
conditional(node)

/*
 * this routine processes the hook operator.
 */
    struct enode  **node;
{
    TYP            *tp1, *tp2, *tp3;
    struct enode   *ep1, *ep2, *ep3;

    tp1 = orop(&ep1);   /* get condition */
    if (tp1 == NULL)
        return NULL;
    if (lastst == hook) {
        getsym();
        if ((tp2 = expression(&ep2)) == NULL) {
            error(ERR_IDEXPECT, NULL);
            goto cexit;
        }
        needpunc(colon);
        if ((tp3 = conditional(&ep3)) == NULL) {
            error(ERR_IDEXPECT, NULL);
            goto cexit;
        }
        switch (tp2->type) {
        case bt_float:
        case bt_double:
        case bt_struct:
        case bt_union:
            break;
        default:
            tp2 = forcefit(NULL, &stdint, &ep2, tp2);
            break;
        }

        switch (tp3->type) {
        case bt_float:
        case bt_double:
        case bt_struct:
        case bt_union:
            break;
        default:
            tp3 = forcefit(NULL, &stdint, &ep3, tp3);
            break;
        }

        tp1 = forcefit(&ep2, tp2, &ep3, tp3);
        ep2 = makenode(en_void, ep2, ep3);
        ep1 = makenode(en_cond, ep1, ep2);
    }
cexit:
    *node = ep1;
    return tp1;
}

TYP            *
asnop(node)

/*
 * asnop handles the assignment operators. currently only the simple
 * assignment is implemented.
 */
    struct enode  **node;
{
    struct enode   *ep1, *ep2, *ep3;
    TYP            *tp1, *tp2;
    enum e_node     op, op2;

    tp1 = conditional(&ep1);
    if (tp1 == NULL)
        return NULL;
    op2 = en_void;
    for (;;) {
        switch (lastst) {
        case assign:
            op = en_assign;
    ascomm:
            getsym();
            tp2 = asnop(&ep2);
    ascomm2:
            if (tp2 == NULL) {
                error(ERR_LVALUE, NULL);
            }
            else if (tp1->type == bt_struct || tp1->type == bt_union) {
                if (op != en_assign || tp1->type != tp2->type)
                    error(ERR_LVALUE, NULL);
                else {
                    tp1 = deref(&ep1, tp1);
                    tp2 = deref(&ep2, tp2);
                    tp1 = asforcefit(&ep1, tp1, &ep2, tp2);
                    ep1 = makenode(op, ep1, ep2);
                }
            }
            else if (!lvalue(ep1)) {
                error(ERR_LVALUE, NULL);
            }
            else {
                if (op2 == en_void) {
                    tp1 = asforcefit(&ep1, tp1, &ep2, tp2);
                    ep1 = makenode(op, ep1, ep2);
                }
                else {
                    tp1 = asforcefit(&ep1, tp1, &ep2, tp2);
                    ep3 = makenode(op2, ep1, ep2);
                    ep1 = makenode(en_assign, ep1, ep3);
                }
            }
            break;
        case asplus:
            switch (tp1->type) {
            case bt_float:
                op = en_assign;
                op2 = en_fadds;
                break;
            case bt_double:
                op = en_assign;
                op2 = en_faddd;
                break;
            default:
                op = en_asadd;
                break;
            }
    ascomm3:
            getsym();
            tp2 = asnop(&ep2);
            if (tp1->type == bt_pointer) {
                ep3 = makenode(en_icon, (long) (tp1->btp->size), NULL);
                ep2 = makenode(en_mul, ep2, ep3);
            }
            goto ascomm2;
        case asminus:
            switch (tp1->type) {
            case bt_float:
                op = en_assign;
                op2 = en_fsubs;
                break;
            case bt_double:
                op = en_assign;
                op2 = en_fsubd;
                break;
            default:
                op = en_assub;
                break;
            }
            goto ascomm3;
        case astimes:
            switch (tp1->type) {
            case bt_float:
                op = en_assign;
                op2 = en_fmuls;
                break;
            case bt_double:
                op = en_assign;
                op2 = en_fmuld;
                break;
            case bt_unsigned:
            case bt_uchar:
            case bt_ushort:
                op = en_asumul;
                break;
            default:
                op = en_asmul;
                break;
            }
            goto ascomm;
        case asdivide:
            switch (tp1->type) {
            case bt_float:
                op = en_assign;
                op2 = en_fdivs;
                break;
            case bt_double:
                op = en_assign;
                op2 = en_fdivd;
                break;
            case bt_unsigned:
            case bt_uchar:
            case bt_ushort:
                op = en_asudiv;
                break;
            default:
                op = en_asdiv;
                break;
            }
            goto ascomm;
        case asmodop:
            switch (tp1->type) {
            case bt_float:
                op = en_assign;
                op2 = en_fmods;
                break;
            case bt_double:
                op = en_assign;
                op2 = en_fmodd;
                break;
            case bt_unsigned:
            case bt_uchar:
            case bt_ushort:
                op = en_asumod;
                break;
            default:
                op = en_asmod;
                break;
            }
            goto ascomm;
        case aslshift:
            op = en_aslsh;
            goto ascomm;
        case asrshift:
            op = en_asrsh;
            goto ascomm;
        case asand:
            op = en_asand;
            goto ascomm;
        case asor:
            op = en_asor;
            goto ascomm;
        case aseor:
            op = en_aseor;
            goto ascomm;
        default:
            goto asexit;
        }
    }
asexit:
    *node = ep1;
    return tp1;
}

TYP            *
exprnc(node)

/*
 * evaluate an expression where the comma operator is not legal.
 */
    struct enode  **node;
{
    TYP            *tp;

    tp = asnop(node);
    if (tp == NULL)
        *node = NULL;
    return tp;
}

TYP            *
commaop(node)

/*
 * evaluate the comma operator. comma operators are kept as void nodes.
 */
    struct enode  **node;
{
    TYP            *tp1;
    struct enode   *ep1, *ep2;

    tp1 = asnop(&ep1);
    if (tp1 == NULL)
        return NULL;
    if (lastst == comma) {
        getsym();
        tp1 = commaop(&ep2);
        if (tp1 == NULL) {
            error(ERR_IDEXPECT, NULL);
            goto coexit;
        }
        ep1 = makenode(en_void, ep1, ep2);
    }
coexit:
    *node = ep1;
    return tp1;
}

TYP            *
expression(node)

/*
 * evaluate an expression where all operators are legal.
 */
    struct enode  **node;
{
    TYP            *tp;

    tp = commaop(node);
    if (tp == NULL)
        *node = NULL;
    return tp;
}
