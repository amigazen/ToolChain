/*
 * crt/ac_crt.c - AC-native CRT for cclib.library (C89)
 *
 * Meets the cclib SetupSTDIO / GetSTDIO / ClearSTDIO contract without
 * SAS/C near-data (LinkerDB / A4), __stdargs, or __saveds.  Intended to
 * compile with AC (and assemble with A68k); link with Blink + amiga.lib.
 *
 * Flow:
 *   open dos + utility + cclib.library v4
 *   optional Workbench message wait
 *   SetupSTDIO(...); main(argc, argv); exit teardown
 *
 * Library LVOs for SetupSTDIO / GetSTDIO / ClearSTDIO and ISO C live in
 * crt/ac_lvos.s.  Exec/dos calls resolve via amiga.lib stack stubs.
 */

/* Opaque FILE - real layout is owned by cclib.library */
typedef void FILE;

struct Library {
	unsigned char pad[20];
	unsigned short lib_Version;
};

struct Message;
struct MsgPort {
	unsigned char pad[34];
};

/* AmigaDOS Process: Task(92) + MsgPort(34) + fields; pr_CLI at offset 172 */
struct Process {
	unsigned char pr_Task[92];
	struct MsgPort pr_MsgPort;
	short pr_Pad;
	long pr_SegList;
	long pr_StackSize;
	void *pr_GlobVec;
	long pr_TaskNum;
	void *pr_StackBase;
	long pr_Result2;
	long pr_CurrentDir;
	long pr_CIS;
	long pr_COS;
	void *pr_ConsoleTask;
	void *pr_FileSystemTask;
	long pr_CLI;
};

struct WBStartup;

/*
 * cclib task_UserData: _argc starts at byte 96 (see libraries/cclib.h).
 * Only the argc/argv face is needed after GetSTDIO().
 */
struct ac_UserData {
	unsigned char skip[96];
	short _argc;
	short _arg_len;
	char **_argv;
};

/* Exec / DOS (amiga.lib stubs; GetArgStr is crt/ac_lvos.s � not in 1.x amiga.lib) */
struct Library *OpenLibrary(char *name, unsigned long version);
void CloseLibrary(struct Library *lib);
struct Process *FindTask(char *name);
void Exit(long code);
char *GetArgStr();
long Output(void);
long Write(long fh, char *buf, long len);
void Forbid(void);
void Permit(void);
void ReplyMsg(struct Message *msg);
void WaitPort(struct MsgPort *port);
struct Message *GetMsg(struct MsgPort *port);

/* cclib LVOs (crt/ac_lvos.s) */
long SetupSTDIO(FILE **stdin_p, FILE **stdout_p, FILE **stderr_p,
	int *errno_p, long *blocksize_p, char **type_p,
	void *mathbase, long alen, char *aptr, void *wbm, void *exitfunc);
struct ac_UserData *GetSTDIO(void);
void ClearSTDIO(void);

extern int main(int argc, char **argv);

void exit();
void _exit();
void abort();
int atexit();
void ac_crt_entry();

/* Call void(void) via pointer � AC cannot parse void (*f)(void). */
void ac_call0();

/* ---- CRT-owned globals (cclib SetupSTDIO wires these) ---- */

struct Library *SysBase = 0;
struct Library *DOSBase = 0;
struct Library *UtilityBase = 0;
struct Library *CCLibBase = 0;
struct Library *MathIeeeDoubBasBase = 0;
struct Library *MathIeeeSingBasBase = 0;
struct WBStartup *WBenchMsg = 0;

FILE *stdin = 0;
FILE *stdout = 0;
FILE *stderr = 0;
int errno = 0;
char *type = 0;
long blocksize = 0;

/*
 * _math: non-zero asks CRT to open IEEE math libraries for soft-float
 * (FD/FS stubs via math.lib) and cclib %f.  Default 0 is crt/ac_math0.c;
 * float programs define short _math = 1 and omit ac_math0 from the link.
 */
extern short _math;

/* Function pointers stored as void * (AC rejects void (*f)(void) syntax). */
void *exit_fcn = 0;

#define AC_ATEXIT_MAX 32
static void *ac_atexit_fn[AC_ATEXIT_MAX];
static long ac_atexit_n;
static long ac_atexit_ran;
static long ac_exiting;
static short ac_math_opened;
static short ac_sing_opened;

static void
ac_dos_msg(msg)
char *msg;
{
	long n;
	long out;

	if (DOSBase == 0 || msg == 0)
		return;
	n = 0;
	while (msg[n] != '\0')
		n++;
	out = Output();
	if (out != 0)
		Write(out, msg, n);
}

static void
ac_run_atexit()
{
	void *f;

	if (ac_atexit_ran)
		return;
	ac_atexit_ran = 1;
	while (ac_atexit_n > 0) {
		ac_atexit_n--;
		f = ac_atexit_fn[ac_atexit_n];
		ac_atexit_fn[ac_atexit_n] = 0;
		if (f != 0)
			ac_call0(f);
	}
}

int
atexit(func)
void *func;
{
	if (func == 0)
		return -1;
	if (ac_atexit_n >= AC_ATEXIT_MAX)
		return -1;
	ac_atexit_fn[ac_atexit_n] = func;
	ac_atexit_n++;
	return 0;
}

static void
ac_terminate(code)
long code;
{
	long rv;

	if (ac_exiting) {
		if (DOSBase != 0)
			Exit(code);
		return;
	}
	ac_exiting = 1;
	rv = code;

	ac_run_atexit();
	if (exit_fcn != 0)
		ac_call0(exit_fcn);

	if (CCLibBase != 0) {
		ClearSTDIO();
		CloseLibrary(CCLibBase);
		CCLibBase = 0;
	}

	if (ac_sing_opened && MathIeeeSingBasBase != 0) {
		CloseLibrary(MathIeeeSingBasBase);
		MathIeeeSingBasBase = 0;
		ac_sing_opened = 0;
	}

	if (ac_math_opened && MathIeeeDoubBasBase != 0) {
		CloseLibrary(MathIeeeDoubBasBase);
		MathIeeeDoubBasBase = 0;
		ac_math_opened = 0;
	}

	if (UtilityBase != 0) {
		CloseLibrary(UtilityBase);
		UtilityBase = 0;
	}

	if (WBenchMsg != 0) {
		Forbid();
		ReplyMsg((struct Message *)WBenchMsg);
		WBenchMsg = 0;
		Permit();
	}

	/* Leave DOSBase open; Exit() requires it.  Process image goes away. */
	if (DOSBase != 0)
		Exit(rv);
}

void
_exit(code)
int code;
{
	ac_terminate((long)code);
}

void
exit(code)
int code;
{
	ac_terminate((long)code);
}

void
abort()
{
	ac_terminate(20L);
}

void
ac_crt_entry()
{
	struct Process *pr;
	struct Library *dos;
	struct Library *util;
	char *aptr;
	long alen;
	struct ac_UserData *ud;
	int rv;

	ac_exiting = 0;
	ac_math_opened = 0;
	ac_sing_opened = 0;
	ac_atexit_n = 0;
	ac_atexit_ran = 0;
	rv = 20;
	aptr = 0;
	alen = 0;

	/* AbsExecBase */
	SysBase = *((struct Library **)4L);

	dos = OpenLibrary("dos.library", 37L);
	if (dos == 0)
		dos = OpenLibrary("dos.library", 0L);
	if (dos == 0)
		return;
	DOSBase = dos;

	/* Early breadcrumb: crash before main leaves no Cmain acdbg lines. */
	ac_dos_msg("acdbg: crt enter\n");

	ac_dos_msg("acdbg: crt utility\n");
	util = OpenLibrary("utility.library", 37L);
	if (util == 0)
		util = OpenLibrary("utility.library", 0L);
	if (util == 0) {
		ac_dos_msg("ac: cannot open utility.library\n");
		CloseLibrary(dos);
		DOSBase = 0;
		return;
	}
	UtilityBase = util;

	ac_dos_msg("acdbg: crt FindTask\n");
	pr = FindTask((char *)0);
	if (pr->pr_CLI == 0L) {
		WaitPort(&pr->pr_MsgPort);
		WBenchMsg = (struct WBStartup *)GetMsg(&pr->pr_MsgPort);
	} else if (DOSBase->lib_Version >= 37) {
		ac_dos_msg("acdbg: crt GetArgStr\n");
		aptr = GetArgStr();
	}

	ac_dos_msg("acdbg: crt cclib\n");
	CCLibBase = OpenLibrary("cclib.library", 4L);
	if (CCLibBase == 0) {
		ac_dos_msg("ac: cannot open cclib.library v4\n");
		goto fail;
	}

	if (aptr != 0) {
		alen = 0;
		while (aptr[alen] != '\0')
			alen++;
	}

	if (_math) {
		/*
		 * Soft-float stubs (.FDadd, .Fd2s, …) need doubbas;
		 * .FSadd / .FSmul / .Fs2l need singbas.  Open both when
		 * the program opts into IEEE math (_math != 0).
		 */
		MathIeeeDoubBasBase = OpenLibrary("mathieeedoubbas.library", 0L);
		if (MathIeeeDoubBasBase == 0) {
			ac_dos_msg("ac: cannot open mathieeedoubbas.library\n");
			goto fail;
		}
		ac_math_opened = 1;

		MathIeeeSingBasBase = OpenLibrary("mathieeesingbas.library", 0L);
		if (MathIeeeSingBasBase == 0) {
			ac_dos_msg("ac: cannot open mathieeesingbas.library\n");
			goto fail;
		}
		ac_sing_opened = 1;
	}

	ac_dos_msg("acdbg: crt SetupSTDIO\n");
	/*
	 * Pass exit as the abort hook so Ctrl-C runs atexit + ClearSTDIO.
	 * SetupSTDIO returns non-zero on success (cclib contract).
	 */
	if (!SetupSTDIO(
		&stdin, &stdout, &stderr,
		&errno, &blocksize, &type,
		(void *)MathIeeeDoubBasBase,
		alen, aptr, (void *)WBenchMsg, (void *)exit)) {
		ac_dos_msg("ac: SetupSTDIO failed\n");
		goto fail;
	}

	ac_dos_msg("acdbg: crt GetSTDIO\n");
	ud = GetSTDIO();
	if (ud != 0) {
		ac_dos_msg("acdbg: crt -> main\n");
		rv = main((int)ud->_argc, ud->_argv);
	} else
		rv = 20;

fail:
	_exit(rv);
}
