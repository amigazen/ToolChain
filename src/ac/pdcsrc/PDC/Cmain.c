
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
 * $Log:	Cmain.c,v $
 * Revision 3.33  90/04/04  02:23:36  lionel
 * Added command line support for stack checking.
 * 
 * Revision 3.32  90/02/03  16:23:09  lionel
 * None
 * 
 *------------------------------------------------------------------
 */

/*
 * Cmain.c
 * 
 * This is the main entry point to the compiler.  It manages compile-time
 * options and the individual steps that are performed during a compilation.
 */

#include    <stdio.h>
#include    "C.h"
#include    "Expr.h"
#include    "Gen.h"
#include    "Cglbdec.h"
#include    "Version.h"

/* #ifdef AZTEC_C */
#if 1
char *HelpMsg[] = {
    VERSION,
    "\nProduced by Paul Petersen and Lionel Hummel.\n",
    "Based upon prior work by Matthew Brandt and Jeff Lydiatt.\n\n",
    "Usage: PDC [ -ABGILNQRP? ] [ -D/U symbol ] [ -F reg ] [ -o file ]\n",
    "           [ -p 0|file ] [ file ... ]\n",
    "\t-a\tAnnotate the assembler listing with source.\n",
    "\t-b\tGenerate inline assembler for builtins.\n",
    "\t-D\tDefine a preprocessor symbol.\n",
    "\t-f\tUse address register [4|5|6] as the frame pointer.\n",
    "\t-g\tGenerate debugging info.\n",
    "\t-I\tDirectory for include files.\n",
    "\t-l\tCreate a source listing file.\n",
    "\t-n\tTurn off optimization.\n",
    "\t-o\tSpecify name for output.\n",
    "\t-p\tCreate or use precompiled header.\n",
    "\t-q\tRun quietly.\n",
    "\t-r\tUse library for integer math.\n",
    "\t-U\tUndefine a preprocessor symbol.\n",
    "\t-?\tThis help information.\n",
    NULL
};
#else
char *HelpMsg =
    VERSION
    "\nProduced by Paul Petersen and Lionel Hummel.\n"
    "Based upon prior work by Matthew Brandt and Jeff Lydiatt.\n\n"
    "Usage: PDC [ -ABGILNQRP? ] [ -D/U symbol ] [ -F reg ] [ -o file ]\n"
    "           [ -p 0|file ] [ file ... ]\n"
    "\t-a\tAnnotate the assembler listing with source.\n"
    "\t-b\tGenerate inline assembler for builtins.\n"
    "\t-D\tDefine a preprocessor symbol.\n"
    "\t-f\tUse address register [4|5|6] as the frame pointer.\n"
    "\t-g\tGenerate debugging info.\n"
    "\t-I\tDirectory for include files.\n"
    "\t-l\tCreate a source listing file.\n"
    "\t-n\tTurn off optimization.\n"
    "\t-o\tSpecify name for output.\n"
    "\t-p\tCreate or use precompiled header.\n"
    "\t-q\tRun quietly.\n"
    "\t-r\tUse library for integer math.\n"
    "\t-U\tUndefine a preprocessor symbol.\n"
    "\t-?\tThis help information.\n";
#endif

/*
 * There are one or two HARMLESS jokes in this beta version of the compiler.
 * Do not worry: there is no effect on code generation or on overall
 * performance.  To the easily affronted: remove this line and please forgive
 * me.  It is very late and my linear thinking mode is shot. (LDH)
 */

#if 0
#define JOKE    1
#endif

char            infile[40], listfile[40], outfile[40], prefile[40];
char           *progname;
int             mainflag;
extern TABLE    tagtable;
extern int      total_errors;
extern int      inclnum;
extern int      fatal;
extern char    *incldir[];
extern char     prepbuffer[];
extern char    *itoa();
extern char    *litlate();
extern char    *xalloc();
extern void     read_precomp(), dump_precomp(), fmt_precomp();

void  usage(), makename(), summary(), closefiles();

void
add_option( tbl, cmd )
    TABLE   *tbl;
    char    *cmd;
{
    char    *ptr;
    SYM     *sp;

    ++global_flag;

    sp = (SYM *) xalloc(sizeof(SYM));
    sp->storage_class = sc_define;
    sp->storage_type = sc_define;
    sp->value.s = NULL;
    sp->tp = NULL;

    for (ptr = cmd; *ptr; ++ptr) 
        if (*ptr == '=')
            break;
    
    if (*ptr == '=') 
        *ptr++ = '\0';

    sp->name = litlate(cmd);

    if (*ptr)
        sp->value.s = litlate( ptr );

    if (tbl->head == NULL) 
        tbl->head = tbl->tail = sp;
    else {
        tbl->tail->next = sp;
        while (tbl->tail->next != NULL)
            tbl->tail = tbl->tail->next;
    }

    --global_flag;
}

main(argc, argv)
    int             argc;
    char          **argv;
{
    extern int      optind;
    extern int      opterr;
    extern char    *optarg;
    extern char     optsign;
    int             used_stdin;
    int             i, c;

    opterr = 1;
    used_stdin = FALSE;
    progname = argv[0];

#ifdef _unix_
    open_stdio();
#endif

    while ((c = getopt(argc, argv, "ABGLNQRSabglnqrsd:D:F:f:I:o:P:u:U:?")) != EOF)
        switch (c) {
        case 'a':
        case 'A':
            Options.Annote = !Options.Annote;
            break;
        case 'b':
        case 'B':
            Options.Builtin = !Options.Builtin;
            break;
        case 'd':
        case 'D':   /* Define a preprocessor Symbol */
            strcpy(prepbuffer, optarg);
            add_option( &cmd_defsyms, prepbuffer );
            break;
        case 'u':
        case 'U':   /* Undefine a preprocessor Symbol */    
            strcpy(prepbuffer, optarg);
            add_option( &cmd_undefsyms, prepbuffer );
            break;
        case 'g':
        case 'G':
            Options.Debug = !Options.Debug;
            break;
        case 'I':   /* Preprocessor include directory */
            ++global_flag;
            strcpy(prepbuffer, optarg);
            for (i = 0; prepbuffer[i]; i++)
                c = prepbuffer[i];
            if (c != ':' && c != '/')
                strcat(prepbuffer, "/");
            incldir[inclnum++] = litlate(prepbuffer);
            --global_flag;
            break;
        case 'l':
        case 'L':
            Options.List = !Options.List;
            break;
        case 'n':
        case 'N':
            Options.Optimize = !Options.Optimize;
            break;
        case 'o':
        case 'O':
            strcpy(outfile, optarg);
            break;
        case 'p':
        case 'P':
            Options.PreComp = 1;
            if (strcmp(optarg, "0") != 0) {
                Options.PreComp = 2;
                strcpy(prefile, optarg);
            }
            break;
        case 'q':
        case 'Q':
            Options.Quiet = !Options.Quiet;
            break;
        case 'r':
        case 'R':
            Options.MulDiv32 = !Options.MulDiv32;
            break;
        case 's':
        case 'S':
            Options.Stack = !Options.Stack;
            break;
        case 'f':
        case 'F':
            i = atoi(optarg);
            if (i >= 4 || i >= 6)
                Options.Frame = i;
            break;
        case '?':
            usage();
            break;
        }

    argc -= optind;
    argv += optind;

    for (i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-") == 0) {
            if (used_stdin) {
                fprintf( stderr, "%s: stdin used more than once.\n", progname );
            }
            used_stdin = TRUE;
        }
#if unix
        else if (access(argv[i], 4) == -1) {
            fprintf( stderr, "%s : cannot access %s", progname, argv[i] );
            perror("");
            exit(1);
        }
#endif
    }

    for (i = 0; i < argc; i++) {
        if (openfiles(argv[i])) {
            lineno = 0;
            initsym();

            if (Options.PreComp == 2)
                read_precomp(prefile);

            install_defines();

            getch();
            getsym();
            compile();
            getline(1);

            if (!fatal && Options.PreComp == 1) {
                strcpy(prefile, infile);
                makename(prefile, ".pre");
                dump_precomp(prefile);
                strcpy(prefile, infile);
                makename(prefile, ".prc");
                fmt_precomp(prefile);
            }

            summary();
            release_global();
            closefiles();
        }
    }

#ifdef _unix_
    close_stdio();
#endif
    exit(fatal || (total_errors != 0));
}

void
usage()
{
/* #ifdef AZTEC_C */
#if 1
    char **cp = HelpMsg;

    while (*cp) {
        fprintf( stderr, *cp++ );
    }
#else
    fputs(HelpMsg, stderr);
#endif
    exit(1);
}

void
formsection( buffer, name, ext )
    char    *buffer, *name, *ext;
{
    while (*name && *name != '.')
        *buffer++ = *name++;

    while (*ext)
        *buffer++ = *ext++;

    *buffer = '\0';
}

int
openfiles(s)
    char           *s;
{
    if (!Options.Quiet) {
#ifdef AZTEC_C
        fprintf( stderr, VERSION );
        fprintf( stderr, "\nProduced by Paul Petersen and Lionel Hummel.\n" );
        fprintf( stderr, "Based upon prior work by Matthew Brandt and Jeff Lydiatt.\n\n" );
#else
        fputs( VERSION
               "\nProduced by Paul Petersen and Lionel Hummel.\n"
               "Based upon prior work by Matthew Brandt and Jeff Lydiatt.\n"
               "\n", stderr );
#endif
    }

    if (strcmp(s, "-") == 0) {  /* Read from stdin */
        strcpy(infile, "stdin");
        if (outfile[0] == '\0')
            strcpy(outfile, "stdout");
        strcpy(listfile, "stdlist");

        input = stdin;
        output = stdout;
    }
    else {
        strcpy(infile, s);
        strcpy(listfile, s);
        makename(listfile, ".lis");

        if (outfile[0] == '\0') {
            strcpy(outfile, s);
            makename(outfile, ".s");
        }

        if ((input = fopen(infile, "r")) == 0) {
            fputs(" can't open ", stderr);
            fputs(infile, stderr);
            fputs("\n", stderr);
            fatal = TRUE;
            return 0;
        }

        if ((output = fopen(outfile, "w")) == NULL) {
            fprintf( stderr, " can't open %s\n ", outfile );
            fclose(input);
            fatal = TRUE;
            return 0;
        }
    }

    if (Options.List) {
        if ((list = fopen(listfile, "w")) == NULL) {
            fprintf( stderr, " can't open %s\n", listfile );
            fclose(input);
            fclose(output);
            fatal = TRUE;
            return 0;
        }
    }

    curfile = infile;

    if (Options.Debug) {
        formsection( code_name, infile, "_code" );
        formsection( data_name, infile, "_data" );
        formsection( bss_name, infile, "_bss" );
    }

    return 1;
}

void
makename(s, e)
    char           *s, *e;
{
    char    *p = NULL;

    if (s != NULL && e != NULL) {
        while (*s) {
            if (*s == '.')
                p = s;
            ++s;
        }
        if (p == NULL)
            p = s;

        while (*p++ = *e++);
    }
}

void
summary()
{
    if (!Options.Quiet) {
#ifdef JOKE
        if (total_errors == 1) {
            fputs("\n *** error 65536 Illegal Number of Errors: 1\n", stderr);
            total_errors++;
        }
#endif
        fprintf( stderr, "\n -- " );
        if (total_errors == 0)
            fprintf( stderr, "no errors found." );
        else {
            if (total_errors == 1)
                fprintf( stderr, "1 error found." );
            else
                fprintf( stderr, "%d errors found.", total_errors );
        }
    }

    if (Options.List)
        fprintf( list, "\f\n *** global scope symbol table ***\n\n" );
    list_table(&gsyms, 0);
    if (Options.List)
        fprintf( list, "\n *** structures and unions ***\n\n" );
    list_table(&tagtable, 0);
    fprintf( output, "\tEND\n" );
}


void
closefiles()
{
    fclose(input);
    fclose(output);
    if (Options.List)
        fclose(list);
}
