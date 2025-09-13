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

/*    system.c
 *
 *    system() - Executes a program within the context of the current process
 *    _PathAccess() - Attempts access of file along a trial path
 *
 *    Note: The following two routines are a non-optimal solution to the
 *          need to find the full path name given only a directory lock.
 *
 *    _SearchPath() - Attempts to locate a file along the current CLI path
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/alerts.h>
#include <libraries/dosextens.h>
#include <ctype.h>

#define MAXPATH (1024L)

extern long LoadSeg();
extern void UnLoadSeg();

char *_SearchPath();
char *_PathAccess();

int system(line)
char *line;
{
    char *buffer;
    char *command;
    char *args;
    char *ptr;
    char scan_for;
    long segment_list;
    long return_code;

    buffer = malloc(strlen(line)+1+3+1); /* null + space to LW align + LF */
    if (!(ptr = buffer)) {
        Alert(AG_NoMemory, 0L);
        return(-1);
    }

    while (isspace(*line))
        line++;

    if (*line == '"')    {
        scan_for = '"';
        line++;
    }
    else
        scan_for = ' ';

    while (*ptr = *line)    {
        if ((isspace(*line) && (scan_for == ' ')) || (*line == scan_for))
            break;
        else    {
            ptr++;
            line++;
        }
    }

    *ptr++ = '\0';
    args = ptr = (ptr+3) & 0xfffffffc;  /* Longword align */

    while (isspace(*line))
        line++;
    while (*ptr = *line)
        ptr++, line++;
    *ptr++ = '\n';
    *ptr++ = '\0';

    command = _SearchPath(buffer);
    if (!command) {
        free(buffer);
        return(-1);
    }

    segment_list = LoadSeg(command);
    if (!segment_list) {
        free(buffer);
        free(command);
        return(-1);
    }

    return_code = System0(command, segment_list, args);

    UnLoadSeg(segment_list);
    free(buffer);
    free(command);
    return(return_code);
}

char *_SearchPath(target)
char *target;
{
    struct CommandLineInterface *this_cli;
    struct FileLock *pl;
    struct FileLock *nextpl;
    struct FileInfoBlock *fib;
    char *fullpath = 0L;
    char *buffer;
    struct {
        BPTR NextPath;
        BPTR PathLock;
    } *p_chain;
    unsigned int count;

/* Check current directory:
 */
    fullpath = _PathAccess("", target);
    if (fullpath)
        return(fullpath);

/* Then check search path:
 */
    this_cli = BADDR(((struct Process *)FindTask(0L))->pr_CLI);
    if (this_cli == 0L) {
        return(0L);
    }

    p_chain = BADDR(this_cli->cli_CommandDir);

    buffer = malloc(MAXPATH+2); /* Path + nul + initial slosh byte */
    if (buffer == 0L) {
        Alert(AG_NoMemory, 0L);
        return(0L);
    }


    fib = malloc(sizeof(struct FileInfoBlock));
    if (!fib) {
        Alert(AG_NoMemory, 0L);
        return(0L);
    }

    while (p_chain)    {
        pl = DupLock(p_chain->PathLock);

        buffer[MAXPATH+1] = '\0';
        fullpath = &buffer[MAXPATH];

        while (pl) {
            if (Examine(pl, fib)) {
                if (nextpl = ParentDir(pl))
                    *--fullpath = '/';
                else
                    *--fullpath = ':';
                fullpath -= (count = strlen(fib->fib_FileName));
                if (fullpath <= buffer) {
                    UnLock(pl);
                    if (nextpl)
                        UnLock(nextpl);
                    goto next_path;
                }
                bcopy( fib->fib_FileName, fullpath, count );
                UnLock(pl);
                pl = nextpl;
            }
            else {
                UnLock(pl);
                goto next_path;
            }
        }
 
        fullpath = _PathAccess(fullpath, target);
        if (fullpath) {
            UnLock(nextpl);
            free(buffer);
            free(fib);
            return(fullpath);
        }

next_path:
        p_chain = BADDR(p_chain->NextPath);
    }

/* If that fails, default to c:
 */
    fullpath = _PathAccess("c:", target);

    free(buffer);
    free(fib);
    return(fullpath);  /* Gets 0L from _PathAccess if failed */

}


char *_PathAccess(path, target)
char *path;
char *target;
{
    char *buf;
    struct FileLock *fl;

    buf = malloc(1024);
    if (buf == 0L) {
        Alert(AG_NoMemory, 0L);
        return(NULL);
    }

    buf[0] = '\0';
    strcat(buf, path);
    strcat(buf, target);
    if (fl = Lock(buf, ACCESS_READ)) {
        UnLock(fl);
        return(buf);
    }

    free(buf);
    return(0L);
}
