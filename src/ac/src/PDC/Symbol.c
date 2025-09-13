
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
 * $Log:	Symbol.c,v $
 * Revision 3.33  90/04/05  22:47:07  lionel
 * None.
 * 
 * Revision 3.32  90/02/03  16:25:53  lionel
 * None
 * 
 *------------------------------------------------------------------
 */

/*
 * Symbol.c
 * 
 * Routines for managing the symbol table.
 */

#include    <stdio.h>
#include    "C.h"
#include    "Expr.h"
#include    "Gen.h"
#include    "Cglbdec.h"

extern TABLE    gsyms;
extern TABLE    lsyms;

#ifdef SLOW_BUT_SURE

int
hashkey(str)            /* Compute the 16 bit CRC of the name */
    char           *str;
{
    int             key = 0;
    int             val;
    char           *ptr;

    if (str == NULL)
        return (0);

    for (ptr = str; *ptr; ++ptr) {
        val = ((key >> 8) ^ (*ptr)) & 0xFF;
        val = val ^ (val >> 4);
        key = 0xFFFF & ((key << 8) ^ val ^ (val << 5) ^ (val << 12));
    }
    return (key);
}

#else

int
hashkey(str)            /* Compute a sliding XOR of the name */
    char           *str;
{
    register int    key = 0;
    register char  *ptr;

    if (str == NULL)
        return (0);

    for (ptr = str; *ptr; ++ptr)
        key = (key << 1) ^ (*ptr);

    return (key);
}

#endif

SYM            *
search(str, ptr)
    register char  *str;
    register SYM   *ptr;
{
    register int    newkey;
    register SYM   *answer = NULL;

    newkey = hashkey(str);

    while (ptr != NULL) {
        if (ptr->key == newkey)
            if (ptr->name != NULL && strcmp(str, ptr->name) == 0)
                answer = ptr;
        ptr = ptr->next;
    }
    return answer;
}

SYM            *
gsearch(str)
    char           *str;
{
    SYM            *ptr;


    if ((ptr = search(str, &lsyms.head)) == NULL)
        ptr = search(str, &gsyms.head);
    return (ptr);
}

/* Insert the SYM into the TABLE, Add to end of the table    */

void
insert(sp, table)
    SYM            *sp;
    TABLE          *table;
{
    sp->key = hashkey(sp->name);

    if (table != NULL && sp != NULL) {
        sp->next = NULL;
        if (table->head == NULL)
            table->head = sp;
        else
            table->tail->next = sp;
        table->tail = sp;
    }
}

/* Remove the string sp from table */

void
remove(s, table)
    char           *s;
    TABLE          *table;
{
    register int    newkey;
    register SYM   *ptr, *last;

    if (table != NULL && s != NULL) {

        newkey = hashkey(s);

        ptr = table->head;

        if (ptr != NULL && ptr->key == newkey) {
            if (ptr->name != NULL && strcmp(s, ptr->name) == 0) {
                table->head = ptr->next;
                if (table->head == NULL)
                    table->tail = NULL;
                return;
            }
        }

        last = ptr;
        ptr = ptr->next;

        while (ptr != NULL) {
            if (ptr->key == newkey) {
                if (ptr->name != NULL && strcmp(s, ptr->name) == 0) {
                    last->next = ptr->next;
                    if (table->tail == ptr)
                        table->tail = last;
                    return;
                }
            }
            last = ptr;
            ptr = ptr->next;
        }
    }
}
