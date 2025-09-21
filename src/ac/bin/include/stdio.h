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

/* stdio.h - ANSI C input/output functions */

#ifndef STDIO_H
#define STDIO_H

#include <stddef.h>
#include <stdarg.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

/* File structure */
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

typedef struct _iobuf FILE;

/* Type definitions */
typedef long fpos_t;

/* ANSI C required constants */
#define BUFSIZ 1024
#define EOF (-1)
#define FOPEN_MAX 20
#define FILENAME_MAX 255
#define L_tmpnam 255
#define TMP_MAX 32767

/* File positioning constants */
#define SEEK_CUR 1
#define SEEK_END 2
#define SEEK_SET 0

/* Internal flags */
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

/* ANSI C required macros - using function calls instead of macros for SAS/C compatibility */

/* Macros removed to avoid conflicts with function declarations */

/* Standard streams */
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

/* ANSI C required functions */
void clearerr(FILE *);
int fclose(FILE *);
int feof(FILE *);
int ferror(FILE *);
int fflush(FILE *);
int fgetc(FILE *);
int fgetpos(FILE *, fpos_t *);
char *fgets(char *, int, FILE *);
FILE *fopen(const char *, const char *);
int fprintf(FILE *, const char *, ...);
int fputc(int, FILE *);
int fputs(const char *, FILE *);
size_t fread(void *, size_t, size_t, FILE *);
FILE *freopen(const char *, const char *, FILE *);
int fscanf(FILE *, const char *, ...);
int fseek(FILE *, long int, int);
int fsetpos(FILE *, const fpos_t *);
long int ftell(FILE *);
size_t fwrite(const void *, size_t, size_t, FILE *);
int getc(FILE *);
int getchar(void);
char *gets(char *);
void perror(const char *);
int printf(const char *, ...);
int putc(int, FILE *);
int putchar(int);
int puts(const char *);
int remove(const char *);
int rename(const char *, const char *);
void rewind(FILE *);
int scanf(const char *, ...);
void setbuf(FILE *, char *);
int setvbuf(FILE *, char *, int, size_t);
int sprintf(char *, const char *, ...);
int sscanf(const char *, const char *, ...);
FILE *tmpfile(void);
char *tmpnam(char *);
int ungetc(int, FILE *);
int vfprintf(FILE *, const char *, va_list);
int vprintf(const char *, va_list);
int vsprintf(char *, const char *, va_list);

/* ANSI C required macros - using function declarations instead of macros for SAS/C compatibility */

/* Non-ANSI extensions */
extern FILE *_fdevtab[];
#define MAX_READ_SIZE 1024
FILE *fdopen(int, const char *);

#endif /* STDIO_H */
