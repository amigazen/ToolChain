
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
#include    "C.h"
#include    "Expr.h"
#include    "Gen.h"
#include    "Cglbdec.h"

extern TYP stdint;
extern TYP stdunsigned;
extern TYP stdchar;
extern TYP stdshort;
extern TYP stdstring;
extern TYP stdfunc;
extern TYP stdfloat;
extern TYP stddouble;

extern struct enode *makenode();
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
            return( tp1 );
        case bt_uchar:
        case bt_ushort:
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
    case bt_uchar:
        switch (tp2->type) {
        case bt_char:
        case bt_short:
        case bt_enum:
        case bt_long:
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
    switch (tp1->type) {
    case bt_char:
        switch (tp2->type) {
        case bt_char:
            conv_signed( node1, en_cbl );
            conv_signed( node2, en_cbl );
            return (&stdint);
        case bt_uchar:
            conv_unsigned( node1, en_cbl );
            conv_unsigned( node2, en_cbl );
            return (&stdunsigned);
        case bt_ushort:
            conv_unsigned( node1, en_cbl );
            conv_unsigned( node2, en_cwl );
            return (&stdunsigned);
        case bt_short:
        case bt_enum:
            conv_signed( node1, en_cbl );
            conv_signed( node2, en_cwl );
            return (&stdint);
        case bt_long:
            conv_signed( node1, en_cbl );
            return (&stdint);
        case bt_pointer:
            conv_unsigned( node1, en_cbl );
            return (tp2);
        case bt_unsigned:
            conv_unsigned( node1, en_cbl );
            return (&stdunsigned);
        case bt_float:
            conv_signed( node2, en_cfd );
            /* FALL THRU */
        case bt_double:
            conv_signed( node1, en_cbl );
            conv_signed( node1, en_cld );
            return (&stddouble);
        }
        break;
    case bt_uchar:
        switch (tp2->type) {
        case bt_char:
        case bt_uchar:
            conv_unsigned( node1, en_cbl );
            conv_unsigned( node2, en_cbl );
            return (&stdunsigned);
        case bt_short:
        case bt_ushort:
        case bt_enum:
            conv_unsigned( node1, en_cbl );
            conv_unsigned( node2, en_cwl );
            return (&stdunsigned);
        case bt_pointer:
            conv_unsigned( node1, en_cbl );
            return (tp2);
        case bt_unsigned:
        case bt_long:
            conv_unsigned( node1, en_cbl );
            if (node2 != NULL) 
                (*node2)->signedflag = 0;
            return (&stdunsigned);
        case bt_float:
            conv_signed( node2, en_cfd );
        case bt_double:
            conv_unsigned( node1, en_cbl );
            conv_signed( node1, en_cld );
            return (&stddouble);
        }
        break;
    case bt_ushort:
        switch (tp2->type) {
        case bt_char:
        case bt_uchar:
            conv_unsigned( node1, en_cwl );
            conv_unsigned( node2, en_cbl );
            return (&stdunsigned);
        case bt_short:
        case bt_ushort:
        case bt_enum:
            conv_unsigned( node1, en_cwl );
            conv_unsigned( node2, en_cwl );
            return (&stdunsigned);
        case bt_pointer:
            conv_unsigned( node1, en_cwl );
            return (tp2);
        case bt_unsigned:
        case bt_long:
            conv_unsigned( node1, en_cwl );
            if (node2 != NULL)
                (*node2)->signedflag = 0;
            return (&stdunsigned);
        case bt_float:
            conv_signed( node2, en_cfd );
            /* FALL THRU */
        case bt_double:
            conv_unsigned( node1, en_cwl );
            conv_signed( node1, en_cld );
            return (&stddouble);
        }
        break;
    case bt_short:
    case bt_enum:
        switch (tp2->type) {
        case bt_uchar:
            conv_unsigned( node1, en_cwl );
            conv_unsigned( node2, en_cbl );
            return (&stdunsigned);
        case bt_char:
            conv_signed( node1, en_cwl );
            conv_signed( node2, en_cbl );
            return (&stdint);
        case bt_ushort:
            conv_unsigned( node1, en_cwl );
            conv_unsigned( node2, en_cwl );
            return (&stdunsigned);
        case bt_short:
        case bt_enum:
            conv_signed( node1, en_cwl );
            conv_signed( node2, en_cwl );
            return (&stdint);
        case bt_pointer:
            conv_unsigned( node1, en_cwl );
            return (tp2);
        case bt_unsigned:
            conv_unsigned( node1, en_cwl );
            if (node2 != NULL)
                (*node2)->signedflag = 0;
            return (&stdunsigned);
        case bt_long:
            conv_signed( node1, en_cwl );
            return (&stdint);
        case bt_float:
            conv_signed( node2, en_cfd );
            /* FALL THRU */
        case bt_double:
            conv_signed( node1, en_cwl );
            conv_signed( node1, en_cld );
            return (&stddouble);
        }
        break;
    case bt_long:
        switch (tp2->type) {
        case bt_long:
            return( tp1 );
        case bt_char:
            conv_signed( node2, en_cbl );
            return (&stdint);
        case bt_uchar:
            conv_unsigned( node2, en_cbl );
            if (node1 != NULL)
                (*node1)->signedflag = 0;
            return (&stdunsigned);
        case bt_ushort:
            conv_unsigned( node2, en_cwl );
            if (node1 != NULL)
                (*node1)->signedflag = 0;
            return (&stdunsigned);
        case bt_short:
        case bt_enum:
            conv_signed( node2, en_cwl );
            return (&stdint);
        case bt_unsigned:
            if (node1 != NULL)
                (*node1)->signedflag = 0;
            return (&stdunsigned);
        case bt_pointer:
            return tp2;
        case bt_float:
            conv_signed( node2, en_cfd );
            /* FALL THRU */
        case bt_double:
            conv_signed( node1, en_cld );
            return (&stddouble);
            break;
        }
        break;
    case bt_pointer:
        if (isscalar(tp2) || tp2->type == bt_pointer)
            return tp1;
        if (tp2->type == bt_ifunc || tp2->type == bt_func)
            return tp1;
        break;
    case bt_unsigned:
        switch (tp2->type) {
        case bt_pointer:
            return tp2;
        case bt_uchar:
        case bt_char:
            conv_unsigned( node2, en_cbl );
            return (&stdunsigned);
        case bt_ushort:
        case bt_short:
        case bt_enum:
            conv_unsigned( node2, en_cwl );
            return (&stdunsigned);
        case bt_unsigned:
            return( tp1 );
        case bt_long:
            if (node2 != NULL)
                (*node2)->signedflag = 0;
            return (&stdunsigned);
        case bt_float:
            conv_signed( node2, en_cfd );
            /* FALL THRU */
        case bt_double:
            conv_signed( node1, en_cld );
            return (&stddouble);
        }
        break;
    case bt_union:
    case bt_struct:
        if (tp1->type == tp2->type)
            return (tp1);
        break;
    case bt_float:
        switch (tp2->type) {
        case bt_float:
            return (tp1);
        case bt_double:
            conv_signed( node1, en_cfd );
            return (&stddouble);
        case bt_long:
            conv_signed( node2, en_clf );
            return (tp1);
        }
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


