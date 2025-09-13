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

/*  bufmgt.c
 *
 *  Performs buffer management for PDC's stdio facilities
 */

#include <stdio.h>

extern void (*_fcloseall)(), free();
extern int fclose();
extern int read(), write();

/*    Our internal routine to close all buffered I/O
 */
static void fcloseall()
{
    FILE *fp, *fp2;

    for ( fp = (FILE *) &_fdevtab[0]; fp != NULL; fp = fp2 ) {
        fp2 = fp->next;
        fclose( fp );
    }
}


FILE *addStream()
{
    FILE *fp, *fp1;
    extern char *malloc();

    /* If we're going to open a stream, there should be a routine to
     * close it before the program exits.  If there isn't one already,
     * provide our own.
     */
    if (!_fcloseall)
        _fcloseall = &fcloseall;

    /*
     * Try to nab an unused buffer
     */
    fp = (FILE *) &_fdevtab[0];
    while( fp != 0L ) {
        fp1 = fp;
        if ( !fp->_fileflag )
            break;
        fp = fp->next;
    }

    /*
     * If fp is null, then there isn't an empty buffer - make one
     * and link it into the chain.
     */

    if ( fp == NULL ) {
        fp = (FILE *) malloc(sizeof(FILE));
        if ( fp == NULL )
            return NULL;
        fp1->next = fp;
        fp->next = NULL;
    }

    fp->_filecpos = NULL;
    fp->_fileend = NULL;
    fp->_filebufp = NULL;
    fp->_fileflag = 0;
    return fp;
}


/* Allocate a buffer to use with the given I/O stream:
 */
addStreamBuf( fp )
FILE *fp;
{
    char *buffer;
    extern char *malloc();

    if ( !(fp->_filebufp = malloc(BUFSIZ)) ) {
        fp->_filebufp = &fp->_filebyte;
        fp->_filelen = 1;
    } else {
        fp->_fileflag |= _FILEISDYNA;
        fp->_filelen = BUFSIZ;
    }

    if ( isatty((int)fp->_fileunit) )
        fp->_fileflag |= _FILEISTTY;
    
    return;
}


int _filebfill( fp )
FILE *fp;
{
    FILE *fp2;
    int   len;

    if ( fp->_fileflag & (_FILEATEOF | _FILEBAD ) )
	return EOF;

    fp->_fileflag &= ~_FILEISDIRTY;

    if ( fp->_filebufp == NULL )
	addStreamBuf( fp );

    if ( fp->_fileflag & 0x80 ) {
	for ( fp2= (FILE *) &_fdevtab[0]; fp2; fp2 = fp2->next)
	    if ( (fp->_fileflag & (0x80|_FILEISDIRTY)) == (0x80|_FILEISDIRTY) )
		_doflush( fp2, -1 );
    }

    len = read( (int)fp->_fileunit, fp->_filebufp, (int)fp->_filelen);
    if ( len > 0 ) {
	fp->_filecpos = fp->_filebufp;
	fp->_fileend = fp->_filebufp + len;
	return (*fp->_filecpos++ & 0xFF);
    }

    if ( len < 0 )
	fp->_fileflag |= _FILEBAD;
    else
	fp->_fileflag |= _FILEATEOF;
    return EOF;
}


int _doflush( fp, data )
FILE *fp;
int  data;
{
    int  nchars;
    char c = data;

    if ( fp->_fileflag & _FILEBAD )
        return EOF;
    if ( fp->_fileflag & _FILEISDIRTY ) {
        nchars = fp->_filecpos - fp->_filebufp;
        if ( write( (int)fp->_fileunit, fp->_filebufp, nchars ) != nchars )
            goto err;
    }

    if ( data == EOF ) {
        fp->_fileflag &= ~_FILEISDIRTY;
        fp->_filecpos = NULL;
        fp->_fileend  = NULL;
        return 0;
    }

    if ( !fp->_filebufp )
        addStreamBuf( fp );
    if ( fp->_filelen == 1 ) {
        if ( write( (int)fp->_fileunit, &c, 1 ) != 1 )
            goto err;
        return data;
    }

    fp->_filecpos = fp->_filebufp;
    fp->_fileend  = fp->_filebufp + fp->_filelen;
    fp->_fileflag |= _FILEISDIRTY;
    *fp->_filecpos++ = data;
    return data;

err:
    fp->_fileflag |= _FILEBAD;
    fp->_filecpos = NULL;
    fp->_fileend  = NULL;
    return EOF;
}
