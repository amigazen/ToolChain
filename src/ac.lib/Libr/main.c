
#include "dolib.h"

char           *progname;

extern int      optind;
extern int      opterr;
extern FILE    *efopen();
extern char    *malloc();

int             a_flag;
int             d_flag;
int             l_flag;
int             r_flag;
int             v_flag;
int             x_flag;

LL             *objhead, *objtail;

char            libname[80];    /* name of the library file  */
FILE           *libfp;
char            dirname[80];    /* name of the directory file */
FILE           *dirfp;
char            objname[80];    /* name of current object file */
FILE           *objfp;

void
makename(buffer, name, ext)
    char           *buffer, *name, *ext;
{
    while (*name && *name != '.')
        *buffer++ = *name++;
    while (*ext)
        *buffer++ = *ext++;
    *buffer = '\0';
}

list_library()
{
    LD              d_entry;
    int             count;
    int             bytes;

    if ((dirfp = fopen(dirname, "r")) == NULL) {
        printf("%s not found\n", dirname);
        exit(1);
    }
    printf("Directory of library %s\n\n", libname);
    printf("Module Name                    Size   Offset\n");
    count = bytes = 0;
    while (fread((char *) &d_entry, 1, sizeof(LD), dirfp) == sizeof(LD)) {
        printf("%-30.30s %-6d %-6d\n", d_entry.object_filename,
               d_entry.module_size, d_entry.module_offset);
        count++;
        bytes += d_entry.module_size;
    }
    printf("\nLibrary consists of %d entries totaling %d bytes\n",
           count, bytes);
    fclose(dirfp);
}

read_library()
{
    LL             *lp; /* pointer to the load module */
    int             rc;

    objhead = objtail = NULL;

    /*
     * Open the library and directory files
     */
    if ((libfp = fopen(libname, "r")) == NULL)
        return;
    if ((dirfp = fopen(dirname, "r")) == NULL) {
        printf("%s has been corrupted\n", dirname);
        exit(1);
    }

    /*
     * The files are now open, read in the library
     */
    for (;;) {

        /*
         * Allocate a node to hold the object module
         */
        lp = (LL *) malloc(sizeof(LL));
        if (lp == NULL) {
            printf("Not enough memory\n");
            exit(1);
        }

        /*
         * Check for end of file
         */
        rc = fread((char *) &lp->dir_entry, 1, sizeof(LD), dirfp);
        if (rc != sizeof(LD))
            break;

        /*
         * Allocate a buffer to hold the object file image.
         */
        lp->object_module = (char *) malloc(lp->dir_entry.module_size);
        if (lp->object_module == NULL) {
            printf("Not enough memory.\n");
            exit(1);
        }

        /*
         * Read in the object module
         */
        rc = fread(lp->object_module, 1, lp->dir_entry.module_size, libfp);
        if (rc != lp->dir_entry.module_size) {
            printf("Library file is corrupted.\n");
            exit(1);
        }

        /*
         * Add the module to the linked list
         */
        lp->next = NULL;
        if (objhead == NULL)
            objhead = objtail = lp;
        else
            objtail->next = lp;
        while (objtail->next != NULL)
            objtail = objtail->next;
    }

    /*
     * Library has been read in ok.  Close the files and return
     */
    fclose(libfp);
    fclose(dirfp);
    return (FALSE);
}

write_library()
{
    LL             *lp;
    long            file_position;
    long            len, rc;

    /*
     * Open the library
     */
    if ((libfp = fopen(libname, "w")) == NULL) {
        printf("Error opening %s for output\n", libname);
        exit(1);
    }
    if ((dirfp = fopen(dirname, "w")) == NULL) {
        printf("Error opening %s for output\n", dirname);
        exit(1);
    }

    /*
     * Write out the individual modules and directory records.
     */
    file_position = 0;
    for (lp = objhead; lp != NULL; lp = lp->next) {
        len = fwrite(lp->object_module, 1, lp->dir_entry.module_size, libfp);
        if (len != lp->dir_entry.module_size) {
            printf("Error writing to library file\n");
            exit(1);
        }
        lp->dir_entry.module_offset = file_position;
        file_position += len;
        rc = fwrite((char *) &lp->dir_entry, 1, sizeof(LD), dirfp);
        if (rc != sizeof(LD)) {
            printf("Error writing to directory file\n");
            exit(1);
        }
    }

    /*
     * Close the library files
     */
    fclose(libfp);
    fclose(dirfp);
    printf("library is %d bytes\n", file_position);
}

LL             *
search()
{
    LL             *lp;

    for (lp = objhead; lp != NULL; lp = lp->next) {
        if (strcmp(lp->dir_entry.object_filename, objname) == 0)
            return (lp);
    }
    return (NULL);
}

delete_library(name)
    char           *name;
{
    LL             *lp, *lp2;

    makename(objname, name, ".o");
    lp = objhead;
    if (strcmp(lp->dir_entry.object_filename, objname) == 0) {
        free(lp->object_module);
        objhead = objtail = lp->next;
        if (objtail) {
            while (objtail->next != NULL)
                objtail = objtail->next;
        }
        printf("%s deleted\n", objname);
        return (TRUE);
    }
    lp2 = objhead;
    for (lp = objhead; lp != NULL; lp = lp->next) {
        if (strcmp(lp->dir_entry.object_filename, objname) == 0) {
            free(lp->object_module);
            lp2->next = lp->next;
            objtail = objhead;
            if (objtail) {
                while (objtail->next != NULL)
                    objtail = objtail->next;
            }
            printf("%s deleted\n", objname);
            return (TRUE);
        }
        lp2 = lp;
    }
    return (FALSE);
}

add_library(name)
    char           *name;
{
    LL             *lp;
    int             rc, new;

    makename(objname, name, ".o");

    /*
     * Open the object file
     */
    if ((objfp = fopen(objname, "r")) == NULL) {
        printf("%s cannot be added\n", objname);
        exit(1);
    }

    /*
     * See if we are replacing an old copy
     */
    lp = search();
    if (!(new = (lp == NULL))) {
        free(lp->object_module);
        lp->object_module = NULL;
    }
    else {

        /*
         * Allocate a node to hold the file.
         */
        if ((lp = (LL *) malloc(sizeof(LL))) == NULL) {
            printf("Not enough memory.\n");
            exit(1);
        }

        /*
         * Initialize it.
         */
        strcpy(lp->dir_entry.object_filename, objname);
    }
    fseek(objfp, 0L, 2);
    lp->dir_entry.module_size = ftell(objfp);
    fseek(objfp, 0L, 0);

    /*
     * Allocate the buffer to read the object module into.
     */
    lp->object_module = (char *) malloc(lp->dir_entry.module_size);
    if (lp->object_module == NULL) {
        printf("Not enough memory.\n");
        exit(1);
    }

    /*
     * Read in the file.
     */
    rc = fread(lp->object_module, 1, lp->dir_entry.module_size, objfp);
    if (rc != lp->dir_entry.module_size) {
        printf("Library file is corrupted.\n");
        exit(1);
    }

    /*
     * Add module to linked list.
     */
    if (new) {
        lp->next = NULL;
        if (objhead == NULL)
            objhead = objtail = lp;
        else
            objtail->next = lp;
        while (objtail->next != NULL)
            objtail = objtail->next;
    }

    /*
     * Close object file and return.
     */
    fclose(objfp);
    printf("%s added, size = %d\n", name, lp->dir_entry.module_size);
}

usage()
{
    printf("Usage: %s [ -adlrx ] <Library> [ file ... ]\n", progname);
    printf("\t-a:\tAdd files to the library.\n");
    printf("\t-d:\tDelete files from the library.\n");
    printf("\t-l:\tList the library.\n");
    printf("\t-r:\tReplace files in the library.\n");
    printf("\t-x:\tSort modules for one-pass linker.\n");
    exit(1);
}

main(argc, argv)
    int             argc;
    char           *argv[];

{
    FILE           *inp;
    int             c;
    int             i;
    FILE           *fp;

    opterr = 1;
    progname = argv[0];
    a_flag = d_flag = l_flag = r_flag = v_flag = x_flag = FALSE;
    while ((c = getopt(argc, argv, "adlrvx")) != EOF) {
        switch (c) {
        case 'a':
            a_flag = TRUE;
            if (l_flag || d_flag || r_flag)
                usage();
            break;
        case 'd':
            d_flag = TRUE;
            if (r_flag || l_flag || a_flag)
                usage();
            break;
        case 'l':
            l_flag = TRUE;
            if (d_flag || x_flag || r_flag || a_flag)
                usage();
            break;
        case 'r':
            r_flag = TRUE;
            if (l_flag || d_flag || a_flag)
                usage();
            break;
        case 'v':
            v_flag = TRUE;
            break;
        case 'x':
            x_flag = TRUE;
            if (l_flag)
                usage();
            break;
        case '?':
            usage();
            break;
        }
    }

    if (!(a_flag || r_flag || d_flag || l_flag || x_flag))
        usage();

    argc -= optind;
    argv += optind;

    if (l_flag) {       /* directory listing */
        if (argc != 1)
            usage();
        else {
            makename(libname, argv[0], ".lib");
            makename(dirname, argv[0], ".dir");
            list_library();
        }
    }
    else {
        if (argc == 0)
            usage();
        else {
            makename(libname, argv[0], ".lib");
            makename(dirname, argv[0], ".dir");
            read_library();
            for (i = 1; i < argc; i++) {
                if (a_flag) {
                    delete_library(argv[i]);
                    add_library(argv[i]);
                }
                else if (r_flag)
                    add_library(argv[i]);
                else if (d_flag)
                    delete_library(argv[i]);
            }
            if (x_flag) {
                topsort();
                printf("%s cross-referenced.\n", libname);
            }
            write_library();
        }
    }
    exit(0);
}
