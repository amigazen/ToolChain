/*
 * PDC I/O Library Copyright (C) 1987 by J.A. Lydiatt.
 * Freely Distributable for non-commercial use.
 */
/*
 *	This module is called from _main().  It will open a window for
 *	standard I/O to use if the ToolType, "WINDOW" is defined.  The
 *	window size and title can be specified in the tool definition.
 *	If you don't ever plan to call this program from workbench, you
 *	may replace this module by a dummy stub - try _wb_parse(){} for
 *	example.
 */

#include <libraries/dosextens.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>
#include <workbench/icon.h>

typedef struct DiskObject DISKOBJ;
typedef struct Process PROCESS;
typedef struct FileHandle HANDLE;
typedef struct WBStartup WBSTART;
typedef struct WBArg WBARG;

extern void *IconBase;
extern void CloseLibrary(), FreeDiskObject();
extern DISKOBJ *GetDiskObject();

_wb_parse(processp, wbMsg)
register PROCESS *processp;
WBSTART *wbMsg;
{
	register char *cp;
	register DISKOBJ *diskobjp;
	register HANDLE	*handlep;
	register ULONG window;
	WBARG *wbMsgp;
	extern void *OpenLibrary();
	extern long Open();

	if ( !(IconBase = OpenLibrary("icon.library", 0L)) )
		return;
	wbMsgp = wbMsg->sm_ArgList;
	if ( !(diskobjp = GetDiskObject(wbMsgp->wa_Name)) )
		goto done;
	if (cp = FindToolType(diskobjp->do_ToolTypes, "WINDOW")) {
		if (window = Open(cp, MODE_OLDFILE)) {
			handlep =  window << 2;
			processp->pr_ConsoleTask = handlep->fh_Type;
			processp->pr_CIS = window;
			processp->pr_COS = Open("*", MODE_OLDFILE);
		}
	}
	FreeDiskObject(diskobjp);
done:
	CloseLibrary(IconBase);
	IconBase = 0L;
}
