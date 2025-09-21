
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
#include    <stdlib.h>
#include    <string.h>
#include    <unistd.h>
#include    "C.h"
#include    "Expr.h"
#include    "Gen.h"
#include    "Cglbdec.h"
#include    "Version.h"

char *HelpMsg[] = {
    VERSION,
    " by amigazen project\n",
    "Based upon prior work by Paul Petersen, Lionel Hummel, Matthew Brandt and Jeff Lydiatt.\n\n",
    "Usage: AC [options] file...\n\n",
    "\t-c\t\tCompile source files without linking\n",
    "\t-D name[=value]\tDefine a preprocessor symbol\n",
    "\t-E\t\tPreprocess only; do not compile, assemble or link\n",
    "\t-g\t\tGenerate debugging information\n",
    "\t-I directory\tAdd directory to include search path\n",
    "\t-L directory\t(Not Implemented) Add directory to library search path\n",
    "\t-o outfile\tSpecify output file name\n",
    "\t-O\t\tOptimize code (default: ON, use -n to turn off)\n",
    "\t-s\t\t(Not Implemented) Strip symbol information from output\n",
    "\t-U name\t\tUndefine a preprocessor symbol\n\n",
    "PDC-Specific Options:\n",
    "\t-a\t\tAnnotate the assembler listing with source\n",
    "\t-b\t\tGenerate inline assembler for builtins\n",
    "\t-f reg\t\tUse address register [4|5|6] as the frame pointer\n",
    "\t-l\t\tCreate a source listing file\n",
    "\t-n\t\tTurn off optimization\n",
    "\t-p [0|file]\tCreate or use precompiled header\n",
    "\t-q\t\tRun quietly\n",
    "\t-r\t\tUse library for integer math\n",
    "\t-S\t\tEnable stack checking\n",
    "\t-?\t\tThis help information\n",
    NULL
};



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
add_option(TABLE *tbl, char *cmd)
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

int
main(int argc, char **argv)
{
    extern char     optsign;
    extern int      optind;
    extern int      opterr;
    extern char    *optarg;
    int             used_stdin;
    int             i, c;

    opterr = 1;
    used_stdin = FALSE;
    progname = argv[0];

#ifdef _unix_
    open_stdio();
#endif

    while ((c = getopt(argc, argv, "ABGLNQRSabglnqrsd:D:F:f:I:L:o:P:u:U:cE?")) != EOF)
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
        case 'c':
            Options.CompileOnly = 1;
            break;
        case 'E':
            Options.PreprocessOnly = 1;
            break;
        case 'L':
            /* Library directory - not implemented yet */
            fprintf(stderr, "%s: -L option not implemented\n", progname);
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
usage(void)
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
formsection(char *buffer, char *name, char *ext)
{
    while (*name && *name != '.')
        *buffer++ = *name++;

    while (*ext)
        *buffer++ = *ext++;

    *buffer = '\0';
}

int
openfiles(char *s)
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
makename(char *s, char *e)
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
summary(void)
{
    if (!Options.Quiet) {

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
closefiles(void)
{
    fclose(input);
    fclose(output);
    if (Options.List)
        fclose(list);
}
