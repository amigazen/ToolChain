
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

#ifdef __PDC__
#ifdef unix
#define free    xfree    /* We do not presently mess with block deallocation */
#define calloc  xcalloc
#endif
#endif

extern char    *itoa();

struct blk {
    struct blk     *next;
    char            m[1];   /* memory area */
};

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

    if (siz > 2048) {
        fprintf( stderr, "DIAG -- xalloc, size greater than 2048\n" );
        exit(1);
    }

    if (global_flag) {
        if (glbsize > siz) {
            rv = &(glbblk->m[glbindx]);
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
                fprintf( stderr, " not enough memory.\n" );
                exit(1);
            }
            memset( (char *)bp, 0, sizeof(struct blk) + 2047 );
            bp->next = glbblk;
            glbblk = bp;
            glbsize = 2048 - siz;
            glbindx = siz;
            mem = (char *) glbblk->m;
            goto done;
        }
    }
    else {
        if (locsize > siz) {
            rv = &(locblk->m[locindx]);
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
                fprintf( stderr, " not enough local memory.\n" );
                exit(1);
            }
            memset( (char *)bp, 0, sizeof(struct blk) + 2047 );
            bp->next = locblk;
            locblk = bp;
            locsize = 2048 - siz;
            locindx = siz;
            mem = (char *) locblk->m;
            goto done;
        }
    }
done:
    if (mem == NULL) {
        fprintf( stderr, "DIAG -- NULL responce from xalloc\n" );
    }
    return (mem);
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
        fprintf( stderr, "%s : %d bytes local tables.\n", 
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
        fprintf( stderr, " releasing %d bytes global tables\n", 
                         blkcnt * 2048L );
    }
}
