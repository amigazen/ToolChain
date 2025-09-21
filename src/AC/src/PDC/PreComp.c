
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
 * $Log:	PreComp.c,v $
 * Revision 3.33  90/04/05  22:44:59  lionel
 * None.
 * 
 * Revision 3.32  90/02/03  16:25:20  lionel
 * None
 * 
 *------------------------------------------------------------------
 */

/*
 * PreComp.c - Generate and read precompiled header files
 */

#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        "C.h"
#include        "Expr.h"
#include        "Gen.h"
#include        "Cglbdec.h"

#if 0
#define GENERATE_FMT
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
extern char    *litlate(), *itoa();
extern char    *xalloc();

extern char     prepbuffer[];

extern TABLE    tagtable;

extern struct libcall   *libpragma;

void            read_tbl();
void            dump_tbl();
void            fmt_tbl2();
void            fmt_type();

static FILE    *fp = NULL;
static int      indent_level = 0;

#define MAXCHAIN    100

struct chain {
    struct chain   *next;
    int             entries;
    void           *data[MAXCHAIN + 1];
};

static struct chain *chead = NULL, *ctail = NULL;

enum e_comp {
    TOPLEV_TBL, ENDOF_TBL,
    TOPLEV_SYM, ENDOF_SYM,
    TOPLEV_TYP, ENDOF_TYP,
    TOPLEV_RSYM, ENDOF_RSYM,
    TOPLEV_RTYP, ENDOF_RTYP,
    TOPLEV_LIBENTRY, ENDOF_LIBENTRY,
    TOPLEV_LIBCALL, ENDOF_LIBCALL
};

static void
putcomp1(val)
    long            val;
{
    char            data = val;

    fwrite(&data, 1, sizeof(char), fp);
}

static long
getcomp1()
{
    char            val;

    fread(&val, 1, sizeof(char), fp);
    return ((long) val);
}

static void
putcomp2(val)
    long            val;
{
    short           data = val;

    fwrite(&data, 1, sizeof(short), fp);
}

static short
getcomp2()
{
    short           val;

    fread(&val, 1, sizeof(short), fp);
    return (val);
}

static void
putcomp4(val)
    long            val;
{
    fwrite(&val, 1, sizeof(long), fp);
}

static long
getcomp4()
{
    long            val;

    fread(&val, 1, sizeof(long), fp);
    return (val);
}

static void
putstrlit(s)
    char           *s;
{
    long            count;

    if (s == NULL)
        putcomp2(0);
    else {
        count = strlen(s) + 1;
        putcomp2(count);
        fwrite(s, 1, count, fp);
    }
}

static char    *
getstrlit()
{
    long            count;


    count = getcomp2();
    if (count == 0)
        return (NULL);
    else {
        fread(laststr, 1, count, fp);
        return (litlate(laststr));
    }
}


void           *
get_recur(key)
    int             key;
{
    int             i, index;
    struct chain   *link;

    index = 0;
    for (link = chead; link != NULL; link = link->next) {
        for (i = 0; i < link->entries; ++i) {
            ++index;
            if (key == index)
                return (link->data[i]);
        }
    }
    return (NULL);
}

int
check_recur(ptr)
    void           *ptr;
{
    int             i, index;
    struct chain   *link;

    index = 0;
    for (link = chead; link != NULL; link = link->next) {
        for (i = 0; i < link->entries; ++i) {
            ++index;
            if (ptr == link->data[i])
                return (index);
        }
    }
    return (-1);
}

void
put_recur(ptr)
    void           *ptr;
{
    struct chain   *link;

    if (ctail == NULL) {
        chead = ctail = (struct chain *) xalloc(sizeof(struct chain));
        memset((char *)ctail, 0, sizeof(struct chain));
    }
    else if (ctail->entries >= MAXCHAIN) {
        link = (struct chain *) xalloc(sizeof(struct chain));
        memset((char *)link, 0, sizeof(struct chain));
        ctail->next = link;
        ctail = link;
    }
    ctail->data[ctail->entries] = ptr;
    ctail->entries += 1;
}

void
dump_type(tp)
    TYP            *tp;
{
    int             index;

    if (tp != NULL) {
        if ((index = check_recur(tp)) >= 0) {
            putcomp1(TOPLEV_RTYP);
            putcomp2(index);
            putcomp1(ENDOF_RTYP);
        }
        else {
            put_recur(tp);
            putcomp1(TOPLEV_TYP);
            putstrlit(tp->sname);
            putcomp1(tp->type);
            putcomp1(tp->val_flag);
            putcomp4(tp->size);
            dump_tbl(&(tp->lst));
            dump_type(tp->btp);
            putcomp1(ENDOF_TYP);
        }
    }
}

void
dump_union(sp)
    SYM            *sp;
{
    switch (sp->storage_class) {
    case sc_define:
        putstrlit(sp->value.s);
        break;
    default:
        putcomp4(sp->value.i);
        break;
    }
}

void
dump_symbol(sp)
    SYM            *sp;
{
    int             index;

    if ((index = check_recur(sp)) >= 0) {
        putcomp1(TOPLEV_RSYM);
        putcomp2(index);
        putcomp1(ENDOF_RSYM);
    }
    else {
        put_recur(sp);
        putcomp1(TOPLEV_SYM);
        putstrlit(sp->name);
        putcomp4(sp->key);
        putcomp1(sp->storage_class);
        putcomp1(sp->storage_type);
        dump_union(sp);
        dump_type(sp->tp);
        putcomp1(ENDOF_SYM);
    }
}

void
dump_libentry( lb )
    struct libcall  *lb;
{
    putcomp1(TOPLEV_LIBENTRY);

    putstrlit( lb->basename );
    putstrlit( lb->funcname );
    putstrlit( lb->args     );
    putstrlit( lb->offset   );

    putcomp1(ENDOF_LIBENTRY);
}

void
dump_libcall()
{
    struct libcall  *lb;

    putcomp1(TOPLEV_LIBCALL);

    for (lb = libpragma; lb != NULL; lb = lb->next) 
        dump_libentry( lb );
   
    putcomp1(ENDOF_LIBCALL);
}

void
dump_tbl(tbl)
    TABLE          *tbl;
{
    SYM            *sp;
    int             index;

    putcomp1(TOPLEV_TBL);

    for (sp = tbl->head; sp != NULL; sp = sp->next) {
        if ((index = check_recur(sp)) < 0)
            dump_symbol(sp);
        else {
            putcomp1(TOPLEV_RSYM);
            putcomp2(index);
            putcomp1(ENDOF_RSYM);
            break;
        }
    }
    putcomp1(ENDOF_TBL);
}

void
dump_precomp(name)
    char           *name;
{
    if ((fp = fopen(name, "w")) == NULL) {
        fprintf(stderr, "Error in opening precomp file '%s' !\n", name );
        exit(1);
    }

    dump_tbl(&defsyms);
    dump_tbl(&gsyms);
    dump_tbl(&tagtable);

    dump_tbl(&cmd_local);

    dump_libcall();

    fclose(fp);

    chead = ctail = NULL;
}

void           *
read_recur(comp)
    enum e_comp     comp;
{
    long            ctype;
    int             index;

    index = getcomp2();
    ctype = getcomp1();

    if ((enum e_comp) ctype != comp) {
        fprintf(stderr, "Reading a RECUR, got [%d]\n", ctype );
    }

    return (get_recur(index));
}

void
read_type(tp)
    TYP            *tp;
{
    long            ctype;

    put_recur(tp);

    tp->sname = getstrlit();
    tp->type = (enum e_bt) getcomp1();
    tp->val_flag = getcomp1();
    tp->size = getcomp4();

    read_tbl(&tp->lst);

    ctype = getcomp1();

    if ((enum e_comp) ctype == TOPLEV_RTYP) {
        tp->btp = (TYP *) read_recur(ENDOF_RTYP);
        ctype = getcomp1();
    }
    else if ((enum e_comp) ctype == TOPLEV_TYP) {
        ++global_flag;
        tp->btp = (TYP *) xalloc(sizeof(TYP));
        --global_flag;
        memset((char *)tp->btp, 0, sizeof(TYP));
        read_type(tp->btp);
        ctype = getcomp1();
    }
    if ((enum e_comp) ctype != ENDOF_TYP) {
        fprintf( stderr, "Reading a TYPE, got [%d]\n", ctype );
    }
}

void
read_union(sp)
    SYM            *sp;
{
    switch (sp->storage_class) {
    case sc_define:
        sp->value.s = getstrlit();
        break;
    default:
        sp->value.i = getcomp4();
        break;
    }
}

void
read_symbol(sp)
    SYM            *sp;
{
    long            ctype;

    put_recur(sp);

    sp->name = getstrlit();
    sp->key = getcomp4();
    sp->storage_class = (enum e_sc) getcomp1();
    sp->storage_type = (enum e_sc) getcomp1();
    read_union(sp);

    ctype = getcomp1();

    if ((enum e_comp) ctype == TOPLEV_RTYP) {
        sp->tp = (TYP *) read_recur(ENDOF_RTYP);
        ctype = getcomp1();
    }
    else if ((enum e_comp) ctype == TOPLEV_TYP) {
        ++global_flag;
        sp->tp = (TYP *) xalloc(sizeof(TYP));
        --global_flag;
        memset((char *)sp->tp, 0, sizeof(TYP));
        read_type(sp->tp);
        ctype = getcomp1();
    }
    if ((enum e_comp) ctype != ENDOF_SYM) {
        fprintf(stderr, "Reading a SYMBOL, got [%d]\n", ctype );
    }
}

void
read_libentry( lb )
    struct libcall  *lb;
{
    long    ctype;

    lb->basename = getstrlit();
    lb->funcname = getstrlit();
    lb->args     = getstrlit();
    lb->offset   = getstrlit();

    ctype = getcomp1();

    if ((enum e_comp) ctype != ENDOF_LIBENTRY) {
        fprintf(stderr, "Reading a LIBENTRY, got [%d]\n", ctype );
    }
}

void
read_libcall()
{
    long            ctype;

    struct libcall  *lb;

    ctype = getcomp1();

    if ((enum e_comp) ctype != TOPLEV_LIBCALL) {
        fprintf(stderr, "Reading a LIBCALL start, got [%d]\n", ctype );
        exit(1);
    }

    libpragma = NULL;

    for (;;) {
        switch (ctype = getcomp1()) {
        case ENDOF_LIBCALL:
            return;
        case TOPLEV_LIBENTRY:
            ++global_flag;
            lb = (struct libcall *) xalloc(sizeof(struct libcall));
            --global_flag;

            memset((char *)lb, 0, sizeof(struct libcall));
            read_libentry(lb);

            lb->next = libpragma;
            libpragma = lb;

            break;
        default:
            fprintf(stderr, "Reading a LIBCALL, got [%d]\n", ctype );
            return;
        }
    }
}

void
read_tbl(tbl)
    TABLE          *tbl;
{
    long            ctype;

    SYM            *sp;

    ctype = getcomp1();

    if ((enum e_comp) ctype != TOPLEV_TBL) {
        fprintf(stderr, "Reading a TABLE start, got [%d]\n", ctype );
        exit(1);
    }

    memset((char *)tbl, 0, sizeof(TABLE));

    for (;;) {
        switch (ctype = getcomp1()) {
        case ENDOF_TBL:
            return;
        case TOPLEV_RSYM:
            sp = (SYM *) read_recur(ENDOF_RSYM);
            if (tbl->head == NULL)
                tbl->head = tbl->tail = sp;
            else
                tbl->tail->next = sp;
            while (tbl->tail->next != NULL)
                tbl->tail = tbl->tail->next;
            break;
        case TOPLEV_SYM:
            ++global_flag;
            sp = (SYM *) xalloc(sizeof(SYM));
            --global_flag;
            memset((char *)sp, 0, sizeof(SYM));
            read_symbol(sp);
            if (tbl->head == NULL)
                tbl->head = tbl->tail = sp;
            else
                tbl->tail->next = sp;
            while (tbl->tail->next != NULL)
                tbl->tail = tbl->tail->next;
            break;
        default:
            fprintf(stderr, "Reading a TABLE, got [%d]\n", ctype );
            return;
        }
    }
}

void
read_precomp(name)
    char           *name;
{
    if ((fp = fopen(name, "r")) == NULL) {
        fprintf(stderr, "Error in opening precomp file '%s'!\n", name );
        exit(1);
    }

    read_tbl(&defsyms);
    read_tbl(&gsyms);
    read_tbl(&tagtable);

    read_tbl(&cmd_include);

    read_libcall();

    fclose(fp);

    chead = ctail = NULL;
}


#ifndef GENERATE_FMT

void fmt_precomp() { }

#else

static void
prepend(s1)
    char           *s1;
{
    char           *ptr;
    int             len, ofs = strlen(s1);

    len = strlen(prepbuffer);
    for (ptr = &prepbuffer[len]; len >= 0; --ptr, --len)
        ptr[ofs] = *ptr;
    ptr = prepbuffer;
    while (*s1)
        *ptr++ = *s1++;
}

static void
append(s1)
    char           *s1;
{
    strcat(prepbuffer, s1);
}

void
fmt_indent()
{
    int             i;
    char buffer[1024], *cp;

    cp = &buffer[0];
    i = (indent_level > 1023 ? 1023 : indent_level);
    while (i--)
        *cp++ = " ";
    *cp = '\0';
    fprintf(fp, "%s", buffer);
}

void
fmt_aux(tp)
    TYP            *tp;
{
    char            hold[80];

    strcpy(hold, prepbuffer);
    fmt_tbl2(tp, "");
    strcpy(prepbuffer, hold);
}

void
fmt_func(tp)
    TYP            *tp;
{
    SYM            *sp;
    char            hold[256];

    append("(");
    strcpy(hold, prepbuffer);
    sp = tp->lst.head;
    if (sp != NULL) {
        strcpy(prepbuffer, sp->name != NULL ? sp->name : "");
        fmt_type(sp->tp, bt_unknown);
        strcat(hold, prepbuffer);
        for (sp = sp->next; sp != NULL; sp = sp->next) {
            strcpy(prepbuffer, sp->name != NULL ? sp->name : "");
            fmt_type(sp->tp, bt_unknown);
            strcat(hold, ", ");
            strcat(hold, prepbuffer);
        }
    }
    strcpy(prepbuffer, hold);
    append(")");
}

void
fmt_type(tp, prev)
    TYP            *tp;
    enum e_bt       prev;
{
    int             len;

    switch (tp->type) {
    case bt_pointer:
        if (tp->val_flag) {
            if (prev == bt_pointer) {
                prepend("(");
                append(")");
            }
            len = strlen(prepbuffer);
            if (tp->size <= 0)
                append("[]");
            else {
                prepbuffer[len] = '[';
                strcpy( &prepbuffer[len+1], itoa( tp->size / tp->btp->size ));
                strcat( &prepbuffer[len], "]" );
            }
            fmt_type(tp->btp, bt_unknown);
        }
        else {
            prepend("*");
            fmt_type(tp->btp, bt_pointer);
        }
        break;
    case bt_char:
        prepend("char ");
        break;
    case bt_uchar:
        prepend("unsigned char ");
        break;
    case bt_short:
        prepend("short ");
        break;
    case bt_ushort:
        prepend("unsigned short ");
        break;
    case bt_unsigned:
        prepend("unsigned int ");
        break;
    case bt_long:
        prepend("long ");
        break;
    case bt_float:
        prepend("float ");
        break;
    case bt_double:
        prepend("double ");
        break;
    case bt_struct:
        if (tp->sname == NULL)
            fmt_aux(tp);
        else {
            prepend(" ");
            prepend(tp->sname);
            prepend("struct ");
        }
        break;
    case bt_union:
        if (tp->sname == NULL)
            fmt_aux(tp);
        else {
            prepend(" ");
            prepend(tp->sname);
            prepend("union ");
        }
        break;
    case bt_enum:
        if (tp->sname == NULL)
            fmt_aux(tp);
        else {
            prepend(" ");
            prepend(tp->sname);
            prepend("enum ");
        }
        break;
    case bt_func:
        if (prev == bt_pointer) {
            prepend("(");
            append(")");
        }
        fmt_func(tp);
        fmt_type(tp->btp, bt_unknown);
        break;
    case bt_ifunc:
        if (prev == bt_pointer) {
            prepend("(");
            append(")");
        }
        fmt_func(tp);
        fmt_type(tp->btp, bt_unknown);
        break;
    case bt_void:
        prepend("void ");
        break;
    case bt_unknown:
        prepend("ERROR *2 ");
        break;
    }
}

void
fmt_symbol(sp)
    SYM            *sp;
{
    if (sp != NULL && sp->tp != NULL) {
        strcpy(prepbuffer, (sp->name != NULL ? sp->name : ""));
        fmt_type(sp->tp, bt_unknown);
        fprintf(fp, "%s;\n", prepbuffer );
    }
}

void
fmt_tags(tbl)
    TABLE          *tbl;
{
    SYM            *sp, *sp1;
    int             count;

    for (sp = tbl->head; sp != NULL; sp = sp->next) {
        if (sp->tp != NULL) {
            switch (sp->storage_class) {
            case sc_const:  /* Ignore enum labels */
                break;
            case sc_type:
                switch (sp->tp->type) {
                case bt_enum:
                    fprintf(fp, "enum %s {\n", sp->name );
                    indent_level += 4;
                    sp1 = sp->tp->lst.head;
                    if (sp1 != NULL) {
                        fmt_indent();
                        fprintf( fp, "%s", sp1->name );
                        count = 1;
                        for (sp1 = sp1->next; sp1 != NULL; sp1 = sp1->next) {
                            ++count;
                            fprintf( fp, ", ");
                            if (count > 5) {
                                count = 1;
                                fprintf(fp, "\n");
                                fmt_indent();
                            }
                            fprintf( fp, "%s", sp1->name );
                        }
                    }
                    indent_level -= 4;
                    fprintf( fp, "\n");
                    fmt_indent();
                    fprintf( fp, "};\n\n");
                    break;
                case bt_union:
                    fprintf(fp, "union %s {\n", sp->name );
                    indent_level += 4;
                    sp1 = sp->tp->lst.head;
                    if (sp1 != NULL) {
                        fmt_indent();
                        fmt_symbol(sp1);
                        for (sp1 = sp1->next; sp1 != NULL; sp1 = sp1->next) {
                            fmt_indent();
                            fmt_symbol(sp1);
                        }
                    }
                    indent_level -= 4;
                    fmt_indent();
                    fprintf( fp, "};\n\n");
                    break;
                case bt_struct:
                    fprintf(fp, "struct %s {\n", sp->name );
                    indent_level += 4;
                    sp1 = sp->tp->lst.head;
                    if (sp1 != NULL) {
                        fmt_indent();
                        fmt_symbol(sp1);
                        for (sp1 = sp1->next; sp1 != NULL; sp1 = sp1->next) {
                            fmt_indent();
                            fmt_symbol(sp1);
                        }
                    }
                    indent_level -= 4;
                    fmt_indent();
                    fprintf( fp, "};\n\n");
                    break;
                }
                break;
            }
        }
    }
}

void
fmt_tbl2(tp, name)
    TYP            *tp;
    char           *name;
{
    SYM            *sp1;

    if (tp != NULL) {
        switch (tp->type) {
        case bt_enum:
            fprintf(fp, "enum %s {\n", name );
            indent_level += 4;
            sp1 = tp->lst.head;
            fpprintf( fp, sp1->name );
            for (sp1 = sp1->next; sp1 != NULL; sp1 = sp1->next) {
                fprintf( fp, ",%s", sp1->name );
            }
            indent_level -= 4;
            fmt_indent();
            fprintf( fp, "} ");
            break;
        case bt_union:
            fprintf(fp, "union %s {\n", name );
            indent_level += 4;
            sp1 = tp->lst.head;
            fmt_indent();
            fmt_symbol(sp1);
            for (sp1 = sp1->next; sp1 != NULL; sp1 = sp1->next) {
                fmt_indent();
                fmt_symbol(sp1);
            }
            indent_level -= 4;
            fmt_indent();
            fprintf( fp, "} ");
            break;
        case bt_struct:
            fprintf(fp, "struct %s {\n", name );
            indent_level += 4;
            sp1 = tp->lst.head;
            fmt_indent();
            fmt_symbol(sp1);
            for (sp1 = sp1->next; sp1 != NULL; sp1 = sp1->next) {
                fmt_indent();
                fmt_symbol(sp1);
            }
            indent_level -= 4;
            fmt_indent();
            fprintf( fp, "} " );
            break;
        case bt_typedef:
            fprintf(fp, "typedef " );
            strcpy(prepbuffer, name);
            fmt_type(tp->btp, bt_unknown);
            fprintf(fp, "%s", name );
            break;
        }
    }
}

void
fmt_tbl(tbl)
    TABLE          *tbl;
{
    SYM            *sp;

    for (sp = tbl->head; sp != NULL; sp = sp->next) {
        switch (sp->storage_class) {
        case sc_const:
            break;
        case sc_type:
            fmt_tbl2(sp->tp, sp->name);
            fprintf(fp, ";\n\n" );
            break;
        case sc_external:
            if (sp != NULL && sp->tp != NULL) {
                fprintf(fp, "extern " );
                fmt_symbol(sp);
            }
            break;
        case sc_static:
            if (sp != NULL && sp->tp != NULL) {
                fprintf( fp, "static " );
                fmt_symbol(sp);
            }
            break;
        default:
            fmt_symbol(sp);
            break;
        }
    }
}

void
fmt_libentry( lb )
    struct libcall  *lb;
{
    fprintf( fp, "#pragma libcall %s %s %s %s\n",
             lb->basename, lb->funcname, lb->offset, lb->args );
}

void
fmt_libcall()
{
    struct libcall  *lb;

    for (lb = libpragma; lb != NULL; lb = lb->next) 
        fmt_libentry( lb );
}


void
fmt_define(tbl)
    TABLE          *tbl;
{
    int             len;
    SYM            *sp, *sp1;

    for (sp = tbl->head; sp != NULL; sp = sp->next) {
        fprintf( fp, "#define %s", sp->name);
        if (sp->tp != NULL && (sp1 = sp->tp->lst.head) != NULL) {
            fprintf( fp, "(%s", sp1->name );
            for (sp1 = sp1->next; sp1 != NULL; sp1 = sp1->next) {
                fprintf( fp, ",%s", sp1->name );
            }
            fprintf( fp, ") " );
        }
        if (sp->value.s != NULL) {
            for (len = strlen(sp->value.s) - 1; len >= 0; --len) {
                if (sp->value.s[len] != '\n')
                    break;
                sp->value.s[len] = ' ';
            }
            fprintf( fp, " %s", sp->value.s );
        }
        fprintf( fp, "\n" );
    }
}

void
fmt_include(tbl)
    TABLE          *tbl;
{
    int             len;
    SYM            *sp, *sp1;

    for (sp = tbl->head; sp != NULL; sp = sp->next) {
        fprintf( "#include ", fp );
        if (sp->value.i) {
            fprintf( fp, "#include <%s>\n", sp-name);
        }
        else {
            fprintf( fp, "#include \"%s\"\n", sp-name);
        }
    }
}

void
fmt_precomp(name)
    char           *name;
{
    char *comment_string = "\n/* ------------------------------ */\n\n" ;

    if ((fp = fopen(name, "w")) == NULL) {
        fprintf( stderr, "Error in opening precomp file '%s' !\n", name );
        exit(1);
    }
    fprintf( fp, comment_string );
    fmt_define(&defsyms);
    fprintf( fp, comment_string );
    fmt_tags(&tagtable);
    fprintf( fp, comment_string );
    fmt_tbl(&gsyms);
    fprintf( fp, comment_string );
    fmt_libcall();
    fprintf( fp, comment_string );
    fmt_include(&cmd_local);

    fclose(fp);

    chead = ctail = NULL;
}

#endif
