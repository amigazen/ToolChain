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

/*  stdio.h
*/

#ifndef STDIO_H

#ifndef NULL
#define NULL	0L
#endif

struct _iobuf {
   struct _iobuf *next;	        /* Link to next buffer in chain	          */
   int   _fileunit;             /* File descriptor                        */
   char *_filecpos;		/* Current position in buffer		  */
   char *_fileend;		/* Last position in buffer		  */
   long  _filelen;		/* Buffer size				  */
   char *_filebufp;           	/* za buffer                              */
   char  _filebyte;		/* Instant one-byte buffer                */
   char  _padbyte;
   unsigned short _fileflag;    /* Condition flags                        */
};

#define FILE struct _iobuf
extern FILE *_fdevtab[];

#define BUFSIZ          (1024)
#define MAX_READ_SIZE	(1024)

#define _FILEACTIVE	0x01
#define _FILEISDIRTY	0x02
#define _FILEATEOF	0x04
#define _FILEBAD	0x08
#define _FILEISDYNA	0x10
#define _FILEISTTY	0x20

#define _IOREAD         0x1
#define _IOWRT          0x2
#define _IOEOF          0x10
#define _IOERR          0x20
#define _IORW           0x100

#define EOF     (-1)
	
#define feof(p)         (((p)->_fileflag&_FILEATEOF)!=0)
#define ferror(p)       (((p)->_fileflag&_FILEBAD)!=0)
#define fileno(p)       ((p)->_fileunit)
#define clearerr(p)     ((p)->_fileflag &= ~(_FILEBAD|_FILEATEOF))

#define getc(c)		fgetc((c))
#define getchar()	fgetc(stdin)
#define putc(c,s)	fputc((c),(s))
#define putchar(c)	fputc((c), stdout)

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

#if 0
extern FILE *fopen(char *, char *);
extern FILE *fdopen(int, char *);
extern int   fgetc(FILE *);
extern int   ungetc(char, FILE *);
extern char *fgets(char *, int, FILE *);
#else
extern FILE *fopen();
extern FILE *fdopen();
extern int   fgetc();
extern int   ungetc();
extern char *fgets();
#endif

#define STDIO_H
#endif
