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
 * $Log:	Force.c,v $
 * Revision 3.33  90/04/05  23:04:41  lionel
 * None.
 * 
 * Revision 3.32  90/02/03  16:23:44  lionel
 * None
 * 
 *------------------------------------------------------------------
 */

/* Force.c - Performs type conversions for binary and assignment ops
 */

#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    "C.h"
#include    "Expr.h"
#include    "Gen.h"
#include    "Cglbdec.h"

extern TYP stdint;
extern TYP stdunsigned;
extern TYP stdlonglong;
extern TYP stdulonglong;
extern TYP stdchar;
extern TYP stdshort;
extern TYP stdstring;
extern TYP stdfunc;
extern TYP stdfloat;
extern TYP stddouble;

/* Integer promotion function - promotes char/short to int/unsigned int */
TYP *integer_promote(TYP *tp, struct enode **node)
{
    switch (tp->type) {
    case bt_char:
        if (node != NULL) {
            *node = makenode(en_cbl, *node, NULL);
            (*node)->signedflag = 1;
        }
        return &stdint;
    case bt_uchar:
        if (node != NULL) {
            *node = makenode(en_cbl, *node, NULL);
            (*node)->signedflag = 0;
        }
        return &stdunsigned;
    case bt_short:
        if (node != NULL) {
            *node = makenode(en_cwl, *node, NULL);
            (*node)->signedflag = 1;
        }
        return &stdint;
    case bt_ushort:
        if (node != NULL) {
            *node = makenode(en_cwl, *node, NULL);
            (*node)->signedflag = 0;
        }
        return &stdunsigned;
    default:
        return tp;  /* No promotion needed */
    }
}

/* Array decay function - converts arrays to pointers when used in expressions */
TYP *array_decay(TYP *tp, struct enode **node)
{
    /* Arrays decay to pointers to their first element */
    if (tp != NULL && tp->type == bt_pointer && tp->val_flag == 1) {
        /* This is an array type, convert to pointer */
        TYP *new_tp = maketype(bt_pointer, 4);
        new_tp->btp = tp->btp;
        new_tp->val_flag = 0;  /* Not an array anymore */
        new_tp->qualifiers = tp->qualifiers;  /* Preserve qualifiers */
        return new_tp;
    }
    return tp;
}

/* Usual arithmetic conversions - implements ANSI C conversion rules */
TYP *usual_arithmetic_conversions(TYP *tp1, TYP *tp2, struct enode **node1, struct enode **node2)
{
    /* First apply integer promotion to both operands */
    tp1 = integer_promote(tp1, node1);
    tp2 = integer_promote(tp2, node2);
    
    /* If either operand is floating point, convert to floating point */
    if (tp1->type == bt_double || tp2->type == bt_double) {
        if (tp1->type != bt_double) {
            if (node1 != NULL) {
                *node1 = makenode(en_cld, *node1, NULL);
                (*node1)->signedflag = 1;
            }
        }
        if (tp2->type != bt_double) {
            if (node2 != NULL) {
                *node2 = makenode(en_cld, *node2, NULL);
                (*node2)->signedflag = 1;
            }
        }
        return &stddouble;
    }
    
    if (tp1->type == bt_float || tp2->type == bt_float) {
        if (tp1->type != bt_float) {
            if (node1 != NULL) {
                *node1 = makenode(en_clf, *node1, NULL);
                (*node1)->signedflag = 1;
            }
        }
        if (tp2->type != bt_float) {
            if (node2 != NULL) {
                *node2 = makenode(en_clf, *node2, NULL);
                (*node2)->signedflag = 1;
            }
        }
        return &stdfloat;
    }
    
    /* Both operands are now integer types after promotion */
    /* If both are unsigned, result is unsigned */
    if ((tp1->type == bt_unsigned || tp1->type == bt_ulong || tp1->type == bt_ulonglong) && 
        (tp2->type == bt_unsigned || tp2->type == bt_ulong || tp2->type == bt_ulonglong)) {
        if (tp1->type == bt_ulonglong || tp2->type == bt_ulonglong) {
            return &stdulonglong;
        }
        return &stdunsigned;
    }
    
    /* If one is unsigned and the other is signed, convert to unsigned */
    if ((tp1->type == bt_unsigned || tp1->type == bt_ulong || tp1->type == bt_ulonglong) || 
        (tp2->type == bt_unsigned || tp2->type == bt_ulong || tp2->type == bt_ulonglong)) {
        if (tp1->type == bt_long) {
            if (node1 != NULL) {
                *node1 = makenode(en_cld, *node1, NULL);
                (*node1)->signedflag = 0;
            }
        }
        if (tp1->type == bt_longlong) {
            if (node1 != NULL) {
                *node1 = makenode(en_clll, *node1, NULL);
                (*node1)->signedflag = 0;
            }
        }
        if (tp2->type == bt_long) {
            if (node2 != NULL) {
                *node2 = makenode(en_cld, *node2, NULL);
                (*node2)->signedflag = 0;
            }
        }
        if (tp2->type == bt_longlong) {
            if (node2 != NULL) {
                *node2 = makenode(en_clll, *node2, NULL);
                (*node2)->signedflag = 0;
            }
        }
        if (tp1->type == bt_ulonglong || tp2->type == bt_ulonglong) {
            return &stdulonglong;
        }
        return &stdunsigned;
    }
    
    /* Both are signed, result is signed */
    if (tp1->type == bt_longlong || tp2->type == bt_longlong) {
        return &stdlonglong;
    }
    return &stdint;
}

extern int          isscalar();
extern void         error();

void
conv_signed( node, cmd )
    struct enode    **node;
    enum e_node     cmd;
{
    if (node != NULL) {
        *node = makenode( cmd, *node, NULL );
        (*node)->signedflag = 1;
    }
}

void
conv_unsigned( node, cmd )
    struct enode    **node;
    enum e_node     cmd;
{
    if (node != NULL) {
        *node = makenode( cmd, *node, NULL );
        (*node)->signedflag = 0;
    }
}

/*
 * asforcefit will coerce the nodes passed into compatable types and return
 * the type of the resulting expression.
 * 
 * NOTE: This routine is only called during assignment statements. It is
 * supposed to make ival = dval a type mismatch instead of promoting ival to
 * a double. e.g. ival*dval and dval*ival
 * 
 */

TYP            *
asforcefit(node1, tp1, node2, tp2)
    struct enode  **node1, **node2;
    TYP            *tp1, *tp2;
{
    node1 = node1;

    switch (tp1->type) {
    case bt_char:
        switch (tp2->type) {
        case bt_char:
        case bt_short:
        case bt_enum:
        case bt_long:
        case bt_longlong:
        case bt_bool:
            return( tp1 );
        case bt_uchar:
        case bt_ushort:
        case bt_unsigned:
        case bt_ulonglong:
            return( tp1 );
        case bt_float:
            conv_signed( node2, en_cfl );
            return( tp1 );
        case bt_double:
            conv_signed( node2, en_cdl );
            return( tp1 );
        }
        break;
    case bt_uchar:
        switch (tp2->type) {
        case bt_char:
        case bt_short:
        case bt_enum:
        case bt_long:
        case bt_bool:
            return( tp1 );
        case bt_uchar:
        case bt_ushort:
        case bt_unsigned:
            return( tp1 );
        case bt_float:
            conv_unsigned( node2, en_cfl );
            return( tp1 );
        case bt_double:
            conv_unsigned( node2, en_cdl );
            return( tp1 );
        }
        break;
    case bt_ushort:
        switch (tp2->type) {
        case bt_char:
            conv_signed( node2, en_cbl );
            return( tp1 );
        case bt_uchar:
            conv_unsigned( node2, en_cbl );
            return( tp1 );
        case bt_ushort:
        case bt_short:
        case bt_enum:
        case bt_long:
        case bt_unsigned:
        case bt_bool:
            return( tp1 );
        case bt_float:
            conv_unsigned( node2, en_cfl );
            return( tp1 );
        case bt_double:
            conv_unsigned( node2, en_cdl );
            return( tp1 );
        }
        break;
    case bt_short:
    case bt_enum:
        switch (tp2->type) {
        case bt_char:
            conv_signed( node2, en_cbl );
            return( tp1 );
        case bt_uchar:
            conv_unsigned( node2, en_cbl );
            return( tp1 );
        case bt_ushort:
        case bt_short:
        case bt_enum:
        case bt_long:
        case bt_unsigned:
            return( tp1 );
        case bt_float:
            conv_signed( node2, en_cfl );
            return( tp1 );
        case bt_double:
            conv_signed( node2, en_cdl );
            return( tp1 );
        }
        break;
    case bt_long:
        switch (tp2->type) {
        case bt_pointer:
        case bt_unsigned:
        case bt_long:
            return( tp1 );
        case bt_char:
            conv_signed( node2, en_cbl );
            return( tp1 );
        case bt_uchar:
            conv_unsigned( node2, en_cbl );
            return( tp1 );
        case bt_short:
        case bt_enum:
            conv_signed( node2, en_cwl );
            return( tp1 );
        case bt_ushort:
            conv_unsigned( node2, en_cwl );
            return( tp1 );
        case bt_float:
            conv_signed( node2, en_cfl );
            return( tp1 );
        case bt_double:
            conv_signed( node2, en_cdl );
            return( tp1 );
        }
        break;
    case bt_pointer:
        if (isscalar(tp2) || tp2->type == bt_pointer)
            return tp1;
        if (tp2->type == bt_ifunc || tp2->type == bt_func)
            return tp1;
        break;
    case bt_unsigned:
        if (isscalar(tp2) || tp2->type == bt_pointer)
            return tp1;
        break;
    case bt_union:
    case bt_struct:
        if (tp1->type == tp2->type)
            return (tp1);
        break;
    case bt_float:
        if (tp1->type == tp2->type)
            return (tp1);
        if (tp2->type != bt_double)
            conv_signed( node2, en_clf );
        else
            conv_signed( node2, en_cdf );
        return (tp1);
        break;
    case bt_double:
        switch (tp2->type) {
        case bt_double:
            return (tp1);
        case bt_float:
            conv_signed( node2, en_cfd );
            return (tp1);
        case bt_long:
            conv_signed( node2, en_cld );
            return (tp1);
        }
        break;
    }
    error(ERR_MISMATCH, NULL);
    return tp1;
}

/*
 * forcefit will coerce the nodes passed into compatable types and return the
 * type of the resulting expression.
 */

TYP            *
forcefit(node1, tp1, node2, tp2)
    struct enode  **node1, **node2;
    TYP            *tp1, *tp2;
{
    /* Handle special cases first */
    if (tp1->type == bt_pointer) {
        if (isscalar(tp2) || tp2->type == bt_pointer)
            return tp1;
        if (tp2->type == bt_ifunc || tp2->type == bt_func)
            return tp1;
        error(ERR_MISMATCH, NULL);
        return tp1;
    }
    
    if (tp2->type == bt_pointer) {
        if (isscalar(tp1))
            return tp2;
        if (tp1->type == bt_ifunc || tp1->type == bt_func)
            return tp2;
        error(ERR_MISMATCH, NULL);
        return tp2;
    }
    
    if (tp1->type == bt_union || tp1->type == bt_struct) {
        if (tp1->type == tp2->type)
            return tp1;
        error(ERR_MISMATCH, NULL);
        return tp1;
    }
    
    if (tp2->type == bt_union || tp2->type == bt_struct) {
        if (tp1->type == tp2->type)
            return tp2;
        error(ERR_MISMATCH, NULL);
        return tp2;
    }
    
    /* Apply array decay first */
    tp1 = array_decay(tp1, node1);
    tp2 = array_decay(tp2, node2);
    
    /* Use usual arithmetic conversions for all other cases */
    return usual_arithmetic_conversions(tp1, tp2, node1, node2);
}