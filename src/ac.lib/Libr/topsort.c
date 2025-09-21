
#include "dolib.h"

char           *data;
int             left;
int             column;
char            buf[256];
LL            **filetab;
SM             *symhead, *symtail;

extern LL      *objhead, *objtail;
extern int      v_flag;
extern char    *malloc();

nl(n)
{
    if (column + n > 70) {
        column = 0;
        printf("\n");
    }
    column += n;
}

char           *
litlate(s)
    char           *s;
{
    char           *ptr;

    ptr = (char *) malloc(strlen(s) + 1);
    strcpy(ptr, s);
    return (ptr);
}

SM             *
search_sym(buf)
    char           *buf;
{
    SM             *sp;

    for (sp = symhead; sp != NULL; sp = sp->next) {
        if (strcmp(sp->name, buf) == 0)
            return (sp);
    }
    return (NULL);
}

SM             *
insert_sym(buf, lp)
    char           *buf;
    LL             *lp;
{
    SM             *sp;

    if (symhead == NULL) {
        sp = (SM *) malloc(sizeof(SM));
        sp->name = litlate(buf);
        sp->parent = lp;
        sp->next = NULL;
        symhead = symtail = sp;
    }
    else {
        sp = search_sym(buf);
        if (sp != NULL) {
            if (sp->parent == NULL)
                sp->parent = lp;
            else {
                if (lp != NULL && sp->parent != lp) {
                    nl(65);
                    printf("WARNING: Multiple define of %s\n", buf);
                    column = 0;
                }
            }
        }
        else {
            sp = (SM *) malloc(sizeof(SM));
            sp->name = litlate(buf);
            sp->parent = lp;
            sp->next = NULL;
            symtail->next = sp;
            while (symtail->next != NULL)
                symtail = symtail->next;
        }
    }
    return (sp);
}

long
getl()
{
    long           *ptr;

    ptr = (long *) data;
    left -= sizeof(long);
    data += sizeof(long);
    return (*ptr);
}

int
load_name()
{
    int             len, code;
    long           *base;

    base = (long *) buf;
    len = getl();
    code = (len >> 24) & 0xFF;
    len = (len & 0xFFFFFF);
    while (len-- > 0)
        *base++ = getl();
    *base = 0;
    return (code);
}

do_hunk(lp)
    LL             *lp;
{
    long            atomf;
    int             i, j, len, hunk;
    SP             *sp;

    hunk = getl();

    if (atomf = hunk & 0xC0000000) {
        if (v_flag) {
            nl(10);
            if (atomf == 0x80000000)
                printf("%10s", "MEMF_FAST");
            else if (atomf == 0x40000000)
                printf("%10s", "MEMF_CHIP");
        }
        if (atomf == 0xC0000000) {
            atomf = getl();
            if (v_flag) {
                nl(20);
                printf("Extra: %08x", atomf);
            }
        }
    }

    switch (hunk) {
    case EOF:       /* getl() return eof on endof file */
        break;
    case HunkNone:
        break;
    case HunkEnd:
        if (v_flag) {
            nl(20);
            printf("%10s", "HUNK_END");
        }
        break;
    case HunkDbg:
        if (v_flag) {
            nl(20);
            printf("%10s", "HUNK_DEBUG");
        }
        len = getl();
        for (i = 0; i < len; i++)
            getl();
        break;
    case HunkName:
        load_name();
        if (v_flag) {
            nl(22);
            printf("%10s '%10s'", "HUNK_NAME", buf);
        }
        break;
    case HunkUnit:
    case HunkCode:
    case HunkData:
        if (v_flag) {
            nl(20);
            switch (hunk) {
              case HunkUnit:
                printf("HUNK_UNIT   ");
                break;
              case HunkCode:
                printf("HUNK_CODE   ");
                break;
              case HunkData:
                printf("HUNK_DATA   ");
                break;
            }
        }
        len = getl();
        for (i = 0; i < len; i++)
            j = getl();
        break;
    case HunkBSS:
        if (v_flag) {
            nl(22);
            printf("HUNK_BSS (%08x)", getl());
        }
        else
            getl();
        break;
    case HunkR32:
    case HunkR16:
    case HunkR8:
        if (v_flag) {
            nl(20);
            switch (hunk) {
              case HunkR32:
                printf("%15s", "HUNK_RELOC32");
                break;
              case HunkR16:
                printf("%15s", "HUNK_RELOC16");
                break;
              case HunkR8:
                printf("%15s", "HUNK_RELOC8");
                break;
            }
        }
        for (len = getl(); len > 0; len = getl()) {
            j = getl();
            for (i = 0; i < len; i++)
                j = getl();
        }
        break;
    case HunkExt:
        if (v_flag) {
            nl(20);
            printf("%10s", "HUNK_EXT");
        }
        for (len = load_name(); len != 0; len = load_name()) {
            sp = (SP *) malloc(sizeof(SP));
            switch (len) {
            case 0x01:
                if (v_flag) {
                    nl(22);
                    printf(" XDEF %-16s", buf);
                }
                sp->entry = insert_sym(buf, lp);
                sp->next = lp->xdef;
                lp->xdef = sp;
                j = getl();
                break;
            case 0x02:
                if (v_flag) {
                    nl(22);
                    printf(" XABS %-16s", buf);
                }
                sp->entry = insert_sym(buf, lp);
                sp->next = lp->xdef;
                lp->xdef = sp;
                j = getl();
                break;
            case 0x81:  /* EXT_REF32     */
            case 0x83:  /* EXT_REF16     */
            case 0x84:  /* EXT_REF8      */
                if (v_flag) {
                    nl(22);
                    printf(" XREF %-16s", buf);
                }
                sp->entry = insert_sym(buf, NULL);
                sp->next = lp->xref;
                lp->xref = sp;
                len = getl();
                for (i = 0; i < len; i++)
                    j = getl();
                break;
            default:
                nl(65);
                printf("WARNING: UNKNOWN reference '%s'\n", buf);
                column = 0;
                break;
            }
        }
        break;
    case HunkSym:
        load_name();
        if (v_flag) {
            nl(22);
            printf("%10s '%10s'", "HUNK_SYMBOL", buf);
        }
        len = getl();
        break;
    default:
        nl(65);
        printf("WARNING: UNDEFINED hunk (%08x) in %s\n", hunk,
                lp->dir_entry.object_filename);
        column = 0;
        break;
    }
}

int
compare(x, y)
    LL             *x, *y;
{
    SP             *xp, *yp;

    for (yp = y->xref; yp != NULL; yp = yp->next) {
        if (yp->entry->parent == x) {
            for (xp = x->xref; xp != NULL; xp = xp->next) {
                if (xp->entry->parent == y) {
                    nl(65);
                    printf("WARNING: cycle on files %s, %s\n",
                           x->dir_entry.object_filename,
                           y->dir_entry.object_filename);
                    column = 0;
                    return (FALSE);
                }
            }
            return (TRUE);
        }
    }
    return (FALSE);
}

/*
 * Perform a topological sort of the XDEF and XREF statements
 */

topsort()
{
    LL             *lp, *temp;
    SP             *sp, *sp2;
    SM             *sm, *sm2;
    int             i, j, count;

    column = 0;
    filetab = NULL;
    symhead = symtail = NULL;

    /*
     * Count the number of modules and allocate a temp pointer array
     */
    count = 0;
    for (lp = objhead; lp != NULL; lp = lp->next)
        ++count;
    filetab = (LL **) malloc((count + 1) * sizeof(LL *));

    /*
     * Load the modules into the pointer array and store the reference
     * symbols into the symbol table
     */
    count = 0;
    for (lp = objhead; lp != NULL; lp = lp->next) {
        filetab[count++] = lp;
        lp->xdef = lp->xref = NULL;
        data = lp->object_module;
        left = lp->dir_entry.module_size;
        if (v_flag) {
            nl(65);
            printf("%s:\n", lp->dir_entry.object_filename);
            column = 0;
        }
        while (left > 0)
            do_hunk(lp);
    }

    /*
     * Sort the modules by exchanging them if they are out of order. Two
     * modules (X,Y) are out of order in a XREF of Y is an XDEF of X.
     */
    for (i = 0; i < count - 1; i++) {
        for (j = i + 1; j < count; j++) {
            if (compare(filetab[i], filetab[j])) {
                temp = filetab[i];
                filetab[i] = filetab[j];
                filetab[j] = temp;
            }
        }
    }

    /*
     * Relink the modules in the sorted order
     */
    objhead = objtail = filetab[0];
    objtail->next = NULL;
    for (i = 1; i < count; i++) {
        lp = filetab[i];
        lp->next = NULL;
        objtail->next = lp;
        while (objtail->next != NULL)
            objtail = objtail->next;
    }

    /*
     * Free the module symbol reference storage
     */
    for (lp = objhead; lp != NULL; lp = lp->next) {
        sp = lp->xdef;
        while (sp != NULL) {
            sp2 = sp;
            sp = sp->next;
            free(sp2);
        }
        sp = lp->xref;
        while (sp != NULL) {
            sp2 = sp;
            sp = sp->next;
            free(sp2);
        }
    }

    /*
     * Free the symbol table
     */
    sm = symhead;
    while (sm != NULL) {
        sm2 = sm;
        sm = sm->next;
        free(sm2->name);
        free(sm2);
    }
    free(filetab);

    nl(65);         /* Reset the column counter  */
    column = 0;

    return (0);
}
