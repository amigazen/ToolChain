
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
 * $Log:	List.c,v $
 * Revision 3.33  90/04/05  22:36:00  lionel
 * Prepending of library function's '.' moved from call_library().
 * 
 * Revision 3.32  90/02/03  16:24:51  lionel
 * None
 * 
 *------------------------------------------------------------------
 */

/*
 * List.c
 * 
 * Generates a verbose listing of PDC's results from parsing the source.
 */

#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    "C.h"
#include    "Expr.h"
#include    "Gen.h"
#include    "Cglbdec.h"

#ifdef PROTO
void put_sc(enum e_sc);
void put_ty(TYP);
void list_var(SYM, int);
void list_table(TABLE, int);
#else
void put_sc();
void put_ty();
void list_var();
void list_table();
#endif

extern char    *itoa();

void
put_sc(scl)
    enum e_sc       scl;
{
    char *storage_class = NULL;

    switch (scl) {
    case sc_static:
        storage_class = "Static      ";
        break;
    case sc_auto:
        storage_class = "Auto        ";
        break;
    case sc_global:
        storage_class = "Global      ";
        break;
    case sc_external:
        storage_class = "External    ";
        break;
    case sc_type:
        storage_class = "Type        ";
        break;
    case sc_const:
        storage_class = "Constant    ";
        break;
    case sc_member:
        storage_class = "Member      ";
        break;
    case sc_proto:
        storage_class = "Prototype   ";
        break;
    case sc_parameter:
        storage_class = "Parameter   ";
        break;
    case sc_label:
        storage_class = "Label";
        break;
    case sc_ulabel:
        storage_class = "Undefined label";
        break;
    }

    if (storage_class) {
        fprintf( list, storage_class );
    }
}

void
put_ty(tp)
    TYP            *tp;
{
    char *type = NULL;

    if (tp == NULL)
        return;

    switch (tp->type) {
    case bt_char:
        type = "Char";
        break;
    case bt_uchar:
        type = "unsigned Char";
        break;
    case bt_short:
        type = "Short";
        break;
    case bt_ushort:
        type = "unsigned Short";
        break;
    case bt_enum:
        type = "enum ";
        goto ucont;
    case bt_long:
        type = "Long";
        break;
    case bt_longlong:
        type = "LongLong";
        break;
    case bt_ulonglong:
        type = "ULongLong";
        break;
    case bt_unsigned:
        type = "unsigned long";
        break;
    case bt_float:
        type = "Float";
        break;
    case bt_double:
        type = "Double";
        break;
    case bt_pointer:
        fprintf( list, ( tp->val_flag ? "Array of " : "Pointer to " ));
        put_ty(tp->btp);
        break;
    case bt_union:
        type = "union ";
        goto ucont;
    case bt_struct:
        type = "struct ";
ucont:      if (tp->sname == NULL)
            type = "<no name> ";
        else {
            fprintf( list, "%s ", tp->sname );
            type = NULL;
        }
        break;
    case bt_typedef:
        type = "typedef ";
        goto ucont;
    case bt_ifunc:
    case bt_func:
        fprintf( list, "Function returning " );
        type = NULL;
        put_ty(tp->btp);
        break;
    }

   if (type) {
        fprintf( list, type );
   }
}

void
list_var(sp, i)
    SYM            *sp;
    int             i;
{
    int             j;
    char           *disp;

    if (Options.List) {
        for (j = i; j; --j)
            fprintf( list, "    " );

        disp = sp->name;
        if (disp == NULL)
            disp = "(empty)";

        fprintf( list, "%10s = %-8d ", disp, strlen(disp) );
        put_sc(sp->storage_class);
        put_ty(sp->tp);
        fprintf( list, "\n" );
    }

    if (sp->storage_class == sc_external) {
        if (strncmp("__BUILTIN_", sp->name, 10) != 0) {
            if (strncmp("__LIBCALL_", sp->name, 10) != 0) {
                if (sp->value.i != 0) { /* Don't do xref if not used */
                    if (sp->storage_type == sc_library)
                        fprintf( output, "\tXREF\t%s\n", sp->name );
                    else
                        fprintf( output, "\tXREF\t_%s\n", sp->name );
                }
            }
        }
    }

    else if (sp->storage_class == sc_global) {
        if (sp->tp == NULL || sp->tp->type != bt_typedef) {
            if (strncmp("__BUILTIN_", sp->name, 10) != 0) {
                if (strncmp("__LIBCALL_", sp->name, 10) != 0) {
                    fprintf( output, "\tXDEF\t_%s\n", sp->name );
                }
            }
        }
    }

    if (sp->tp == NULL)
        return;

    if ((sp->tp->type == bt_struct || sp->tp->type == bt_union) &&
        sp->storage_class == sc_type)
        list_table(&(sp->tp->lst), i + 1);
}

void
list_table(t, i)
    TABLE          *t;
    int             i;
{
    SYM            *sp;

    sp = t->head;

    /* If there are no symbols in the table, then note the fact in the listing
     * file.  If there are symbols, then send each of them to list_var() along
     * with the indent level variable, i, that was passed to us.
     */

    if (sp == NULL) {
        if (Options.List) {
            fprintf( list, "(none)\n" );
        }
    }
    else do {             /* sp != NULL on entry to loop */
        list_var(sp, i);
        sp = sp->next;
    } while (sp != NULL);
}
