
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
 * $Log:	Builtins.c,v $
 * Revision 3.33  90/04/04  02:26:59  lionel
 * *** empty log message ***
 * 
 * Revision 3.32  90/02/03  16:22:54  lionel
 * None
 * 
 *------------------------------------------------------------------
 */

/*
 * Builtins.c
 * 
 * Code generation for BUILTIN routines.
 */

#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        "C.h"
#include        "Expr.h"
#include        "Gen.h"
#include        "Cglbdec.h"

extern SYM   *gsearch();
extern void  *xalloc();
extern struct enode *makenode();
extern struct amode *makedreg(), *makeareg();
extern struct amode *temp_data(), *temp_addr();
extern struct amode *request_data(), *request_addr();
extern struct amode *make_immed(), *make_label();
extern struct amode *make_mask();

extern struct amode push[], pop[];

void            gen_strlen(), gen_strcmp(), gen_strcpy(), gen_strcat();
void            gen_memset(), gen_memcpy();

struct bfuncs {
    int             args;
    char           *name;
    void            (*func) ();
};

struct bfuncs   bfuncs_table[] = {
    {4, "__BUILTIN_strlen", gen_strlen},
    {8, "__BUILTIN_strcmp", gen_strcmp},
    {8, "__BUILTIN_strcat", gen_strcat},
    {8, "__BUILTIN_strcpy", gen_strcpy},
    {8, "__BUILTIN_bzero", gen_memset},
    {12, "__BUILTIN_bcopy", gen_memcpy},
    {0, NULL, NULL}
};

void
gen_strcat(int arg)
{
    int             looplab1, looplab2;
    struct amode   *ap1, *ap2, *ap3;

    looplab1 = nextlabel++;
    looplab2 = nextlabel++;

    ap1 = request_data(0);
    ap2 = temp_addr();
    ap3 = temp_addr();

    gen_code(op_move, 4, pop, ap2); /* dest                      */
    gen_code(op_move, 4, pop, ap3); /* src                       */
    gen_code(op_move, 4, ap2, ap1); /* return the destination    */

    gen_label(looplab1);
    ap2->mode = am_ainc;
    gen_code(op_tst, 1, ap2, NULL); /* skip start of string      */
    gen_code(op_bne, 0, make_label(looplab1), NULL);

    ap2->mode = am_areg;    /* dec so it points at the 0 */
    gen_code(op_sub, 4, make_immed(1), ap2);

    ap2->mode = am_ainc;
    ap3->mode = am_ainc;

    gen_label(looplab2);
    gen_code(op_move, 1, ap3, ap2); /* src to dest until zero    */
    gen_code(op_bne, 0, make_label(looplab2), NULL);

    freeop(ap3);
    freeop(ap2);
    freeop(ap1);
}

void
gen_strlen(int arg)
{
    int             looplab;
    struct amode   *ap1, *ap2, *ap3;

    looplab = nextlabel++;

    ap1 = request_data(0);
    ap2 = temp_addr();
    ap3 = temp_addr();

    gen_code(op_move, 4, pop, ap2); /* src               */
    gen_code(op_move, 4, ap2, ap3); /* save src          */
    gen_label(looplab);
    ap3->mode = am_ainc;
    gen_code(op_tst, 1, ap3, NULL); /* find the end      */
    gen_code(op_bne, 0, make_label(looplab), NULL);
    ap3->mode = am_areg;
    gen_code(op_sub, 4, ap2, ap3);  /* sub the front     */
    gen_code(op_sub, 4, make_immed(1), ap3);    /* sub for the '\0'  */
    gen_code(op_move, 4, ap3, ap1); /* return length     */

    freeop(ap3);
    freeop(ap2);
    freeop(ap1);
}

void
gen_strcmp(int arg)
{
    int             looplab1, looplab2, looplab3;
    struct amode   *ap1, *ap2, *ap3, *ap4;

    looplab1 = nextlabel++;
    looplab2 = nextlabel++;
    looplab3 = nextlabel++;

    ap1 = request_data(0);
    ap2 = temp_addr();
    ap3 = temp_addr();
    ap4 = request_data(1);

    gen_code(op_move, 4, pop, ap2); /* dest                      */
    gen_code(op_move, 4, pop, ap3); /* src                       */

    gen_label(looplab1);

    ap3->mode = am_ind;
    gen_code(op_move, 1, ap3, ap1); /* fetch the source char     */

    ap2->mode = am_ainc;
    gen_code(op_cmp, 1, ap1, ap2);  /* compare with and inc dest     */
    gen_code(op_bne, 0, make_label(looplab2), NULL);

    ap3->mode = am_ainc;
    gen_code(op_tst, 1, ap3, NULL); /* If we still have string left  */
    gen_code(op_bne, 0, make_label(looplab1), NULL);

    gen_code(op_clr, 4, ap1, NULL);
    gen_code(op_bra, 0, make_label(looplab3), NULL);

    gen_label(looplab2);

    ap3->mode = am_ind;
    ap2->mode = am_adec;

    gen_code(op_move, 1, ap3, ap4); /* Fetch the source char     */
    gen_code(op_ext, 2, ap4, NULL);
    gen_code(op_ext, 4, ap4, NULL); /* extend to an integer      */

    gen_code(op_move, 1, ap2, ap1); /* Fetch the deste char      */
    gen_code(op_ext, 2, ap1, NULL);
    gen_code(op_ext, 4, ap1, NULL); /* extend to an integer      */

    gen_code(op_sub, 4, ap4, ap1);  /* compute the sign of result    */

    gen_label(looplab3);

    freeop(ap4);
    freeop(ap3);
    freeop(ap2);
    freeop(ap1);
}

void
gen_strcpy(int arg)
{
    int             looplab;
    struct amode   *ap1, *ap2, *ap3;

    looplab = nextlabel++;

    ap1 = request_data(0);
    ap2 = temp_addr();
    ap3 = temp_addr();

    gen_code(op_move, 4, pop, ap2); /* dest              */
    gen_code(op_move, 4, pop, ap3); /* src               */
    gen_code(op_move, 4, ap2, ap1); /* return dest       */

    gen_label(looplab);

    ap2->mode = am_ainc;
    ap3->mode = am_ainc;

    gen_code(op_move, 1, ap3, ap2); /* src to dest until zero */
    gen_code(op_bne, 0, make_label(looplab), NULL);

    freeop(ap3);
    freeop(ap2);
    freeop(ap1);
}

void
gen_memset(int arg)
{
    /* Use standard memset library function instead of custom implementation */
    gen_libcall("memset", 3);
}

void
gen_memcpy(int arg)
{
    /* Use standard memcpy library function instead of custom implementation */
    gen_libcall("memcpy", 3);
}

int
gen_builtins(struct enode *node, int argbytes)
{
    char           *fname;
    void            (*xfunc)(int);
    struct bfuncs  *ptr;

    if (node->v.p[0]->nodetype == en_nacon) {
        fname = node->v.p[0]->v.sp;
        if (strncmp( "__BUILTIN_", fname, 10) == 0) {
            for (ptr = bfuncs_table; ptr->name != NULL; ++ptr) {
                if (strcmp(fname, ptr->name) == 0) {
                    xfunc = ptr->func;
                    (*xfunc) (argbytes);
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

static int
hexval(int ch)
{
    if ((ch >= '0') && (ch <= '9'))
        return( ch - '0' );
    else if ((ch >= 'a') && (ch <= 'f'))
        return( ch - 'a' + 10 );
    else if ((ch >= 'A') && (ch <= 'F'))
        return( ch - 'A' + 10 );
    return( -1 );
}

void
ref_base(char *base_name)
{
    SYM            *sp;

    sp = gsearch(base_name);
    if (sp == NULL) {
        ++global_flag;
        sp = (SYM *) xalloc(sizeof(SYM));
        sp->tp = NULL;
        sp->name = base_name;
        sp->storage_class = sc_external;
        sp->storage_type = sc_external;
        sp->value.i = 1;/* Make it referenced   */
        insert(sp, &gsyms);
        --global_flag;
    }
}

void
gen_libcall2(struct enode *node, int argbytes, struct libcall *ptr)
{
    struct amode    *ap1, *ap2;
    char            *cp;
    int             i, len;
    int             count, mask, reg, liboffset, num, ret;

    len = strlen( ptr->args );

    num = hexval( ptr->args[len-1] );
    ret = hexval( ptr->args[len-2] );

    num &= 0x7;

    cp = ptr->args;
    mask = count = 0;
    for (i = 0; i < num; ++i) {
        reg = hexval( *cp++ );
        if ((reg >= 3 && reg <= 7) || (reg >= 10 && reg <= 15)) {
            ++count;
            mask |= 1 << reg;
        }
    }
    if (Options.Frame == 6) {
        ++count;
        mask |= 1 << 14;
    }

    if (mask != 0) 
        gen_code( op_movem, 4, make_mask(mask), push );
    
    ap1 = makeareg(7);
    ap1->mode = am_indx;

    ap2 = makeareg(0);

    for (i = 0; i < num; ++i) {
        reg = hexval( ptr->args[i] );
        ap1->offset = makenode( en_icon, 4*(i+count), NULL );
        ap2->mode = (reg < 8 ? am_dreg : am_areg );
        ap2->preg = (enum e_am) (reg % 8);
        gen_code( op_move, 4, ap1, ap2 );
    }

    ref_base( ptr->basename );

    ap1->mode = am_direct;
    ap1->offset = makenode( en_nacon, ptr->basename, NULL );
    ap2 = makeareg(6);

    gen_code(op_move, 4, ap1, ap2 );

    liboffset =  0;
    for (cp = ptr->offset; *cp; ++cp) 
        liboffset = (liboffset * 16) + hexval( *cp );

    ap1 = makeareg(6);
    ap1->mode = am_indx;
    ap1->offset = makenode( en_icon, -liboffset, NULL );

    gen_code( op_jsr, 0, ap1, NULL );

    if (ret != 0) {
        if (ret < 8)
            gen_code( op_move, 4, makedreg(ret), makedreg(0));
        else
            gen_code( op_move, 4, makeareg(ret-8), makedreg(0));
    }

    if (mask != 0)
        gen_code( op_movem, 4, pop, make_mask(mask));

    if (num > 0)
        gen_code( op_add, 2, make_immed(num*4), makeareg(7));
}

int
gen_libcall(struct enode *node, int argbytes)
{
    char           *fname;
    struct libcall  *ptr = libpragma;

    if (node->v.p[0]->nodetype == en_nacon) {
        fname = node->v.p[0]->v.sp;
        if (strncmp( "__LIBCALL_", fname, 10) == 0) {
            for (ptr = libpragma; ptr != NULL; ptr = ptr->next) {
                if (strcmp(&fname[10], ptr->funcname) == 0) {
                    gen_libcall2( node, argbytes, ptr );
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}
