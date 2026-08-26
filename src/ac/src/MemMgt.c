
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
 * $Log:	MemMgt.c,v $
 * Revision 3.33  90/04/05  22:40:18  lionel
 * Hid a null-loop in xalloc to sanity check entire global list.
 * 
 * Revision 3.32  90/02/03  16:24:55  lionel
 * None
 * 
 *------------------------------------------------------------------
 */

/*
 * Memmgt.c - Handles efficient and clean allocation of blocks of memory -
 * Cleanly deallocates local and global symbol entries
 */

#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        "C.h"
#include        "Expr.h"
#include        "Gen.h"
#include        "Cglbdec.h"
#include        "host_posix.h"

#ifdef AC_COMPILER_HOST
#if defined(AC_HOST_POSIX)
#define free    xfree    /* We do not presently mess with block deallocation */
#define calloc  xcalloc
#endif
#endif

extern char    *itoa();

struct blk {
    struct blk     *next;
    char            m[1];   /* memory area */
};

/*
 * Byte offset of blk.m.  On Amiga (32-bit) next is 4 bytes.  On LP64 hosts
 * next is 8 bytes — using 4 overwrote blk->next and crashed in release_local.
 * (Amiga ac-self must keep the literal 4; do not use sizeof here for target.)
 */
#if defined(AC_HOST_POSIX)
#define BLK_M_OFF ((int) sizeof(struct blk *))
#else
#define BLK_M_OFF 4
#endif

long            glbsize = 0,    /* size left in current global block    */
                locsize = 0,    /* size left in current local block     */
                glbindx = 0,    /* global index         */
                locindx = 0;    /* local index          */

long            glbwaste = 0;   /* Unused memory at the end of the block */
long            locwaste = 0;   /* Unused memory at the end of the block */

long            glballoc = 0;   /* Number of times calloc is called     */
long            localloc = 0;   /* Number of times calloc is called     */

struct blk     *locblk = NULL,  /* pointer to local block       */
               *glbblk = NULL;  /* pointer to global block      */

char           *
xalloc(siz)
    int             siz;
{
    struct blk     *bp;
    char           *rv, *mem;

    if (siz & 1)        /* if odd size */
        siz += 1;   /* make it even */
#if defined(AC_HOST_POSIX)
    /* LP64: SYM/TYP contain pointers; keep allocations 8-byte aligned. */
    siz = (siz + 7) & ~7;
#endif

    if (siz > 2048) {
        fprintf(AC_DIAG_STREAM, "DIAG -- xalloc, size greater than 2048\n" );
        exit(1);
    }

    if (global_flag) {
        if (glbsize > siz) {
            rv = (char *) glbblk + BLK_M_OFF + glbindx;
            glbsize -= siz;
            glbindx += siz;
            mem = (char *) rv;
#if 0
for (bp = glbblk; bp; bp = bp->next);
#endif
            goto done;
        }
        else {
            glbwaste += glbsize;
            ++glballoc;
            bp = (struct blk *) malloc( sizeof(struct blk) + 2047 );
            if (bp == NULL) {
                fprintf(AC_DIAG_STREAM, " not enough memory.\n" );
                exit(1);
            }
            memset( (char *)bp, 0, sizeof(struct blk) + 2047 );
            bp->next = glbblk;
            glbblk = bp;
            glbsize = 2048 - siz;
            glbindx = siz;
            mem = (char *) glbblk + BLK_M_OFF;
            goto done;
        }
    }
    else {
        if (locsize > siz) {
            rv = (char *) locblk + BLK_M_OFF + locindx;
            locsize -= siz;
            locindx += siz;
            mem = (char *) rv;
            goto done;
        }
        else {
            locwaste += locsize;
            ++localloc;
            bp = (struct blk *) malloc( sizeof(struct blk) + 2047 );
            if (bp == NULL) {
                fprintf(AC_DIAG_STREAM, " not enough local memory.\n" );
                exit(1);
            }
            memset( (char *)bp, 0, sizeof(struct blk) + 2047 );
            bp->next = locblk;
            locblk = bp;
            locsize = 2048 - siz;
            locindx = siz;
            mem = (char *) locblk + BLK_M_OFF;
            goto done;
        }
    }
done:
    if (mem == NULL) {
        fprintf(AC_DIAG_STREAM, "DIAG -- NULL responce from xalloc\n" );
    }
    return (mem);
}

long
u16_product(a, b)
    long a, b;
{
    unsigned long ua, ub;
    unsigned long prod, add;
    int shift;

    /*
     * Shift-add multiply on 16-bit operands.  SAS/C and ac-self both avoid
     * emitting a .lmuls call here; ac.lib .lmuls misbehaves when ac-self runs.
     */
    ua = (unsigned short) a;
    ub = (unsigned short) b;
    prod = 0;
    add = ua;
    for (shift = 0; shift < 16; shift++) {
        if (ub & 1L)
            prod += add;
        add = add << 1;
        ub = ub >> 1;
    }
    return (long) prod;
}

long
safe_lmul(a, b)
    long a, b;
{
    unsigned long ua, ub;
    unsigned long prod, add;
    int shift;

    /*
     * Full 32-bit shift-add multiply.  getbase() must not call ac.lib .lmuls
     * when ac-self runs: bootstrap/ac/GetSym.s shows one .lmuls in radix parse
     * and poisoned literals (589826, 393216, -65540) follow when it misbehaves.
     */
    ua = (unsigned long) a;
    ub = (unsigned long) b;
    prod = 0;
    add = ua;
    for (shift = 0; shift < 32; shift++) {
        if (ub & 1L)
            prod += add;
        add = add << 1;
        ub = ub >> 1;
    }
    return (long) prod;
}

long
imod2(n)
    long n;
{
    return n & 1L;
}

long
imod3(n)
    long n;
{
    if (n < 0)
        n = -n;
    while (n >= 3)
        n -= 3;
    return n;
}

static unsigned long
udiv_ul(ua, ub)
    unsigned long ua, ub;
{
    unsigned long q;
    unsigned long r;
    int i;

    if (ub == 0)
        return 0;
    q = 0;
    r = 0;
    for (i = 31; i >= 0; i--) {
        r = (r << 1) | ((ua >> i) & (unsigned long)1);
        if (r >= ub) {
            r -= ub;
            q |= (unsigned long)1 << i;
        }
    }
    return q;
}

long
safe_ldiv(a, b)
    long a, b;
{
    unsigned long ua, ub, uq;
    int neg;

    if (b == 0)
        return 0;
    neg = 0;
    if (a < 0) {
        neg = !neg;
        ua = (unsigned long)(-a);
    } else {
        ua = (unsigned long)a;
    }
    if (b < 0) {
        neg = !neg;
        ub = (unsigned long)(-b);
    } else {
        ub = (unsigned long)b;
    }
    uq = udiv_ul(ua, ub);
    if (neg)
        return -(long)uq;
    return (long)uq;
}

long
safe_lmod(a, b)
    long a, b;
{
    long q;

    q = safe_ldiv(a, b);
    return a - safe_lmul(q, b);
}


void
release_local()
{
    struct blk     *bp1, *bp2;
    char           *nbuf;
    int             blkcnt;

    locwaste += locsize;

    blkcnt = 0;
    bp1 = locblk;
    while (bp1 != NULL) {
        bp2 = bp1->next;
        free(bp1);
        ++blkcnt;
        bp1 = bp2;
    }
    locblk = 0;
    locsize = 0;
    lsyms.head = NULL;

    if (!Options.Quiet) {
        if (lastfunc != NULL && lastfunc->name != NULL)
            nbuf = lastfunc->name;
        else
            nbuf = "**PDC**";
        fprintf(AC_DIAG_STREAM, "%s : %d bytes local tables.\n", 
                         nbuf, blkcnt * 2048L );
    }
}

void
release_global()
{
    struct blk     *bp1, *bp2;
    int             blkcnt;

    glbwaste += glbsize;

    bp1 = glbblk;
    blkcnt = 0;
    while (bp1 != NULL) {
        bp2 = bp1->next;
        free(bp1);
        ++blkcnt;
        bp1 = bp2;
    }
    glbblk = 0;
    glbsize = 0;
    gsyms.head = NULL;  /* clear global symbol table */
    strtab = NULL;      /* clear literal table */

    if (!Options.Quiet) {
        fprintf(AC_DIAG_STREAM, " releasing %d bytes global tables\n", 
                         blkcnt * 2048L );
    }
}
