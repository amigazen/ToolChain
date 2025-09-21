
#include <stdio.h>

#define TRUE  1
#define FALSE 0

/* Hunk number definitions */

#define HunkNone 0      /* Not in a hunk */
#define HunkUnit 999
#define HunkName 1000
#define HunkCode 1001
#define HunkData 1002
#define HunkBSS  1003
#define HunkR32  1004
#define HunkR16  1005
#define HunkR8   1006
#define HunkExt  1007
#define HunkSym  1008
#define HunkDbg  1009
#define HunkEnd  1010

#define MEMF_FAST 0x80000000L   /* Hunk must load in FAST memory */
#define MEMF_CHIP 0x40000000L   /* Hunk must load in CHIP memory */

/* Hunk numbers denoting special symbol attributes */

#define ABSHUNK 32767       /* Absolute */

#define LD  struct libdir_def
#define LL  struct liblist_def
#define SM  struct symbol_entry
#define SP  struct symbol_pointer

struct symbol_entry {
    char           *name;
    LL             *parent;
    SM             *next;
};

struct symbol_pointer {
    SM             *entry;
    SP             *next;
};

/*
 * The following structure defines the records in the .dir file for the
 * library.
 */

struct libdir_def {
    char            object_filename[80];    /* name of .o file               */
    long            module_offset;          /* offset of module in .lib file */
    long            module_size;            /* size of module in bytes       */
};

/*
 * When the library is openned, each object module is read into memory and
 * managed with the following structure.
 */

struct liblist_def {
    LD              dir_entry;      /* directory entry for module   */
    char           *object_module;  /* actual text of module        */
    LL             *next;           /* pointer to next in list      */
    SP             *xdef;           /* pointer to xdef list         */
    SP             *xref;           /* pointer to xref list         */
};
