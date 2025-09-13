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

/* main.c */

/* This startup code prepares the PDC runtime environment.  This involves
 * setting up a UNIX-style device table. It should be preceded by a system 
 * startup such as acrt0 that will parse the command line and set up DOS I/O.
 * If you do not use the PDC I/O libraries, this function is not necessary and
 * may be stubbed to simply call your main().
 */

/* A typical order of execution when executing a program is:
 *
 *	Entry	(Typically, that would be acrt0 as mentioned above)
 *	PDC Startup (This function; internally known as __main())
 *	Your program (Beginning with your main(), internally known as _main())
 *	PDC Cleanup (The C-language exit() function, also called by __main()
 *			in case your _main() decides to rts.
 *	Exit	(Jumps back to acrt0 which competes termination)
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#undef NULL
#include <exec/types.h>
#include <exec/alerts.h>
#include <exec/memory.h>
#include <libraries/dosextens.h>
#include <workbench/startup.h>


extern void (*_fcloseall)(), (*_freeall)();
extern short _numdev;
extern struct _device *_devtab;
extern void *malloc();

long _main(argc, argv)
int   argc;
char *argv[];
{
    /* _fh_stdin and _fh_stdout are set during startup (acrt0) */
    extern long  _fh_stdin,  /* These are DOS filehandles */
                 _fh_stdout,
                 Open();

/* Size and allocate the device table.
 */
    _numdev = OPEN_MAX;
    if ((_devtab = malloc(OPEN_MAX*sizeof(struct _device))) == NULL)  {
        Alert(AG_NoMemory, 0L);
        _exit(ENOMEM);
    }

/* O_STDIO inhibits close() from Close()'ing the DOS file handles we
 * inherited.
 */
    _devtab[0]._mode = O_RDONLY | O_STDIO;
    _devtab[0]._fileHandle = _fh_stdin;
    stdin = fdopen(0, "r");                 /* stdin */
    _devtab[1]._mode = O_WRONLY | O_STDIO;
    _devtab[1]._fileHandle = _fh_stdout;
    stdout = fdopen(1, "w");                /* stdout */

    if ( _devtab[1]._fileHandle ) {
        _devtab[2]._fileHandle = Open("*", MODE_OLDFILE);
        _devtab[2]._mode |= O_WRONLY;    
        stderr = fdopen(2, "w");            /* stderr */
    }

#ifdef INT_MAIN
    exit(main(argc, argv));
#else
    main(argc, argv);
    exit(0);
#endif
}
