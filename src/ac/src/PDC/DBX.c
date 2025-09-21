
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
 * $Log:	DBX.c,v $
 * Revision 3.33  90/04/04  02:30:41  lionel
 * *** empty log message ***
 * 
 * Revision 3.32  90/02/03  16:23:14  lionel
 * None
 * 
 *------------------------------------------------------------------
 */

/*
 * DBX.c
 * 
 * Output dbx-format symbol table data.
 * 
 * This consists of many symbol table entries, each of them a .stabs assembler
 * pseudo-op with four operands: a "name" which is really a description of
 * one symbol and its type, a "code", which is a symbol defined in stab.h
 * whose name starts with N_, an unused operand always 0, and a "value" which
 * is an address or an offset. The name is enclosed in doublequote
 * characters.
 * 
 * Each function, variable, typedef, and structure tag has a symbol table entry
 * to define it. The beginning and end of each level of name scoping within a
 * function are also marked by special symbol table entries.
 * 
 * The "name" consists of the symbol name, a colon, a kind-of-symbol letter, and
 * a data type number.  The data type number may be followed by "=" and a
 * type definition; normally this will happen the first time the type number
 * is mentioned.  The type definition may refer to other types by number, and
 * those type numbers may be followed by "=" and nested definitions.
 * 
 * This can make the "name" quite long. When a name is more than 80 characters,
 * we split the .stabs pseudo-op into two .stabs pseudo-ops, both sharing the
 * same "code" and "value". The first one is marked as continued with a
 * double-backslash at the end of its "name".
 * 
 * The kind-of-symbol letter distinguished function names from global variables
 * from file-scope variables from parameters from auto variables in memory
 * from typedef names from register variables. See `dbxout_symbol'.
 * 
 * The "code" is mostly redundant with the kind-of-symbol letter that goes in
 * the "name", but not entirely: for symbols located in static storage, the
 * "code" says which segment the address is in, which controls how it is
 * relocated.
 * 
 * The "value" for a symbol in static storage is the core address of the symbol
 * (actually, the assembler label for the symbol).  For a symbol located in a
 * stack slot it is the stack offset; for one in a register, the register
 * number. For a typedef symbol, it is zero.
 * 
 */

#ifndef GENERATE_DBX

#else

#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        "C.h"
#include        "Expr.h"
#include        "Gen.h"
#include        "Cglbdec.h"
#include        <stab.h>

extern char    *litlate();
extern char    *itoa();

extern TYP     *maketype();
extern struct snode *declarestmt();
extern struct snode *linestmt();
extern struct enode *makenode();
extern struct dnode *makednode();

struct dbx_table_entry {
    TYP            *tp;
    int             link;
    int             status;
};

struct _dbx_table {
    enum e_bt       btype;
    int             size;
    char           *name;
    int             index, relative;
    int             low, high;
};

struct _dbx_table dbx_table[10] = {
    {bt_long, 4, "int", 1, 1, -2147483648, 2147483647},
    {bt_char, 1, "char", 2, 2, 0, 127},
    {bt_short, 2, "short", 3, 1, -32768, 32767},
    {bt_uchar, 1, "unsigned_char", 4, 1, 0, 255},
    {bt_ushort, 2, "unsigned_short", 5, 1, 0, 65535},
    {bt_unsigned, 4, "unsigned_int", 6, 1, 0, -1},
    {bt_float, 4, "float", 7, 1, 4, 0},
    {bt_double, 8, "double", 8, 1, 8, 0},
    {bt_void, 4, "void", 9, 9, 0, 0},
    {bt_unknown, 4, "???", 10, 1, 0, 0}
};

#define DBX_MAX 2048

char            dbx_buffer[DBX_MAX];
int             dbx_num_buffer = 0;

#define DBX_ENTRIES     100

struct dbx_table_entry dbx_entries[DBX_ENTRIES];
int             dbx_num_entries = 0;

int
dbx_match(tp1, tp2)
    TYP            *tp1, *tp2;
{
    if (tp1 == tp2)
        return (TRUE);
    if (tp1 != NULL && tp2 != NULL) {
        if (tp1->type == tp2->type) {
            if (tp1->sname == tp2->sname) {
                if (tp1->val_flag == tp2->val_flag) {
                    if (tp1->size == tp2->size) {
                        return (dbx_match(tp1->btp, tp2->btp));
                    }
                }
            }
        }
    }
    return (FALSE);
}

int
dbx_lookup(tp)
    TYP            *tp;
{
    int             i;

    for (i = 1; i <= dbx_num_entries; i++)
        if (dbx_match(tp, dbx_entries[i].tp))
            return (i);
    return (-1);
}

int
dbx_install(tp)
    TYP            *tp;
{
    int             num, siz;
    struct dbx_table_entry *ptr;

    if (tp != NULL) {
        num = dbx_lookup(tp);
        if (num == -1) {
            num = dbx_install(tp->btp);
            dbx_num_entries++;
            if (dbx_num_entries >= DBX_ENTRIES) {
                fprintf( stderr, "DIAG -- dbx_install, No room for types\n" );
                exit(1);
            }
            if (tp->type == bt_struct || tp->type == bt_union)
                num = 0;
            dbx_entries[dbx_num_entries].tp = tp;
            dbx_entries[dbx_num_entries].link = num;
            dbx_entries[dbx_num_entries].status = 0;
            num = dbx_num_entries;
        }
        return (num);
    }
    return (-1);
}

struct dnode   *
dbx_check(dp, buffer)
    struct dnode   *dp;
    char           *buffer;
{
    struct dnode   *dp2;

    dp2 = makednode(dp->tag);   /* Make a new node */

    *dp2 = *dp;     /* Copy the old node onto the new */

    dp->next = dp2;     /* Make the old node ref the new */

    dp->sp = litlate(buffer);   /* Save the old string */

    dp2->sp = NULL;     /* No space for new string */

    return (dp2);       /* Return the new dbug struct */
}

char           *
dbx_addreg(s)
    char           *s;
{
    char           *ptr, *loc;

    ptr = dbx_buffer;
    for (loc = s; *loc; loc++, ptr++) {
        *ptr = *loc;
        if (':' == *loc)
            *(++ptr) = 'r';
    }
    *ptr++ = '\0';
    return (litlate(dbx_buffer));
}

void
dbx_type(num, buffer)
    int             num;
    char           *buffer;
{
    int             loc, entries;
    struct dbx_table_entry *ptr;

    ptr = &dbx_entries[num];

    while (*buffer)
        ++buffer;

    loc = dbx_entries[num].link;

    strcat(buffer, "(0,");
    strcat(buffer, itoa(num));
    strcat(buffer, ")");

    if (ptr->status == 0) {
        ptr->status = 1;
        if (loc > 0) {
            strcat(buffer, "=");
            if (ptr->tp->type == bt_pointer) {
                if (ptr->tp->val_flag == 0)
                    strcat(buffer, "*");
                else {
                    while (*buffer)
                        ++buffer;
                    entries = (ptr->tp->size / ptr->tp->btp->size) - 1;
                    strcat(buffer, "ar(0,1);0;");
                    strcat(buffer, itoa(entries));
                    strcat(buffer, ";");
                }
            }
            dbx_type(loc, buffer);
        }
    }
}

int
dbx_name(sp, dp)
    SYM            *sp;
    struct dnode   *dp;
{
    char           *buffer, *cptr;
    int             clen, nlen, start, stop, num = 0;
    SYM            *sp1;

    buffer = dbx_buffer;
    *buffer = '\0';
    dbx_num_buffer = 0;

    dp->ref = NULL;
    dp->tag = N_LSYM;
    dp->label = dp->nest = 0;

    switch (sp->storage_class) {
    case sc_static:
        num = dbx_install(sp->tp);
        strcat(buffer, sp->name);
        strcat(buffer, ":S");
        dbx_type(num, buffer);
        dp->tag = N_STSYM;
        dp->ref = makenode(en_labcon, sp->value.i, NULL);
        break;
    case sc_auto:
        num = dbx_install(sp->tp);
        if (sp->storage_type == sc_parameter) {
            strcat(buffer, sp->name);
            strcat(buffer, ":p");
            dbx_type(num, buffer);
            dp->ref = makenode(en_autocon, sp->value.i, NULL);
            dp->tag = N_PSYM;
        }
        else {
            strcat(buffer, sp->name);
            strcat(buffer, ":");
            dbx_type(num, buffer);
            dp->ref = makenode(en_autocon, sp->value.i, NULL);
            dp->tag = N_LSYM;
        }
        break;
    case sc_global:
        num = dbx_install(sp->tp);
        if (sp->tp->type == bt_ifunc) {
            num = dbx_install(sp->tp->btp);
            strcat(buffer, sp->name);
            strcat(buffer, ":F(0,");
            strcat(buffer, itoa(num));
            strcat(buffer, ")");
            dp->tag = N_FUN;
        }
        else {
            strcat(buffer, sp->name);
            strcat(buffer, ":G");
            dbx_type(num, buffer);
            dp->tag = N_STSYM;
        }
        dp->ref = makenode(en_nacon, sp->name, NULL);
        break;
    case sc_type:
        num = dbx_install(sp->tp);
        switch (sp->tp->type) {
        case bt_typedef:
            strcat(buffer, sp->name);
            strcat(buffer, ":t(0,");
            strcat(buffer, itoa(num));
            strcat(buffer, ")");
            dp->ref = makenode(en_icon, sp->value.i, NULL);
            break;
        case bt_union:
            dp->ref = makenode(en_icon, sp->value.i, NULL);

            strcat(buffer, sp->name);
            strcat(buffer, ":T(0,");
            strcat(buffer, itoa(num));
            strcat(buffer, ")=u");
            strcat(buffer, itoa(sp->tp->size));

            clen = 0;
            nlen = strlen(buffer);

            cptr = buffer;
            sp1 = sp->tp->lst.head;

            while (sp1 != NULL) {
                if (nlen > DBX_MAX / 2) {
                    dp = dbx_check(dp, buffer);
                    cptr = buffer = dbx_buffer;
                    *buffer = '\0';
                    nlen = clen = 0;
                }

                while (*cptr)
                    cptr++;
                strcat(cptr, sp1->name);
                strcat(cptr, ":");
                nlen += strlen(cptr);

                while (*cptr)
                    cptr++;
                start = sp1->value.i * 8;
                stop = sp1->tp->size * 8;
                num = dbx_install(sp1->tp);
                dbx_type(num, cptr);
                nlen += strlen(cptr);

                while (*cptr)
                    cptr++;
                strcat(cptr, ",");
                strcat(cptr, itoa(start));
                strcat(cptr, ",");
                strcat(cptr, itoa(stop));
                strcat(cptr, ";");
                nlen += strlen(cptr);

                sp1 = sp1->next;

                if (nlen - clen > 60 && (sp1 != NULL)) {
                    strcat(cptr, "\\\\");
                    nlen += 2;
                    clen = nlen;
                }
            }
            strcat(cptr, ";");
            break;
        case bt_struct:
            dp->ref = makenode(en_icon, sp->value.i, NULL);

            strcat(buffer, sp->name);
            strcat(buffer, ":T(0,");
            strcat(buffer, itoa(num));
            strcat(buffer, ")=s");
            strcat(buffer, itoa(sp->tp->size));

            clen = 0;
            nlen = strlen(buffer);

            cptr = buffer;
            sp1 = sp->tp->lst.head;

            while (sp1 != NULL) {
                if (nlen > DBX_MAX / 2) {
                    dp = dbx_check(dp, buffer);
                    cptr = buffer = dbx_buffer;
                    *buffer = '\0';
                    nlen = clen = 0;
                }

                while (*cptr)
                    cptr++;
                strcat(cptr, sp1->name);
                strcat(cptr, ":");
                nlen += strlen(cptr);

                while (*cptr)
                    cptr++;
                start = sp1->value.i * 8;
                stop = sp1->tp->size * 8;
                num = dbx_install(sp1->tp);
                dbx_type(num, cptr);
                nlen += strlen(cptr);

                while (*cptr)
                    cptr++;
                strcat(cptr, ",");
                strcat(cptr, itoa(start));
                strcat(cptr, ",");
                strcat(cptr, itoa(stop));
                strcat(cptr, ";");
                nlen += strlen(cptr);

                sp1 = sp1->next;

                if (nlen - clen > 60 && (sp1 != NULL)) {
                    strcat(cptr, "\\\\");
                    nlen += 2;
                    clen = nlen;
                }
            }
            strcat(cptr, ";");
            break;
        case bt_enum:
            dp->ref = makenode(en_icon, sp->value.i, NULL);

            strcat(buffer, sp->name);
            strcat(buffer, ":T(0,");
            strcat(buffer, itoa(num));
            strcat(buffer, ")=e");

            clen = 0;
            nlen = strlen(buffer);
            cptr = buffer;
            sp1 = sp->tp->lst.head;

            while (sp1 != NULL) {
                if (nlen > DBX_MAX / 2) {
                    dp = dbx_check(dp, buffer);
                    cptr = buffer = dbx_buffer;
                    *buffer = '\0';
                    nlen = clen = 0;
                }
                while (*cptr)
                    cptr++;
                strcat(cptr, sp1->name);
                strcat(cptr, ":");
                strcat(cptr, itoa(sp1->value.i));
                strcat(cptr, ",");
                nlen += strlen(cptr);

                sp1 = sp1->next;

                if (nlen - clen > 60 && (sp1 != NULL)) {
                    strcat(cptr, "\\\\");
                    nlen += 2;
                    clen = nlen;
                }
            }
            strcat(cptr, ";");
            break;
        }
        break;
    case sc_const:
        num = dbx_install(sp->tp);
        strcat(buffer, sp->name);
        strcat(buffer, ":const(0,");
        strcat(buffer, itoa(num));
        strcat(buffer, ")");
        break;
    case sc_label:
    case sc_ulabel:
        dp->ref = makenode(en_nacon, sp->name, NULL);
        num = dbx_install(sp->tp);
        strcat(buffer, sp->name);
        strcat(buffer, ":label(0,");
        strcat(buffer, itoa(num));
        strcat(buffer, ")");
        break;
    case sc_external:
    case sc_member:
    default:
        return (0);
    }
    dp->sp = litlate(buffer);
    return (1);
}

void
dbx_ident(sp)
    SYM            *sp;
{
    struct snode   *snp;

    if (Options.Debug) {
        snp = declarestmt(sp);
        addauto(snp);
    }
}

void
dbx_line()
{
    struct snode   *snp;

    if (Options.Debug) {
        snp = linestmt();
        addauto(snp);
    }
}

void
dbx_init()
{
    int             i;
    struct _dbx_table *dxp;
    struct dbx_table_entry *ptr;

    if (!Options.Debug)
        return;

    dbx_num_entries = 0;

    for (i = 0; i < 10; i++) {
        dxp = &dbx_table[i];
        fprintf( output, 
                 "\tSTABS\t\"%s:t(0,%d)=r(0%d);%d;%d;\",%d,0,0,0\n",
                 dxp->name, dxp->index, dxp->relative, dxp->low, dxp->high,
                 N_LSYM );
        ptr = &dbx_entries[dxp->index];

        ptr->tp = maketype(dxp->btype, dxp->size);
        ptr->link = 0;
        ptr->status = 1;
    }
    dbx_num_entries = 10;
}

#endif
