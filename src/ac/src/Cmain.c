
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
#include    "host_posix.h"
#if defined(AC_HOST_POSIX)
#include    <unistd.h>
#endif
#include    "C.h"
#include    "Expr.h"
#include    "Gen.h"
#include    "Cglbdec.h"
#include    "Version.h"
#include    "FrontEnd.h"

/*
 * Crash / CLI breadcrumbs for Amiga self-host (ac-self / ac-self2).
 * Uses dos.library Output()+Write only - no printf/stdio.
 * Default OFF - the strings and Write() calls cost several KB in Cmain.
 * Enable with -DAC_DEBUG=1 when chasing self-host hangs.
 */
#if !defined(AC_DEBUG)
#define AC_DEBUG 0
#endif

#if AC_DEBUG && !defined(AC_HOST_POSIX)
long Output();
long Write();
int fflush();

/*
 * Raw Write(Output()) bypasses the cclib FILE buffer.  Flush first so a
 * mid-line fprintf (e.g. MemMgt "bytes local tables") is not split by
 * an acdbg line when the 1024-byte stdio buffer fills under redirect.
 */
static void
ac_dbg(msg)
char *msg;
{
	long n;
	long out;

	if (msg == 0)
		return;
	if (stdout != 0)
		fflush(stdout);
	n = 0;
	while (msg[n] != '\0')
		n++;
	out = Output();
	if (out != 0)
		Write(out, msg, n);
}

static void
ac_dbg_long(prefix, v)
char *prefix;
long v;
{
	char buf[12];
	char *p;
	unsigned long u;
	int neg;

	ac_dbg(prefix);
	neg = 0;
	if (v < 0) {
		neg = 1;
		u = (unsigned long)(-v);
	} else
		u = (unsigned long)v;
	p = &buf[11];
	*p = '\0';
	if (u == 0)
		*--p = '0';
	else {
		while (u != 0) {
			*--p = (char)('0' + (int)(u % 10));
			u = u / 10;
		}
	}
	if (neg)
		*--p = '-';
	ac_dbg(p);
	ac_dbg("\n");
}

static void
ac_dbg_s(prefix, s)
char *prefix;
char *s;
{
	ac_dbg(prefix);
	if (s == 0)
		ac_dbg("(null)");
	else
		ac_dbg(s);
	ac_dbg("\n");
}

static void
ac_dbg_opt(c, arg)
int c;
char *arg;
{
	char ch[2];

	ac_dbg("acdbg: getopt '");
	ch[0] = (char)c;
	ch[1] = '\0';
	ac_dbg(ch);
	ac_dbg("'\n");
	if (arg != 0)
		ac_dbg_s("acdbg:   optarg=", arg);
}
#else
#define ac_dbg(msg)		((void)0)
#define ac_dbg_long(p, v)	((void)0)
#define ac_dbg_s(p, s)		((void)0)
#define ac_dbg_opt(c, a)	((void)0)
#endif

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
    "AC-Specific Options:\n",
    "\t-a\t\tAnnotate the assembler listing with source\n",
    "\t-b\t\tGenerate inline assembler for builtins\n",
    "\t-f reg\t\tUse address register [4|5|6] as the frame pointer\n",
    "\t-f gcc|sasc|pdc\tSelect error output format\n",
    "\t-l\t\tCreate a source listing file\n",
    "\t-n\t\tTurn off optimization\n",
    "\t-p [0|file]\tCreate or use precompiled header\n",
    "\t-q\t\tRun quietly\n",
    "\t-r\t\tUse library for integer math\n",
    "\t-S\t\tEnable stack checking\n",
    "\t-W error\t\tTreat warnings as errors\n",
    "\t-W no-error\t\tDon't treat warnings as errors\n",
    "\t-W column\t\tShow column numbers in error messages\n",
    "\t-W no-column\t\tDon't show column numbers in error messages\n",
    "\t-W commentnest\tEnable SAS/C nested block comments\n",
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
extern char    *incldir[10];
extern char     prepbuffer[];
extern char    *itoa();
extern char    *litlate();
extern char    *xalloc();
extern void     read_precomp(), dump_precomp(), fmt_precomp();
extern int      comment_nesting;

void  usage(), makename(), summary(), closefiles();

void
add_option(TABLE *tbl, char *cmd)
{
    char    *ptr;
    SYM     *sp;

    ++global_flag;

    sp = (SYM *) xalloc(SZ_SYM);
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

void
default_options(void)
{
    /* OPT_REF only - named Options.field collapses to offset 0 under ac-self. */
    OPT_REF(OPT_OFF_Optimize) = 1;
    OPT_REF(OPT_OFF_Frame) = 5;
    OPT_REF(OPT_OFF_ShowColumn) = 1;
}

int
main(int argc, char **argv)
{
    extern char     optsign;
    extern int      optind;
    extern int      opterr;
    extern char    *optarg;
    /* Static: ac-self aliased used_stdin with loop index i in one register. */
    static int      used_stdin;
    int             i, c, files_processed;
    int             argc0;

    ac_dbg("acdbg: main enter\n");
    ac_dbg_long("acdbg: argc=", (long)argc);
    if (argv == 0) {
        ac_dbg("acdbg: argv is NULL - abort\n");
        exit(1);
    }
    for (i = 0; i < argc; i++) {
        ac_dbg_long("acdbg: argv#", (long)i);
        ac_dbg_s("acdbg:   =", argv[i]);
    }

    opterr = 1;
    used_stdin = FALSE;
    progname = argv[0];
    ac_dbg_s("acdbg: progname=", progname);

    /* Basename "cc" -> POSIX/gcc options + diagnostics; else PDC ac. */
    frontend_set_from_argv0(argv[0]);

    ac_dbg("acdbg: default_options\n");
    default_options();
    if (frontend_mode == FE_CC) {
        /* Force gcc diagnostic shape; skip PDC version banner. */
        OPT_REF(OPT_OFF_OutputFormat) = 0;
    }

#if !defined(AC_HOST_POSIX)
    ac_dbg("acdbg: install_bootstrap_includes\n");
    install_bootstrap_includes();
#endif

#ifdef AC_HOST_POSIX
    open_stdio();
#endif

    argc0 = argc;
    if (frontend_mode == FE_CC) {
        ac_dbg("acdbg: parse_cc begin\n");
        optind = parse_cc_args(argc0, argv);
        ac_dbg("acdbg: parse_cc done\n");
    } else {
    ac_dbg("acdbg: getopt begin\n");
    while ((c = getopt(argc0, argv, "ABGNQRSabglnqrsd:D:F:f:I:L:o:P:p:u:U:cEW:?")) != EOF) {
        ac_dbg_opt(c, optarg);
        switch (c) {
        case 'a':
        case 'A':
            OPT_REF(OPT_OFF_Annote) = !OPT_REF(OPT_OFF_Annote);
            break;
        case 'b':
        case 'B':
            OPT_REF(OPT_OFF_Builtin) = !OPT_REF(OPT_OFF_Builtin);
            break;
        case 'd':
        case 'D':   /* Define a preprocessor Symbol */
            fe_add_define(optarg);
            break;
        case 'u':
        case 'U':   /* Undefine a preprocessor Symbol */    
            fe_add_undef(optarg);
            break;
        case 'g':
        case 'G':
            OPT_REF(OPT_OFF_Debug) = !OPT_REF(OPT_OFF_Debug);
            break;
        case 'I':   /* Preprocessor include directory */
            fe_add_idir(optarg);
            ac_dbg_s("acdbg: include=", optarg);
            break;
        case 'l':
            OPT_REF(OPT_OFF_List) = !OPT_REF(OPT_OFF_List);
            break;
        case 'n':
        case 'N':
            OPT_REF(OPT_OFF_Optimize) = !OPT_REF(OPT_OFF_Optimize);
            break;
        case 'o':
        case 'O':
            strcpy(outfile, optarg);
            ac_dbg_s("acdbg: outfile=", outfile);
            break;
        case 'p':
        case 'P':
            OPT_REF(OPT_OFF_PreComp) = 1;
            if (strcmp(optarg, "0") != 0) {
                OPT_REF(OPT_OFF_PreComp) = 2;
                strcpy(prefile, optarg);
            }
            break;
        case 'q':
        case 'Q':
            OPT_REF(OPT_OFF_Quiet) = !OPT_REF(OPT_OFF_Quiet);
            break;
        case 'r':
        case 'R':
            OPT_REF(OPT_OFF_MulDiv32) = !OPT_REF(OPT_OFF_MulDiv32);
            break;
        case 's':
        case 'S':
            OPT_REF(OPT_OFF_Stack) = !OPT_REF(OPT_OFF_Stack);
            break;
        case 'f':
        case 'F':
            /* Check if it's a format option first */
            if (strcmp(optarg, "gcc") == 0) {
                OPT_REF(OPT_OFF_OutputFormat) = 0;
            } else if (strcmp(optarg, "sasc") == 0) {
                OPT_REF(OPT_OFF_OutputFormat) = 1;
            } else if (strcmp(optarg, "pdc") == 0) {
                OPT_REF(OPT_OFF_OutputFormat) = 2;
            } else {
                /* Otherwise treat as frame pointer option */
                i = atoi(optarg);
                if (i >= 4 || i >= 6)
                    OPT_REF(OPT_OFF_Frame) = i;
            }
            break;
        case 'c':
            OPT_REF(OPT_OFF_CompileOnly) = 1;
            ac_dbg("acdbg: CompileOnly\n");
            break;
        case 'E':
            OPT_REF(OPT_OFF_PreprocessOnly) = 1;
            break;
        case 'L':
            /* Library directory - not implemented yet */
            fprintf(AC_DIAG_STREAM, "%s: -L option not implemented\n", progname);
            break;
        case 'W':
            /* Warning options */
            if (strcmp(optarg, "error") == 0) {
                OPT_REF(OPT_OFF_WarningsAsErrors) = 1;
            } else if (strcmp(optarg, "no-error") == 0) {
                OPT_REF(OPT_OFF_WarningsAsErrors) = 0;
            } else if (strcmp(optarg, "no-column") == 0) {
                OPT_REF(OPT_OFF_ShowColumn) = 0;
            } else if (strcmp(optarg, "column") == 0) {
                OPT_REF(OPT_OFF_ShowColumn) = 1;
            } else if (strcmp(optarg, "commentnest") == 0) {
                /* SAS/C COMMENTNEST - nested block comments. */
                comment_nesting = 1;
            }
            break;
        case '?':
            usage();
            break;
        }
    }
    ac_dbg("acdbg: getopt done\n");
    } /* FE_AC */

    ac_dbg_long("acdbg: optind=", (long)optind);
    ac_dbg_long("acdbg: Quiet=", (long)OPT_REF(OPT_OFF_Quiet));
    ac_dbg_long("acdbg: CompileOnly=", (long)OPT_REF(OPT_OFF_CompileOnly));
    ac_dbg_long("acdbg: Optimize=", (long)OPT_REF(OPT_OFF_Optimize));

    argc -= optind;
    argv += optind;
    ac_dbg_long("acdbg: input files=", (long)argc);
    for (i = 0; i < argc; i++) {
        ac_dbg_long("acdbg: file#", (long)i);
        ac_dbg_s("acdbg:   =", argv[i]);
    }

    for (i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-") == 0) {
            if (used_stdin) {
                fprintf(AC_DIAG_STREAM, "%s: stdin used more than once.\n", progname );
            }
            used_stdin = TRUE;
        }
#if defined(AC_HOST_POSIX)
        else if (access(argv[i], R_OK) == -1) {
            fprintf(AC_DIAG_STREAM, "%s : cannot access %s", progname, argv[i] );
            perror("");
            exit(1);
        }
#endif
    }

    /* Track if any files were successfully processed */
    files_processed = 0;

    for (i = 0; i < argc; i++) {
        ac_dbg_s("acdbg: openfiles ", argv[i]);
        if (openfiles(argv[i])) {
            ac_dbg("acdbg: openfiles ok\n");
            files_processed++;
            lineno = 0;
            ac_dbg("acdbg: initsym\n");
            initsym();

            if (OPT_REF(OPT_OFF_PreComp) == 2) {
                ac_dbg_s("acdbg: read_precomp ", prefile);
                read_precomp(prefile);
            }

            ac_dbg("acdbg: install_defines\n");
            install_defines();

            ac_dbg("acdbg: getch\n");
            getch();
            ac_dbg("acdbg: getsym\n");
            getsym();
            ac_dbg("acdbg: compile\n");
            compile();
            ac_dbg("acdbg: compile returned\n");
            ac_getline(1);

            if (!fatal && OPT_REF(OPT_OFF_PreComp) == 1) {
                strcpy(prefile, infile);
                makename(prefile, ".pre");
                dump_precomp(prefile);
                strcpy(prefile, infile);
                makename(prefile, ".prc");
                fmt_precomp(prefile);
            }

            ac_dbg("acdbg: summary\n");
            summary();
            ac_dbg("acdbg: release_global\n");
            release_global();
            ac_dbg("acdbg: closefiles\n");
            closefiles();
        } else
            ac_dbg("acdbg: openfiles failed\n");
    }

    /* Check if no files were successfully processed */
    if (files_processed == 0) {
        ac_dbg("acdbg: no files compiled\n");
        fprintf(AC_DIAG_STREAM, "%s: error: no files compiled\n", progname);
        exit(1);
    }

#ifdef AC_HOST_POSIX
    close_stdio();
#endif
    ac_dbg("acdbg: main exit\n");
    /* POSIX exit codes: 0=success, 1=error, 2=usage error */
    if (fatal || total_errors > 0)
        exit(1);
    else
        exit(0);
}

void
usage(void)
{
    int             i;

    if (frontend_mode == FE_CC) {
        usage_cc();
        return;
    }

    /* Index walk - not cp++ - so a zero pointer stride cannot hang. */
    for (i = 0; HelpMsg[i] != NULL; i++)
        fputs(HelpMsg[i], AC_DIAG_STREAM);
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
    /*
     * Do not fclose(stderr) here.  That was an ac.lib FILE-table hack.
     * Under cclib, stderr often aliases the same FILE* as stdout
     * (ErrorOutput == Output); closing it makes the next fputs(stdout)
     * hit CClib panic code 20 (stale/forged stream).
     */

    if (!OPT_REF(OPT_OFF_Quiet) && frontend_mode != FE_CC) {
#ifdef AZTEC_C
        fprintf(AC_DIAG_STREAM, VERSION );
        fprintf(AC_DIAG_STREAM, "\nProduced by Paul Petersen and Lionel Hummel.\n" );
        fprintf(AC_DIAG_STREAM, "Based upon prior work by Matthew Brandt and Jeff Lydiatt.\n\n" );
#else
        fputs( VERSION
               "\nProduced by Paul Petersen and Lionel Hummel.\n"
               "Based upon prior work by Matthew Brandt and Jeff Lydiatt.\n"
               "\n", AC_DIAG_STREAM );
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
            fputs(" can't open input ", AC_DIAG_STREAM);
            fputs(infile, AC_DIAG_STREAM);
            fputs("\n", AC_DIAG_STREAM);
            fatal = TRUE;
            return 0;
        }

        /*
         * Test the fopen return in a local - do not round-trip through the
         * global (linker name clashes on _output have caused false NULLs
         * after MODE_NEWFILE already created an empty .s on disk).
         */
        {
            FILE *ofp;

            ofp = fopen(outfile, "w");
            if (ofp == NULL) {
                fprintf(AC_DIAG_STREAM, " can't create output %s\n", outfile);
                fclose(input);
                fatal = TRUE;
                return 0;
            }
            output = ofp;
        }
    }

    if (OPT_REF(OPT_OFF_List)) {
        if ((list = fopen(listfile, "w")) == NULL) {
            fprintf(AC_DIAG_STREAM, " can't create listing %s\n", listfile);
            fclose(input);
            fclose(output);
            fatal = TRUE;
            return 0;
        }
    }

    curfile = infile;

    if (OPT_REF(OPT_OFF_Debug)) {
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
    if (!OPT_REF(OPT_OFF_Quiet)) {
        /* Print summary in modern format */
        if (total_errors == 0 && total_warnings == 0) {
            fprintf(AC_DIAG_STREAM, "Compilation successful.\n");
        } else {
            if (total_errors > 0) {
                if (total_errors == 1)
                    fprintf(AC_DIAG_STREAM, "1 error");
                else
                    fprintf(AC_DIAG_STREAM, "%d errors", total_errors);
            }
            if (total_warnings > 0) {
                if (total_errors > 0)
                    fprintf(AC_DIAG_STREAM, ", ");
                if (total_warnings == 1)
                    fprintf(AC_DIAG_STREAM, "1 warning");
                else
                    fprintf(AC_DIAG_STREAM, "%d warnings", total_warnings);
            }
            fprintf(AC_DIAG_STREAM, " generated.\n");
        }
    }

    if (OPT_REF(OPT_OFF_List))
        fprintf( list, "\f\n *** global scope symbol table ***\n\n" );
    list_table(&gsyms, 0);
    if (OPT_REF(OPT_OFF_List))
        fprintf( list, "\n *** structures and unions ***\n\n" );
    list_table(&tagtable, 0);
    fprintf( output, "\tEND\n" );
}


void
closefiles(void)
{
    fclose(input);
    fclose(output);
    if (OPT_REF(OPT_OFF_List))
        fclose(list);
}
