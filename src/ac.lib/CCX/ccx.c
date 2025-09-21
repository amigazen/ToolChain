/*
 *	CCX -- C compiler front end for Amiga PDC.
 *
 *	Very PDC/A68k/Blink dependent.  CCX was adapted from the
 *	Lattice and Aztec C versions by Fred Fish on Fish #2, and  
 *	#28 respectively by Jeff Lydiatt.
 *	Changes from Fred's version (C) Copyright 1988 by Jeff Lydiatt.
 *	Massive reworking of Jeff's version Copyright (c)1988 Lionel Hummel.
 *
 */

/*
 * Maintenance Notes:
 * 06Mar88 - (Jal) Only look in <current directory>, "PDC:", or "RAM:". 
 * 15Aug88 - (LDH) Assumes integrated Cpp; external Cpp is an option (-Y).
 * 20Nov88 - (LDH) Extensive rewrite.  Removes a lot of hardcoded limits, adds
 *              many new options for improved UNIX and PDC functionality.
 *              Still has many Execute()-related problems, though.
 * 02Jan89 - (LDH) Devised crufty workaround to difficulties with Execute() and
 *              return codes: the environment variable LASTERR (ick!).
 * 22Jan89 - (LDH) PDC's system() now working.  No I/O redirection yet, but
 *              we've now got return codes and forwarded signals.  Supercedes
 *              previous workaround that used LASTERR.
 * 11Jul89 - (LDH) ParseCommandLine() now restarts getopt() to pick up any 
 *              trailing options, as in "ccx fahr.c -lmath".
 */

#include <libraries/dosextens.h>
#include <exec/lists.h>

#include <stdio.h>
#include <fcntl.h>

struct Task *FindTask();

/*
 *	Manifest constants which can be tuned to fit requirements.
 */

#define CMDBUFFERSIZE (1024)	/* Size of command buffer for CLI command */
#define ARGSIZE (64)		/* Size of temp args for cmd line */

/*
 *	Define QUADDEV to be the default place where you want the compiler
 *	intermediate files (quad files) to be created.  For systems with
 *	512K or more, the preferred place is the ram disk.  However, for
 *	really big source files (>100K), a floppy or hard disk may be needed.
 *	In either case, the default can be overridden with the -q option.
 */
 
#define DEF_QUADDEV	"RAM:"		/* Keep intermediate files in ram */
#define DEF_STARTUP	"acrt0.o"	/* Standard startup file to link */

#define DEF_COMPILER_NAME	"PDC"
#define DEF_ASSEMBLER_NAME	"A68k"
#define DEF_LINKER_NAME		"BLink"
#define DEF_EXT_PREPROC_NAME	"Cpp"

#define DEF_ENV_LIBNAME		"PDCLibs"
#define DEF_ENV_LIBDIRNAME	"PDCLibDirs"
#define DEF_ENV_INCDIRNAME	"PDCIncDirs"
#define DEF_ENV_STARTUP		"PDCStartup"
#define DEF_ENV_QUADDEV		"PDCTmpArea"

char *Compiler = DEF_COMPILER_NAME;
char *Assembler = DEF_ASSEMBLER_NAME;
char *Linker = DEF_LINKER_NAME;
char *StartupFile = DEF_STARTUP;
char *PreProc = DEF_EXT_PREPROC_NAME;

/*	User specified arguments to send at each stage:
 */
char *UCompilerArgs = NULL;
char *UAssemblerArgs = NULL;
char *ULinkerArgs = NULL;
char *UPreProcArgs = NULL;

/*
 *	Manifest constants which are generally the same on all systems.
 */

#define EOS '\000'		/* End of string character */

/*
 *	Command line arguments that represent files to be compiled, assembled,
 *	or linked, are kept track of via an "Operand" list.  If, for example,
 *	the file name is "df0:mydir/junk.c", then the Rootname is
 *	"df0:mydir/junk", the Basename is "junk", and the Suffix is "c".
 *	String suffixes are used, rather than single character suffixes, to
 *	allow use of names with multicharacter suffixes.
 */

char *my_name;			/* Allows global access to argv[0] */

struct Operand {		/* Info about each operand (non option) */
    struct Node Node;		/* Node point for list attachment */
    char *Rootname;		/* Name minus any suffix */
    char *Basename;		/* Name minus any prefix or suffix */
    char *Suffix;		/* Suffix of operand */
};

struct List Includes;
struct List Defines;
struct List Undefines;
struct List Libs;
struct List LibDirs;
struct List Operands;

int NumOps = 0;

char *OptChars = "?aAcD:gI:l:mOo:q:Ss:UVW;w:Y;";

int GlobalStatus;			/* -1 = Ok, 0 = Oh, Oh. */

char Command[CMDBUFFERSIZE];		/* Command line buffer */
char *EndCommand = Command;		/* End of current command string */

/*
 *	Macros to determine the suffix type of a file given a pointer to
 *	its operand structure.
 */
#define CFILE(op) (strcmp(op->Suffix,"c")==0)
#define SFILE(op) (strcmp(op->Suffix,"s")==0 || strcmp(op->Suffix,"a")==0 \
		  || strcmp(op->Suffix,"asm")==0)
#define OFILE(op) (strcmp(op->Suffix,"o")==0 || strcmp(op->Suffix,"obj")==0)
extern int strcmp ();

extern char *strrchr();
extern char *fgets();

/*
 *	Now some macros to map from unix names to the AMIGA equivalents,
 *	and to enable abort on control-C.
 */
 
extern short Enable_Abort;
#define ENABLE_ABORT (Enable_Abort = 1)
#define DISABLE_ABORT (Enable_Abort = 0)
#define CHECK_ABORT Chk_Abort()

/*    Set list of places to search for various executables, libraries, etc.
 *    Searched in order, first match wins, null string is current directory.
 */
char *DefLibs[] = {
    "PDC.lib",
    "Amiga.lib",
    "Math.lib",
    NULL
};

char *DefLibDirs[] = {
    "",
    "PDC:lib",
    NULL
};

char *DefIncDirs[] = {
    "PDC:include",
    "RAM:include",
    NULL
};

#define BRIEF 1
#define COMPLETE 2
#define BOTH (BRIEF|COMPLETE)

struct ConditionalString {
    int   mask;
    char *description;
} HelpStrings[] = {
    { BRIEF, "-?	Descriptions of ALL options.\n"},
    { COMPLETE, "-?	This help message.\n"},
    { COMPLETE, "-a	Preserve .s files\n"},
    { COMPLETE, "-A	Annotate assembly output with source (implied -a)\n"},
    { BOTH, "-c	Suppress link phase, preserve object files.\n"},
    { BOTH, "-D %s	Predefine option to preprocessor.\n"},
    { COMPLETE, "-g	Generate symbols for debugging.\n"},
    { BOTH, "-I %s	Prepend directory %s in search for include files\n"},
    { BOTH, "+I %s	Append directory %s in search for include files\n"},
    { BOTH, "-l %s	Prepend library %s to list of libraries to scan\n"},
    { BOTH, "+l %s	Append library %s to list of libraries to scan\n"},
    { COMPLETE, "-m	Instruct linker to generate map file\n"},
    { BOTH, "-o %s	Name the output %s\n"},
    { COMPLETE, "-q %s	Use %s to temporarily store .s files.\n"},
    { COMPLETE, "-S	Do not run assembler after creating .s files\n"},
    { COMPLETE, "-s %s	Link using object file %s as the startup file\n"},
    { COMPLETE, "-U %s	Undefine option %s to preprocessor\n"},
    { COMPLETE, "-V	Be verbose.  Print out lines as they're executed.\n"},
    { COMPLETE, "-W%c %s	Prepend %s to the argument list for phase %c\n"},
    { COMPLETE, "-w %s	Link with file %s, containing the linker's arguments\n"},
    { COMPLETE, "-Y%c %s	Use %s for phase %c.  Default: %c=p %s='Cpp'\n"},
    { BOTH, "\n  %s is any string (preceding whitespace optional)\n"},
    { COMPLETE, "  %c is a character	p=preprocessor	0|c=compiler\n"},
    { COMPLETE, "			a=assembler	l=linker\n"},
    { 0, NULL }
    };

char *NoMem = "Out of memory";

/*
 *	Flags set by command line arguments:
 */
 
int aflag = 0;			/* -a flag given */
int Aflag = 0;			/* -A flag given */
int cflag = 0;			/* -c flag given */
int gflag = 0;
int Eflag = 0;			/* -E flag given */
int mflag = 0;			/* -m flag given */
int Sflag = 0;			/* -S flag given */
int Vflag = 0;			/* -V flag given (non-standard) */
int xternCpp = 0;		/* -x flag given */

int ErrCount = 0;		/* Count of compile/assemble errors */
char *OutFile = NULL;		/* Output file name from linker */
char *QuadDev = NULL;		/* Where to keep quad files */
char *WithFile = NULL;		/* No particular file to link with */

char *predefines = "-Damiga -Dm68000";

char *Locate ();		/* Find a file */
void AddToCommand ();		/* Add argument to command buffer */
void InitCommand ();		/* Initialize command buffer */
void Fatal ();			/* Quit with fatal error */
void Warning ();		/* Issue warning message */
void AddOperandToList ();	/* Add .c, .s, or .o file to list */
void CleanObjects ();		/* Remove .o for link and go mode */
void MakeObjects ();		/* Reduce to list of object files */
void ParseCommandLine ();	/* Deal with command line */
int Compile ();			/* Translate from .c to .o */
int Assemble ();		/* Translate from .s to .o */
void Link ();			/* Gather .o's into executable */

/*	Environment-handling routines in file Environment.c:
 */
extern char *getenv();
extern char *ParseEnv();

extern char *malloc();

main (argc, argv)
int argc;
char *argv[];
{
    auto char buffer[ARGSIZE];
    struct Operand *op;
    struct Node *NewNode;
    char **ipath;
    char *Str;
    char *StdLibList;
    char *StdIncList;

    if (argc == 0)
	exit(0);		/* Not ready for Workbench yet */

    my_name = argv[0];

    if (argc==1)	{
	printf("Missing command line arguments.\n");
	Usage(BRIEF);
	exit(1);
	}

    GlobalStatus = -1;

    NewList(&Operands);

    NewList(&Libs);
    GenList(&Libs, DEF_ENV_LIBNAME, DefLibs);

    NewList(&Defines);
    NewList(&Undefines);

    NewList(&Includes);
    GenList(&Includes, DEF_ENV_INCDIRNAME, DefIncDirs);

    NewList(&LibDirs);
    GenList(&LibDirs, DEF_ENV_LIBDIRNAME, DefLibDirs);

    QuadDev = getenv(DEF_ENV_QUADDEV);
    if (QuadDev == NULL) {
        QuadDev = DEF_QUADDEV;
    }

    ParseCommandLine (argc, argv);
    MakeObjects();

/*    If everything looks good, then link the object modules, and, 
 *    as per normal UNIX cc behavior, clean up .o file after a single
 *    source file compile.
 */
    if (!cflag && !Eflag && !Sflag && (ErrCount == 0) && GlobalStatus) {
	putchar('\n');
	Link ();
    
        if ((Operands.lh_Head == Operands.lh_TailPred) && (ErrCount == 0)) {
            op = (struct Operand *) Operands.lh_Head;
            if (CFILE (op) || SFILE (op)) {
                sprintf (buffer, "%s.o", op -> Basename);
                if (!DeleteFile (buffer)) {
                    Warning ("can't delete '%s'", buffer);
                }
            }
        }
    }

}


void ParseCommandLine (argc, argv)
int argc;
char **argv;
{
    extern char optsign;
    extern char optqual;        /* Option qualifier (for -W and -Y) */
    extern int optind;
    extern int opterr;
    extern char *optarg;
    int i, c;
    struct Node *NewNode;

    opterr = 1;

    CHECK_ABORT;
    do { 
        while ( (c = getopt(argc, argv, OptChars)) != EOF) {
            switch (c) {
              case '/':           /* Returned by getopt() upon a user error */
                Usage(BRIEF);
                break;
              case '?':  
                Usage(COMPLETE);
                exit(0);
              case 'A':   
                Aflag++;        /* Falls through to add -a too! */
              case 'a':
                aflag++;
                break;
              case 'c':
                cflag++;
                break;
              case 'D':
                NewNode = (struct Node *) malloc(sizeof(struct Node));
                if (NewNode)	{
                    NewNode->ln_Name = optarg;
		    AddTail(&Defines, NewNode);
                }
                else
                    Fatal(NoMem);
		break;
              case 'g':
                gflag++;
                break;
              case 'I':
                if (Readable(optarg))	{
                    NewNode = (struct Node *) malloc(sizeof(struct Node));
                    if (NewNode)	{
                        NewNode->ln_Name = optarg;
                        if (optsign == '+')
                           AddTail(&Includes, NewNode);
                        else
                            AddHead(&Includes, NewNode);
                    }
                    else
                        Fatal(NoMem);
                }
                else
                    Warning("opt I: No such directory: %s", optarg);
                break;
              case 'l':
                NewNode = (struct Node *) malloc(sizeof(struct Node));
                if (NewNode) {
                    NewNode->ln_Name = malloc(strlen(optarg));
                    if (NewNode->ln_Name)    {
                        strcpy(NewNode->ln_Name, optarg);
                        if (optsign == '+')
                            AddTail(&Libs, NewNode);
                        else
                            AddHead(&Libs, NewNode);
                    }
                    else
                        Fatal(NoMem);
                }
                else
                    Fatal(NoMem);
		break;
              case 'm':		/* Generate a link map file */
                mflag++;
                break;
              case 'O':           /* Swallow it */
                break;
              case 'o':		/* Specify name of output file 	*/
                OutFile = optarg;
                break;
              case 'q':		/* Select a place for tmp .s files */
                QuadDev = optarg;
                break;
              case 'S':		/* Do not go on to assembly */
                Sflag++;
                break;
              case 's':		/* Set startup file */
                if (strcmp(optarg, "0") == 0)
                    StartupFile = NULL;
                else
                    StartupFile = optarg;
                break;
              case 'U':
                NewNode = (struct Node *) malloc(sizeof(struct Node));
                if (NewNode)	{
                    NewNode->ln_Name = optarg;
                    AddTail(&Undefines, NewNode);
                }
                else
                    Fatal(NoMem);
                break;
              case 'V':		/* Verbose */
                Vflag++;
                break;
              case 'v':
                Vflag=0;        /* Silent */
                break; 
              case 'W':   /* Send command line options to: */
                        /* p preprocessor (sent to external preproc only)
                           0 compiler     ('c' also)
                           a assembler
                           l link editor 
                         */
                switch (optqual)	{
                    case 'p':
                        UPreProcArgs = optarg;
                        break;
                    case '0':
                    case 'c':
                        UCompilerArgs = optarg;
                        break;
                    case 'a':
                        UAssemblerArgs = optarg;
                        break;
                    case 'l':
                        ULinkerArgs = optarg;
                        break;
                }
                break;
                        		   
              case 'w':		/* Link (w)ith a file */
                WithFile = optarg;
                break;
              case 'Y':		/* Use External Preprocessor */
                    /* p preprocessor (use to select external preproc)
                       0 compiler     ('c' also)
                       a assembler
                       l link editor 
                       defaults to 'p' for external preproc 'Cpp'
                     */
                switch (optqual)	{
                    case 'p':
                        PreProc = optarg;
                        break;
                    case '0':
                    case 'c':
                        Compiler = optarg;
                        break;
                    case 'a':
                        Assembler = optarg;
                        break;
                    case 'l':
                        Linker = optarg;
                        break;
                    default:
                        PreProc = optarg;
                        if (strcmp(PreProc, "") == 0)
                            PreProc = DEF_EXT_PREPROC_NAME;
                        break;
                }
                break;
            }
        }

/* Advance argc/argv to the argument that caused getopt() to bail out.
 * Reset getopt()'s optind so we can resume after gathering this operand.
 */
        argc -= optind;
        argv += optind;
        optind = 1;

        if (argc) {
            AddOperandToList(*argv);
        }

    } while (argc > 0);

    return();
}

/*
 *	For each operand, do compilation or assembly as necessary, to
 *	reduce to an object file in the current directory.
 */

void MakeObjects ()
{
    register struct Operand *op;
    auto char buffer[ARGSIZE];
    int print;

    if (Operands.lh_Head->ln_Succ == NULL) {
        if (WithFile == NULL)
            Fatal("No files given!");
        else
            return;
        }

    op = (struct Operand *) Operands.lh_Head;

    print = ((Operands.lh_TailPred != Operands.lh_Head) 
             && (CFILE (op) || SFILE (op)));

    while (op->Node.ln_Succ && !ErrCount) {
	CHECK_ABORT;

	if (print)
	    printf ("%s.%s:\n", op -> Rootname, op -> Suffix);

	if (CFILE (op)) {

	    /* Preprocess (if explicitly requested): */
	    if (xternCpp) {
	        GlobalStatus = Preprocess (op);
                if (ErrCount)
 		    return;
            }

	    /* Compile: */
	    GlobalStatus = Compile (op);
            if (ErrCount)
		return;

	    /* Assemble (unless instructed otherwise with the -S option): */
	    if (!Sflag) {
		putchar('\n');
		if ( (GlobalStatus = Assemble (op)) == 0 )
			return;
 	        if (CFILE (op) && !aflag) {
		    sprintf (buffer, "%s%s.s", QuadDev, op -> Basename);
		    if (!DeleteFile (buffer)) {
		       Warning ("can't delete '%s'", buffer);
		       }
                    }
	    }

	}

	else if (SFILE (op)) {
	    if ( (GlobalStatus = Assemble (op)) == 0 )
		return;
	}
    op = (struct Operand *) op->Node.ln_Succ;
    }

    return;
}

/*
 *	Note that commands to cc of the form "-l<name>" get interpreted
 *	to mean use a library called "name.lib" from the library
 *	directory.
 */

void Link ()
{
    register struct Operand *op;
    struct Node *lib;
    register char *name;
    char buffer[ARGSIZE];
    
    InitCommand ();

    AddToCommand ("%s ", Linker);

/*	Insert any user specified linker arguments:
 */
    if (ULinkerArgs)
	AddToCommand(" %s ", ULinkerArgs);

    if (WithFile)
	AddToCommand("with %s", WithFile);
    else {
        if (StartupFile)
            AddToCommand ("%s", Locate (StartupFile, &LibDirs));
	for (op = (struct Operand *) Operands.lh_Head;
	     op->Node.ln_Succ;
             op = (struct Operand *) op->Node.ln_Succ)  {
	    if (OFILE (op)) {
	        name = op -> Rootname;
	    } else {
	        name = op -> Basename;
	    }
	    AddToCommand ("+ %s.o", name);
        }

/*  Add list of libraries:
 */
        AddToCommand ("%s", " library ");
	for (lib = Libs.lh_Head; lib->ln_Succ; lib = lib->ln_Succ) {
            AddToCommand("%s", "+ ");
            if (!rindex(lib->ln_Name, '.'))
                sprintf (buffer, "%s.lib", lib->ln_Name);
            else
                strcpy(buffer, lib->ln_Name);
	    AddToCommand ("%s", Locate (buffer, &LibDirs));
	}

/* If no -o option, use the first file in the list.
 */
	if ( OutFile == NULL )
	    OutFile = ((struct Operand *)Operands.lh_Head)->Basename;

        AddToCommand (" to %s map ", OutFile);

	AddToCommand (mflag ? "%s.map" : "nil:", 
                      ((struct Operand *)Operands.lh_Head)->Basename );
    }

    (void) RunCommand ();
}
 

/*VARARGS1*/
void Warning (fmt, arg1, arg2, arg3)
char *fmt;
char *arg1;
char *arg2;
char *arg3;
{
    fprintf (stderr, "%s -- warning: ", my_name);
    fprintf (stderr, fmt, arg1, arg2, arg3);
    fprintf (stderr, "\n");
    (void) fflush (stderr);
}

/*VARARGS1*/
void Fatal (fmt, arg1, arg2, arg3)
char *fmt;
char *arg1;
char *arg2;
char *arg3;
{
    fprintf (stderr, "%s -- fatal error: ", my_name);
    fprintf (stderr, fmt, arg1, arg2, arg3);
    fprintf (stderr, "\n");
    fflush (stderr);
    exit (1);
}

/*
 *	Split an operand name into rootname, basename, and suffix
 *	components.  The rootname is the full name, minus any suffix,
 *	but including any prefix.  The basename is the rootname minus
 *	any prefix.  The suffix is anything after the last '.' character.
 *	Only the suffix is allowed to be the null string.
 */

void AddOperandToList (filename)
char *filename;
{
    register char *split;
    struct Operand *op;

    op = (struct Operand *) malloc(sizeof(struct Operand));
    if (!op)
        Fatal(NoMem);

    op -> Rootname = filename;
    if ((split = strrchr (filename, '/')) == NULL) {
	split = strrchr (filename, ':');
    }
    if (split == NULL) {
	op -> Basename = filename;
    } else {
	op -> Basename = ++split;
    }
    if ((split = strrchr (filename, '.')) == NULL) {
	op -> Suffix = "";
    } else {
	*split++ = EOS;
	op -> Suffix = split;
    }

    NumOps++;
    AddTail(&Operands, op);

    return();
}

/*	Compile one operand from a C source program to an object module.
 *      (Very PDC dependent)
 */
 
int Compile (op)
struct Operand *op;
{
    int index,
	status;
    auto char buffer[ARGSIZE];
    struct Node *ln;

    InitCommand ();

    AddToCommand ("%s", Compiler);

/*	Honor any request for annotated assembly output:
 */
    if (Aflag)
	AddToCommand(" -A");

/*	Insert any user specified compiler arguments:
 */
    if (UCompilerArgs)
	AddToCommand(" %s ", UCompilerArgs);

/*	Insert any defines to go to the compiler:
 */
    if (!xternCpp) 
	for (ln = Defines.lh_Head; ln->ln_Succ; ln = ln->ln_Succ)
	    AddToCommand (" -D%s", ln->ln_Name);

/*	Add in the valid search paths for include files:
 */
    for (ln = Includes.lh_Head; ln->ln_Succ; ln = ln->ln_Succ)
	    AddToCommand (" -I %s", ln->ln_Name);

/*	-o is interpreted by the compiler rather than linker when given along
 *	with -S.  -o is ambiguous and ignored here if there is more than
 *	one file being operated on.
 */
    if (Sflag && (OutFile != NULL) && (NumOps == 1))
        AddToCommand (" -o%s ", OutFile);
    else
        AddToCommand (" -o %s%s.s", (aflag ? "" : QuadDev), op->Basename);

    AddToCommand (" %s.c", op->Basename);
    status = RunCommand ();

    CHECK_ABORT;
    return(status);
}

int Preprocess (op)
register struct Operand *op;
{
    register int status;
    register int index;
    struct Node *ln;
    
    InitCommand ();
    AddToCommand ("%s", PreProc);

/*	Insert any user specified preprocessor arguments:
 */
    if (UPreProcArgs)
	AddToCommand(" %s ", UPreProcArgs);

    AddToCommand (" %s", predefines);

/*	Add in the valid search paths for include files:
 */
    for (ln = Includes.lh_Head; ln->ln_Succ; ln = ln->ln_Succ)
	    AddToCommand (" -I %s", ln->ln_Name);

    for (ln = Defines.lh_Head; ln->ln_Succ; ln = ln->ln_Succ)
	    AddToCommand (" -D %s", ln->ln_Name);

    for (ln = Undefines.lh_Head; ln->ln_Succ; ln = ln->ln_Succ)	{
	/* AddToCommand (" -U %s", ln->ln_Name); */
	Warning ("-U%s ignored! (unimplemented)", ln->ln_Name);
    }

    AddToCommand (" %s.%s", op -> Rootname, op -> Suffix);
    AddToCommand (" %s_%s.c", QuadDev, op->Basename);
    status = RunCommand ();
}

 
int Assemble (op)
struct Operand *op;
{
    register int status;
    
    InitCommand ();

    AddToCommand ("%s", Assembler);

/*	Insert any user specified assembler arguments:
 */
    if (UAssemblerArgs)
	AddToCommand(" %s ", UAssemblerArgs);

/*      If debug (-g) option was selected, implement it by having A68k
 *      put out extra symbol information using its (-d) option.
 */
    if (gflag)
        AddToCommand(" -d!L ");

/*	-o is interpreted by assembler rather than linker when given along
 *	with -c.  -o is ambiguous and ignored here if there is more than
 *	one file being operated on.
 */
    if (cflag && (OutFile != NULL) && (NumOps == 1))
        AddToCommand (" -o%s ", OutFile);
    else
        AddToCommand (" -o%s.o ", op->Basename);

    if (CFILE (op))
        AddToCommand ("%s%s.s", (aflag ? "" : QuadDev), op->Basename);
    else
        AddToCommand ("%s.%s", op -> Rootname, op -> Suffix);

    status = RunCommand ();    
    return(status);
}

 
/* RunCommand()
 *
 * Returns  0 if there was an error
 *          1 if everything went A-OK
 */

int RunCommand ()
{
    int success;
    
    CHECK_ABORT;

    if (Vflag) {
	printf("%s\n", Command);
	}

#ifndef pdc
    success = Execute(Command, 0L, 0L);
    if (!success)
	ErrCount++;
#else
    success = system(Command);
    ErrCount += success;
#endif

    if (Vflag)
        printf("%s: Last command returned error code %d\n",
         my_name, success);

    return(!ErrCount);
}

/*
 *	Look through the list of paths pointed to by "vec" until we find
 *	a file with name given pointed to by "namep".  If none is found,
 *	the name pointed to by namep is returned.
 */
 
char *Locate (namep, list)
char *namep;
struct List *list;
{
    static char namebuf[ARGSIZE];
    struct Node *ln;

    for (ln = list->lh_Head; ln->ln_Succ; ln = ln->ln_Succ)    {
	sprintf (namebuf, "%s%s%s",
            ln->ln_Name, 
            (strrchr(ln->ln_Name, ':')==(ln->ln_Name+strlen(ln->ln_Name)) ? 
                "" : "/"),
            namep);
	if (Readable (namebuf)) {
	    namep = namebuf;
	    break;
	}
    }
    return(namep);
}

/*
 *	Check to see if the file or directory exists and is readable.
 */

int Readable (name)
char *name;
{
    register int status = 0;
    register int filedes;
    
#ifndef ACCESS_READ
#define ACCESS_READ -2L
#endif

    filedes = Lock(name, ACCESS_READ);
    if (filedes)	{
	UnLock(filedes);
	status = 1;
    }
    return(status);
}

/*
 *	Do explicit check for abort.  When Enable_Abort is non-zero,
 *	Chk_Abort() cause program termination if CNTRL-C or CNTRL-D has
 *	been received.  Thus, we temporarily set it back to zero while we
 *	do the explicit test, so we can do our own clean up and exit.
 *	Note that if the -V flag was used, we spit out a confirming message
 *	that we are quitting.
 */
 
#ifdef amiga
void _abort()
{
    if (Vflag) {
        printf ("%s: Terminated by request\n", my_name);
    }
    else    {
        printf("\n** BREAK\n");
    }
    exit (1);
}
#endif

/*
 *	Initialize the command line buffer and associated variables to
 *	discard any previous command line.
 */

void InitCommand ()
{
    Command[0] = EOS;
    EndCommand = &Command[0];
}

/*
 *	Build string to add to end of current command line, checking
 *	for overflow in the command buffer and maintaining the pointer
 *	to the end of the current command.
 *
 *	Note that we are a "printf type" of command, and can be called
 *	with up to three "char *" arguments.  There is a portability
 *	problem here, but Lattice hasn't yet made "varargs" a standard
 *	part of their distribution.
 *
 *	Also, note that the return argument of sprintf is supposed to be
 *	the number of characters to be added to the buffer.  This is
 *	not always true for some C implementations.  In particular,
 *	sprintf in BSD4.1 returns a pointer.  Thus we don't use the
 *	return argument.
 *
 */

/*VARARGS1*/
void AddToCommand (fmt, arg1, arg2, arg3)
char *fmt;
char *arg1, *arg2, *arg3;
{
    register int length;
    register char *s;
    auto char buffer[ARGSIZE];
    auto char word[64];

    (void) sprintf (buffer, fmt, arg1, arg2, arg3);
    length = strlen (buffer);
    if ((EndCommand - Command) + length >= sizeof (Command))
	Fatal ("Command line too long- %d char (%d char max)", 
		length, sizeof (Command));
    else {
	(void) strcat (EndCommand, buffer);
	EndCommand += length;
	}
}


Usage(HelpMask)
register int HelpMask;
{
    register struct ConditionalString *ptr;

    printf( "CCX 1.1 - Front end to PDC release 3.3 by Lionel Hummel\n");
    printf( "Usage: %s -[%s] file [file ...]\n\n", my_name, OptChars);

    for (ptr = HelpStrings; ptr->description; ptr++)
        if (ptr->mask & HelpMask)
            fputs(ptr->description, stderr);
}


/* Generates a parameter list either from an environment variable (if
 * available) or else from a specified "default" list of values.
 */
GenList(list, env, def) 
struct List *list;       /* List structure to fill in */
char *env;               /* Name of environment variable to use */
char *def[];             /* Set of default values */ 
{
    char *EnvList;
    char *str;
    char **sptr;
    struct Node *NewNode;

    EnvList = getenv(env);
    if (EnvList == NULL || strcmp(EnvList, "") != 0L)    {
        str = ParseEnv(EnvList);
        while (str != NULL)    {
            NewNode = (struct Node *) malloc(sizeof(struct Node));
            if (NewNode)	{
                NewNode->ln_Name = malloc(strlen(str));
                if (NewNode->ln_Name)    {
                    strcpy(NewNode->ln_Name, str);
                    AddTail(list, NewNode);
                }
                else
                    Fatal(NoMem);
            }
            else
                Fatal(NoMem);
            str = ParseEnv(NULL)
        }
    }
    else if (def == NULL)
        Warning("No list provided (%s).", env);
    else
        for (sptr = &(def[0]); *sptr != 0L; sptr++)	{
            if (Readable(*sptr))	{
                NewNode = (struct Node *) malloc(sizeof(struct Node));
                    if (NewNode)	{
                        NewNode->ln_Name = *sptr;
	                AddTail(list, NewNode);
                    }
                    else
                        Fatal(NoMem);
            }
        }
}
