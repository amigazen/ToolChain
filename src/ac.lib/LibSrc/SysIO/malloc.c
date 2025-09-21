/*
 * Libraries and headers for PDC release 3.3 (C) 1989 Lionel Hummel.
 * PDC Software Distribution (C) 1989 Lionel Hummel and Paul Petersen.
 * PDC I/O Library (C) 1987 by J.A. Lydiatt.
 *
 * This code is freely redistributable upon the conditions that this 
 * notice remains intact and that modified versions of this file not
 * be included as part of the PDC Software Distribution without the
 * express consent of the copyright holders.  No warrantee of any
 * kind is provided with this code.  For further information, contact:
 *
 *  PDC Software Distribution    Internet:                     BIX:
 *  P.O. Box 4006             or hummel@cs.uiuc.edu            lhummel
 *  Urbana, IL  61801-8801       petersen@uicsrd.csrd.uiuc.edu
 */

/* malloc - routines for managing memory
 *
 * calloc     allocates a cleared array of blocks
 * free       frees a previously allocated block of memory
 * malloc     allocates a block of memory
 * realloc    alter buffer size; same contents, new ptr 
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <functions.h>

/* According to X3J11, the response to a request for zero bytes is
   implementation defined.  For PDC, the intention is to return a
   successful result, but one that will cause a trap if it is ever
   dereferenced.  Under most present-day Amiga's, 0xFFFFFFFF (-1L)
   fills the bill just fine.  This may have problems, though, on
   systems with MMU's and very high memory addresses.
 */

#define ZERO_REQUEST -1L

typedef struct memchunk {
    struct memchunk    *next;
    struct memchunk    *prev;
    long                size;
    } MEMCHUNK;

extern void  (*_freeall)();

static MEMCHUNK sentinel = {&sentinel, &sentinel, 0};

/*
 * Called by exit() to free any allocated memory.
 */

static void freeall()
{
    MEMCHUNK    *mp, *mp1;

    for ( mp = sentinel.prev; mp != &sentinel; ) {
        mp1 = mp->prev;
        FreeMem( (APTR) mp, (ULONG) mp->size );
        mp = mp1;
    }
}


char *malloc( size )
unsigned size;
{
    MEMCHUNK *mp;
    ULONG chunksize;    

    if (size == 0)
        return(ZERO_REQUEST);

    chunksize = size + sizeof(MEMCHUNK);
    mp = AllocMem( chunksize, (ULONG) MEMF_CLEAR);
    if ( mp == NULL )
        return NULL;

/* Keep the forward and backward links.
 */
    sentinel.prev->next = mp;
    mp->prev = sentinel.prev;
    sentinel.prev = mp;

    mp->next = &sentinel;
    mp->size = chunksize;
    _freeall = &freeall;
    return ++mp;
}


void free( p )
char *p;
{
    MEMCHUNK *mp, *prevmp, *nextmp;

    mp = p - sizeof(MEMCHUNK);

/* Sanity check: the prev link should point to us. Do nothing if bad.
 */
    prevmp = mp->prev;
    nextmp = mp->next;
    if ( prevmp->next != mp ) {
        return;
    }

    FreeMem( (APTR) mp, (ULONG) mp->size );
    prevmp->next = nextmp;
    nextmp->prev = prevmp;

    return;
}


char *calloc(nelem, elsize)
unsigned int nelem, 
             elsize;
{
    register char *newmem;
    register int   totsize;

    totsize = nelem * elsize;
    newmem = malloc(totsize);
    bzero(newmem, totsize);
    
    return(newmem);
}


char *realloc(ptr, size)
char *ptr;
unsigned size;
{
    MEMCHUNK *mp, *prevmp;
    char *newmem;

    mp = (MEMCHUNK *) (ptr - sizeof(MEMCHUNK));

/* Sanity check: the prev link should point to us. Do nothing if bad.
 */
    prevmp = mp->prev;
    if ( prevmp->next != mp ) {
        return(0);
    }

    newmem = malloc(size);
    size = ( (size > mp->size) ? mp->size : size);

    bcopy(ptr, newmem, size);

    return(newmem);
}
