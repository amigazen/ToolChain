
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
 * $Log:	Decl.c,v $
 * Revision 3.33  90/04/04  23:17:41  lionel
 * Fixed parsing of multidimensional arrays.
 * 
 * Revision 3.32  90/02/03  16:23:20  lionel
 * None
 * 
 *------------------------------------------------------------------
 */

/*
 * Decl.c
 * 
 * Manages typedef's and variable declarations in the symbol table.
 */

#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    "C.h"
#include    "Expr.h"
#include    "Gen.h"
#include    "Cglbdec.h"

TYP            *head = NULL;
TYP            *tail = NULL;
char           *declid = NULL;
TABLE           tagtable = {NULL, NULL};
TYP             stdconst = {bt_long, 1, 4, {NULL, NULL}, 0, "const", QUAL_CONST};

void    decl2(), declenum(), enumbody(), declstruct(), structbody();
void    apply_qualifiers();

extern SYM      *search();
extern long     intexpr();
extern int      fatal;

#ifdef  GENERATE_DBX
extern int      dbx_ident();
#endif

extern struct snode *asmstmt();
extern char    *xalloc();

int
imax(int i, int j)
{
    return (i > j) ? i : j;
}

char           *
litlate(char *s)
{
    char           *p;

    p = (char *) xalloc(strlen(s) + 1);
    strcpy(p, s);
    return p;
}

SYM            *
copysym(SYM *sp)
{
    SYM            *esp;
    int             siz;

    siz = sizeof(SYM);
    ++global_flag;
    esp = (SYM *) xalloc(siz);
    --global_flag;

    if (esp != NULL) {
        *esp = *sp;
        esp->next = NULL;
    }
    return (esp);
}


TYP            *
maketype(enum e_bt bt, int siz)
{
    TYP            *tp;

    tp = (TYP *) xalloc(sizeof(TYP));
    tp->val_flag = 0;
    tp->size = siz;
    tp->type = bt;
    tp->sname = NULL;
    tp->lst.head = NULL;
    tp->lst.tail = NULL;
    tp->btp = NULL;
    tp->qualifiers = 0;  /* Initialize qualifiers to none */
    tp->mem_section = 0; /* Initialize memory section to none */
    return tp;
}

TYP            *
istypedef(TABLE *table)
{
    SYM            *sp;

    if (lastst == id) {
        if (table != NULL) {
            sp = search(lastid, table->head);
            if (sp != NULL && sp->tp != NULL && sp->tp->type == bt_typedef)
                return (sp->tp->btp);
        }
        sp = search(lastid, lsyms.head);
        if (sp != NULL && sp->tp != NULL && sp->tp->type == bt_typedef)
            return (sp->tp->btp);
        sp = search(lastid, gsyms.head);
        if (sp != NULL && sp->tp != NULL && sp->tp->type == bt_typedef)
            return (sp->tp->btp);
    }
    return (NULL);
}

int
is_class_error(void)
{
    if (lastst == kw_auto) {
        error( ERR_SYNTAX, "auto keyword in wrong position" );
        getsym();
    }
    else if (lastst == kw_const) {
        error( ERR_SYNTAX, "const keyword in wrong position" );
        getsym();
    }
    else if (lastst == kw_static) {
        error( ERR_SYNTAX, "static keyword in wrong position" );
        getsym();
    }
    else if (lastst == kw_register) {
        error( ERR_SYNTAX, "register keyword in wrong position" );
        getsym();
    }
    return (0);
}

void
decl(TABLE *table)
{
    TYP            *tp;

    switch (lastst) {
    case kw_typedef:
        getsym();
        fprintf( stderr, "DIAG -- SHOULD NEVER HAPPEN\n" );
        break;
    case kw_auto:
        getsym();
        decl(table);
        break;
    case kw_const:
        getsym();
        decl(table);
        if (head != NULL)
            head->qualifiers |= QUAL_CONST;
        break;
    case kw_volatile:
        getsym();
        decl(table);
        if (head != NULL)
            head->qualifiers |= QUAL_VOLATILE;
        break;
    case kw_chip:
        getsym();
        decl(table);
        if (head != NULL)
            head->mem_section = MEM_CHIP;
        break;
    case kw_far:
        getsym();
        decl(table);
        if (head != NULL)
            head->mem_section = MEM_FAR;
        break;
    case kw_near:
        getsym();
        decl(table);
        if (head != NULL)
            head->mem_section = MEM_NEAR;
        break;
    case kw_fast:
        getsym();
        decl(table);
        if (head != NULL)
            head->mem_section = MEM_FAST;
        break;
    case kw_interrupt:
        getsym();
        decl(table);
        if (head != NULL)
            head->qualifiers |= QUAL_INTERRUPT;
        break;
    case kw_regargs:
        getsym();
        decl(table);
        if (head != NULL)
            head->qualifiers |= QUAL_REGARGS;
        break;
    case kw_stdargs:
        getsym();
        decl(table);
        if (head != NULL)
            head->qualifiers |= QUAL_STDARGS;
        break;
    case kw_char:
        head = tail = maketype(bt_char, 1);
        getsym();
        is_class_error();
        break;
    case kw_short:
        head = tail = maketype(bt_short, 2);
        getsym();
        if (lastst == kw_int)
            getsym();
        is_class_error();
        break;
    case kw_long:
        head = tail = maketype(bt_long, 4);
        getsym();
        if (lastst == kw_int)
            getsym();
        else if (lastst == kw_long) {
            /* long long - 64-bit integer */
            head = tail = maketype(bt_longlong, 8);
            getsym();
        }
        is_class_error();
        break;
    case kw_int:
    case kw_void:
        head = tail = maketype(bt_long, 4);
        getsym();
        is_class_error();
        break;
    case kw_bool:
        head = tail = maketype(bt_bool, 1);
        getsym();
        is_class_error();
        break;
    case kw_signed:
        getsym();
        switch (lastst) {
        case kw_char:
            head = tail = maketype(bt_char, 1);
            getsym();
            break;
        case kw_short:
            head = tail = maketype(bt_short, 2);
            getsym();
            break;
        case kw_int:
        case kw_long:
            head = tail = maketype(bt_long, 4);
            getsym();
            break;
        }
        is_class_error();
        break;
    case kw_unsigned:
        getsym();
        switch (lastst) {
        case kw_char:
            head = tail = maketype(bt_uchar, 1);
            getsym();
            break;
        case kw_short:
            head = tail = maketype(bt_ushort, 2);
            getsym();
            break;
        case kw_int:
        case kw_long:
            head = tail = maketype(bt_unsigned, 4);
            getsym();
            if (lastst == kw_long) {
                /* unsigned long long - 64-bit unsigned integer */
                head = tail = maketype(bt_ulonglong, 8);
                getsym();
            }
            break;
        default:
            head = tail = maketype(bt_unsigned, 4);
            break;
        }
        is_class_error();
        break;
    case id:        /* no type declarator, or could be a typedef  */
        if ((tp = istypedef(table)) != NULL) {
            head = tail = tp;
            getsym();
        }
        else
            head = tail = maketype(bt_long, 4);
        break;
    case kw_float:
        head = tail = maketype(bt_float, 4);
        getsym();
        is_class_error();
        break;
    case kw_double:
        head = tail = maketype(bt_double, 8);
        getsym();
        is_class_error();
        break;
    case kw_enum:
        getsym();
        declenum(table);
        is_class_error();
        break;
    case kw_struct:
        getsym();
        declstruct(bt_struct);
        is_class_error();
        break;
    case kw_union:
        getsym();
        declstruct(bt_union);
        is_class_error();
        break;
    }
}

void
decl1(void)
{
    TYP            *temp1, *temp2, *temp3, *temp4;

    switch (lastst) {
    case id:
        declid = litlate(lastid);
        getsym();
        decl2();
        break;
    case star:
        temp1 = maketype(bt_pointer, 4);
        temp1->btp = head;
        head = temp1;
        if (tail == NULL)
            tail = head;
        getsym();
        decl1();
        break;
    case openpa:
        getsym();
        temp1 = head;
        temp2 = tail;
        head = tail = NULL;
        decl1();
        needpunc(closepa);
        temp3 = head;
        temp4 = tail;
        head = temp1;
        tail = temp2;
        decl2();
        if (temp4 != NULL) {
            temp4->btp = head;
            if (temp4->type == bt_pointer && temp4->val_flag != 0
                && head != NULL)
                temp4->size *= head->size;
        }
        head = temp3;
        break;
    default:
        decl2();
        break;
    }
}

void
decl2(void)
{
    TYP            *temp1;

    switch (lastst) {
    case openbr:
        getsym();
        temp1 = maketype(bt_pointer, 4);
        temp1->val_flag = 1;
        if (lastst == closebr) {
            temp1->size = 0;
            temp1->val_flag = 1;
            getsym();
        }
        else {
            temp1->size = intexpr();
            needpunc(closebr);
        }
        decl2();
        if (head != NULL) {
            temp1->size *= head->size;
        }
        temp1->btp = head;
        head = temp1;
        if (tail == NULL)
            tail = head;
        break;
    case openpa:
        getsym();
        temp1 = maketype(bt_func, 0);
        temp1->val_flag = 1;
        temp1->btp = head;
        head = temp1;
        if (lastst == closepa) {
            getsym();
            if (lastst == begin)
                temp1->type = bt_ifunc;
        }
        else
            temp1->type = bt_ifunc;
        break;
    }
}

int
alignment(TYP *tp)
{
    if (tp == NULL) {
        fprintf( stderr, "DIAG -- NULL argument to alignment.\n" );
        return (AL_CHAR);
    }
    switch (tp->type) {
    case bt_char:
    case bt_uchar:
    case bt_bool:
        return AL_CHAR;
    case bt_short:
    case bt_ushort:
        return AL_SHORT;
    case bt_long:
    case bt_unsigned:
        return AL_LONG;
    case bt_longlong:
    case bt_ulonglong:
        return AL_LONG;  /* 8-byte alignment on 68000 */
    case bt_enum:
        return AL_SHORT;
    case bt_pointer:
        if (tp->val_flag)
            return alignment(tp->btp);
        else
            return AL_POINTER;
    case bt_float:
        return AL_FLOAT;
    case bt_double:
        return AL_DOUBLE;
    case bt_struct:
    case bt_union:
        return AL_STRUCT;
    default:
        return AL_CHAR;
    }
}

int
declare(TABLE *table, enum e_sc al, int ilc, enum e_bt ztype, enum e_sc ral)

/*
 * process declarations of the form:
 * 
 * <type>  <decl>, <decl>...;
 * 
 * leaves the declarations in the symbol table pointed to by table and returns
 * the number of bytes declared. al is the allocation type to assign, ilc is
 * the initial location counter. if al is sc_member then no initialization
 * will be processed. ztype should be bt_struct for normal and in structure
 * declarations and sc_union for in union declarations.
 * 
 * If al == sc_auto && ral == sc_member then we are processing parameters
 * 
 */
{
    SYM            *sp, *sp1;
    TYP            *dhead, *tp2;
    int             nbytes = 0, num;

    if (lastst == kw_typedef) {
        getsym();
        decl(table);
        dhead = head;
        for (;;) {
            declid = NULL;
            decl1();
            if (declid != NULL) {
                sp = (SYM *) xalloc(sizeof(SYM));
                sp->name = declid;
                sp->storage_class = sc_type;
                sp->value.i = 0;
                tp2 = maketype(bt_typedef, 0);
                tp2->btp = head;
                tp2->size = head->size;
                head = tp2;
                sp->tp = head;
                insert(sp, table);
#ifdef  GENERATE_DBX
                dbx_ident(sp);
#endif
            }
            if (lastst != comma)
                break;
            needpunc(comma);
            head = dhead;
        }
        needpunc(semicolon);
        return (0);
    }

    decl(table);
    dhead = head;
    for (;;) {
        declid = NULL;
        decl1();

        if (head == NULL) {
            error( ERR_SYNTAX, "unknown type" );
            break;
        }
        
        if (ral == sc_member && al == sc_auto || ral == sc_parameter) {
            ral = sc_parameter; /* Parameter Decl */
            switch (head->type) {
            case bt_uchar:
            case bt_ushort:
                head = tail = maketype(bt_unsigned, 4);
                break;
            case bt_char:
            case bt_short:
            case bt_enum:
                head = tail = maketype(bt_long, 4);
                break;
            case bt_float:
                head = tail = maketype(bt_double, 8);
                break;
            }
            if (head->size == 0 || head->type == bt_pointer)
                head->val_flag = 0; /* Make *argv[] work */
        }

        if (declid != NULL) {   /* otherwise just struct tag... */
            sp = (SYM *) xalloc(sizeof(SYM));
            sp->name = declid;
            sp->storage_class = al;
            sp->storage_type = ral;
            sp->next = NULL;
            sp->value.i = 0;
            num = alignment(head);
            if ((ilc + nbytes) % num != 0) {
                if (al != sc_member && al != sc_external && al != sc_auto) {
                    dseg();
                    genalignment(num);
                }
                while ((ilc + nbytes) % num != 0)
                    ++nbytes;
            }
            sp->tp = head;
            if (al == sc_static) {
                sp->value.i = nextlabel++;
                if (sp->tp->type == bt_ifunc) {
                    sp1 = search(sp->name, table->head);
                    if (sp1 != NULL && sp1->tp->type == bt_func) {
                        sp->value.i = sp1->value.i;
                    }
                }
            }
            else if (ztype == bt_union)
                sp->value.i = ilc;
            else if (al != sc_auto)
                sp->value.i = ilc + nbytes;
            else
                sp->value.i = -(ilc + nbytes + head->size);

            if (sp->tp->type == bt_func) {
                if (sp->storage_class == sc_global || 
                    sp->storage_class == sc_auto) {
                    sp->storage_class = sc_external;
                }
            }

            if (ztype == bt_union)
                nbytes = imax(nbytes, sp->tp->size);
            else if (al != sc_external)
                nbytes += sp->tp->size;

            if (sp->tp->type != bt_ifunc) 
                insert(sp, table);
            else {
                sp1 = search(sp->name, table->head);
                if (sp1 == NULL || sp1->tp->type != bt_func) 
                    insert(sp, table);
                else {
#if 0
                    if ((sp1->storage_class != sp->storage_class) ||
                        (sp1->storage_type  != sp->storage_type ))
                        error( ERR_SYNTAX, "storage class is not consistant" );
#endif
                    sp1->tp = sp->tp;
                    sp1->storage_class = sp->storage_class;
                    sp1->storage_type  = sp->storage_type;
                    sp1->value.i = sp->value.i;
                    sp = sp1;
                }
            }

            if (al == sc_auto)
                doinitauto(sp);

            if ((al == sc_global || al == sc_static) &&
                (sp->tp->type != bt_func && sp->tp->type != bt_ifunc))
                doinit(sp);

#ifdef  GENERATE_DBX
            if (ral != sc_parameter)
                dbx_ident(sp);
#endif

            if (sp->tp->type == bt_ifunc) { /* function body follows */
                funcbody(sp);
                return nbytes;
            }
        }

        if (lastst == semicolon)
            break;

        needpunc(comma);

        if (!declbegin(table, lastst))
            break;
        head = dhead;
    }
    getsym();
    return nbytes;
}

int
declbegin(TABLE *table, enum e_sym st)
{
    TYP            *tp;

    tp = istypedef(table);

    if (tp != NULL)
        return (FALSE);

    return st == star || st == id || st == openpa || st == openbr;
}

void
declenum(TABLE *table)
{
    SYM            *sp;

    ++global_flag;

    if (lastst == id) {
        if ((sp = search(lastid, tagtable.head)) == NULL) {
            sp = (SYM *) xalloc(sizeof(SYM));
            memset( (char *)sp, 0, sizeof(SYM));
            sp->storage_class = sc_type;
            sp->name = litlate(lastid);
            sp->tp = (TYP *) xalloc(sizeof(TYP));
            sp->tp->type = bt_enum;
            sp->tp->size = 2;
            sp->tp->lst.head = NULL;
            sp->tp->lst.tail = NULL;
            sp->tp->btp = NULL;
            sp->tp->sname = sp->name;
            getsym();
            if (lastst != begin)
                error(ERR_INCOMPLETE, NULL);
            else {
                insert(sp, &tagtable);
                getsym();
                enumbody(sp, table);
#ifdef  GENERATE_DBX
                dbx_ident(sp);
#endif
            }
        }
        else
            getsym();
        head = sp->tp;
    }
    else {
        sp = (SYM *) xalloc(sizeof(SYM));
        memset( (char *)sp, 0, sizeof(SYM) );
        sp->storage_class = sc_type;
        sp->name = NULL;
        sp->tp = (TYP *) xalloc(sizeof(TYP));
        sp->tp->type = bt_short;
        sp->tp->size = 2;
        sp->tp->lst.head = NULL;
        sp->tp->lst.tail = NULL;
        sp->tp->btp = NULL;
        sp->tp->sname = NULL;
        if (lastst != begin)
            error(ERR_INCOMPLETE, NULL);
        else {
            getsym();
            enumbody(sp, table);
#ifdef  GENERATE_DBX
            dbx_ident(sp);
#endif
        }
        head = sp->tp;
    }

    --global_flag;
}

void
enumbody(SYM *sym_sp, TABLE *table)
{
    int             evalue;
    SYM            *sp, *sp2;

    evalue = 0;
    while (lastst == id) {
        sp = (SYM *) xalloc(sizeof(SYM));
        sp->name = litlate(lastid);
        sp->storage_class = sc_const;
        sp->value.i = evalue++;
        sp->tp = &stdconst;
        insert(sp, table);
        sp2 = copysym(sp);
        insert(sp2, &(sym_sp->tp->lst));
        getsym();   /* Skip the identifier */
        if (lastst == assign) {
            getsym();
            sp->value.i = sp2->value.i = intexpr();
            evalue = (sp->value.i) + 1;
        }
        if (lastst == comma)
            getsym();
        else if (lastst != end)
            break;
    }
    needpunc(end);
}

void
declstruct(enum e_bt ztype)

/*
 * declare a structure or union type. ztype should be either bt_struct or
 * bt_union.
 */
{
    SYM            *sp;

    ++global_flag;

    if (lastst == id) {
        if ((sp = search(lastid, tagtable.head)) == NULL) {
            sp = (SYM *) xalloc(sizeof(SYM));
            memset((char *)sp, 0, sizeof(SYM));
            sp->name = litlate(lastid);
            sp->storage_class = sc_type;
            sp->next = NULL;
            sp->tp = (TYP *) xalloc(sizeof(TYP));
            sp->tp->type = ztype;
            sp->tp->lst.head = NULL;
            sp->tp->sname = sp->name;
            getsym();
            if (lastst == star || lastst == semicolon) {
                sp->tp->size = 0;   /* Forward declaration */
                insert(sp, &tagtable);
            }
            else if (lastst != begin)
                error(ERR_INCOMPLETE, NULL);
            else {
                insert(sp, &tagtable);
                getsym();
                structbody(sp->tp, ztype);
#ifdef  GENERATE_DBX
                dbx_ident(sp);
#endif
            }
        }
        else {
            if (sp->tp->size != 0)
                getsym();
            else {
                sp->tp->type = ztype;
                sp->tp->lst.head = NULL;
                sp->storage_class = sc_type;
                sp->tp->sname = sp->name;
                getsym();
                if (lastst == star || lastst == semicolon)
                    sp->tp->size = 0;   /* Forward declaration */
                else if (lastst != begin)
                    error(ERR_INCOMPLETE, NULL);
                else {
                    getsym();
                    structbody(sp->tp, ztype);
#ifdef  GENERATE_DBX
                    dbx_ident(sp);
#endif
                }
            }
        }
        head = sp->tp;
    }
    else {
        sp = (SYM *) xalloc(sizeof(SYM));
        sp->name = NULL;
        sp->storage_class = sc_type;
        sp->next = NULL;
        sp->tp = (TYP *) xalloc(sizeof(TYP));
        sp->tp->type = ztype;
        sp->tp->sname = NULL;
        sp->tp->lst.head = NULL;
        sp->tp->lst.tail = NULL;
        if (lastst != begin)
            error(ERR_INCOMPLETE, NULL);
        else {
            getsym();
            structbody(sp->tp, ztype);
#ifdef  GENERATE_DBX
            dbx_ident(sp);
#endif
        }
        head = sp->tp;
    }

    --global_flag;
}

void
structbody(TYP *tp, enum e_bt ztype)
{
    int             slc;

    if (tp == NULL) {
        fprintf( stderr, "DIAG -- NULL argument to structbody.\n" );
        return;
    }
    slc = 0;
    tp->val_flag = 1;
    tp->size = -1;
    while (lastst != end) {
        if (ztype == bt_struct)
            slc += declare(&(tp->lst), sc_member, slc, ztype, sc_member);
        else
            slc = imax(slc, declare(&tp->lst, sc_member, 0, ztype, sc_member));
    }

    /*
     * The size of a structure includes padding, which should be a power
     * of 2 (for the implementation below
     */
    tp->size = (slc + AL_STRUCT - 1) & ~(AL_STRUCT - 1);
    getsym();
}

void
dodecl(enum e_sc defclass)
{
    TYP            *tp;

    for (;;) {
        if (fatal)
            return;
        switch (lastst) {
        case asmconst:
            addauto(asmstmt());
            break;
        case kw_typedef:
            if (defclass == sc_global)
                declare(&gsyms, sc_global, lc_static, bt_struct, defclass);
            else if (defclass == sc_auto)
                declare(&lsyms, sc_auto, lc_auto, bt_struct, defclass);
            else
                declare(&lsyms, sc_auto, 0, bt_struct, defclass);
            break;
        case kw_register:
            getsym();
            if (defclass != sc_auto && defclass != sc_member)
                error(ERR_ILLCLASS, NULL);
            goto do_decl;
        case kw_auto:
            getsym();
            if (defclass != sc_auto && defclass != sc_member)
                error(ERR_ILLCLASS, NULL);
            goto do_decl;
        case kw_const:
            getsym();
            if (defclass != sc_auto && defclass != sc_member)
                error(ERR_ILLCLASS, NULL);
            goto do_decl;
        case kw_volatile:
            getsym();
            if (defclass != sc_auto && defclass != sc_member)
                error(ERR_ILLCLASS, NULL);
            goto do_decl;
        case id:
            if ((tp = istypedef(&lsyms)) != NULL)
                goto do_decl;
            if ((tp = istypedef(&gsyms)) != NULL)
                goto do_decl;
            tp = tp;
            if (defclass == sc_auto)
                return;

            /* else fall through to declare    */
        case kw_char:
        case kw_int:
        case kw_short:
        case kw_unsigned:
        case kw_long:
        case kw_struct:
        case kw_union:
        case kw_enum:
        case kw_void:
        case kw_float:
        case kw_double:
    do_decl:    if (defclass == sc_global)
                lc_static +=
                    declare(&gsyms, sc_global, lc_static, bt_struct, defclass);
            else if (defclass == sc_auto)
                lc_auto +=
                    declare(&lsyms, sc_auto, lc_auto, bt_struct, defclass);
            else
                declare(&lsyms, sc_auto, 0, bt_struct, defclass);
            break;
        case kw_static:
            getsym();
            if (defclass == sc_member)
                error(ERR_ILLCLASS, NULL);
            if (defclass == sc_auto)
                lc_static +=
                    declare(&lsyms, sc_static, lc_static, bt_struct, defclass);
            else
                lc_static +=
                    declare(&gsyms, sc_static, lc_static, bt_struct, defclass);
            break;
        case kw_extern:
            getsym();
            if (defclass == sc_member)
                error(ERR_ILLCLASS, NULL);
            ++global_flag;
            declare(&gsyms, sc_external, 0, bt_struct, defclass);
            --global_flag;
            break;
        default:
            return;
        }
    }
}

void
compile(void)

/*
 * main compiler routine. this routine parses all of the declarations using
 * declare which will call funcbody as functions are encountered.
 */
{
    if (autohead != NULL) {
        genstmt(autohead);
        flush_peep();
        autohead = autotail = NULL;
    }

#ifdef  GENERATE_DBX
    dbx_init();
#endif

    while (lastst != eof) {
        if (fatal)
            return;
        dodecl(sc_global);
        if (lastst != eof)
            getsym();
    }

    if (autohead != NULL) {
        genstmt(autohead);
        flush_peep();
        autohead = autotail = NULL;
    }
    dumplits();
}

int
declproto(TABLE *table)  /* table is the argument table for the function */

/*
 * process declarations of the form:
 * 
 * ( <type> [<decl>] [,<type> [<decl>]] ... )
 * 
 */

{
    SYM            *sp, *sp2;
    int             nbytes;

    nbytes = 8;     /* return block */
    global_flag++;

    for (;;) {
        declid = NULL;

        decl(&lsyms);   /* Set the base type of the variable */
        decl1();

        /* Make *argv[] work */

        if (head->size == 0 || head->type == bt_pointer)
            head->val_flag = 0;

        sp = (SYM *) xalloc(sizeof(SYM));
        sp->name = declid;
        sp->storage_class = sc_auto;
        sp->storage_type = sc_proto;
        sp->next = NULL;

        sp->value.i = nbytes;
        sp->tp = head;

        if (sp->tp->size >= 4)
            nbytes += sp->tp->size;
        else {
            if (sp->tp->size == 1)  /* byte reference */
                sp->tp->size += 1;
            nbytes += 2;
        }

        sp2 = copysym(sp);

        switch (sp->tp->type) {
        case bt_uchar:
        case bt_ushort:
            sp->tp = maketype(bt_unsigned, 4);
            break;
        case bt_char:
        case bt_short:
        case bt_enum:
            sp->tp = maketype(bt_long, 4);
            break;
        case bt_float:
            sp->tp = maketype(bt_double, 8);
            break;
        }

        insert(sp, &lsyms); /* Insert into the local symbols */
        insert(sp2, table); /* Insert into the function list */

        if (lastst != comma)
            break;
        getsym();
    }
    if (lastst == ellipsis) {
        getsym();
        if (lastst != closepa)
            error(ERR_SYNTAX, NULL);
    }

    global_flag--;
    return nbytes;
}
