/*
 * Libraries and headers for PDC release 3.3 (C) 1989 Lionel Hummel.
 * PDC Software Distribution (C) 1989 Lionel Hummel and Paul Petersen.
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

/* functions.h - Prototypes for Amiga library functions.
 *
 * Honestly, I think these should be provided in the Amiga header files.
 *
 * This list is not yet complete.
 */

/* exec.library */
#ifdef EXEC_TYPES_H
#  ifdef EXEC_PORTS_H
extern struct MsgPort      *CreatePort(char *name);
extern struct MsgPort      *FindPort(char *name);
#  endif
#  ifdef EXEC_TASKS_H
extern struct Task         *FindTask(char *name);
#  endif
#  ifdef EXEC_MEMORY_H
extern APTR                 AllocMem(ULONG, ULONG);
extern void                 FreeMem(APTR, ULONG);
#  endif
#endif

/* dos.library */
#ifdef LIBRARIES_DOS_H
extern long                 DeleteFile(char *name);
#  ifdef LIBRARIES_DOSEXTENS_H
extern int                  Examine(struct FileLock *fl, struct FileInfoBlock *fib);
extern int                  ExNext(struct FileLock *fl, struct FileInfoBlock *fib);
#  endif
extern int                  IoErr(void);
extern struct FileLock     *Lock(char *name, long mode);
extern struct FileHandle   *Open(char *name , long mode);
extern int                  Rename(char *oldName, char *newName);
#  ifdef LIBRARIES_DOSEXTENS_H
extern int                  Seek(struct FileHandle *fh, int pos, int mode);
extern void                 UnLock(struct FileLock *fl);
extern int                  Write(struct FileHandle *fh, char *buf, int len);
#  endif
#endif
