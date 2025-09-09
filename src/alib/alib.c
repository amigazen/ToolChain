/*
 * ALIB Rev. 1.0
 * Amiga Object Module Librarian
 * Programmed by Mike Schwartz
 *               (C)1985, MS Software, all rights reserved.
 *
 * Feel free to distribute this program on a no-charge basis.
 *
 * Use:
 *    ALIB  option  library filelist
 * Where
 *    option may be one of the following:
 *       D  =  delete object modules from library
 *       R  =  replace object modules (or add if not defined)
 *       L  =  directory of library
 *    library is the name of the library file to be created or modified.
 *       if no .lib extension is provided on the command line, it is
 *       appended.  Also note that a file library.dir is created for
 *       each library file and is managed by this program.
 *    filelist is a list of object module filenames separated by spaces
 *       or commas.  Note that if .o is not present in the filenames,
 *       .o will be appended.
 */
#include "stdio.h"
#include "fcntl.h"

/*
 * The following structure defines the records in the .dir file for
 * the library.
 */
#define  LD    struct   libdir_def
struct   libdir_def     {
   char  object_filename[80];       /* name of .o file */
   long  module_offset;             /* offset of module in .lib file */
   long  module_size;               /* size of module in bytes */
   };

/*
 * When the library is openned, each object module is read into memory
 * and managed with the following structure.
 */
#define  LL    struct   liblist_def
struct   liblist_def    {
   struct   libdir_def  dir_entry;  /* directory entry for module */
   char     *object_module;         /* actual text of module */
   LL       *next;                  /* pointer to next in list */
   };

/*
 * external functions
 */
extern   char  *getmem();
extern   long  lseek();

/*
 * Global variables
 */
LL    *objlist=NULL;                /* linked list of object modules */

char  libname[80];                  /* name of library file */
int   libfd;                        /* file descriptor for library file */
char  objname[80];                  /* name of object file */
int   objfd;                        /* file descriptor for object file */
char  dirname[80];                  /* name of directory file */
int   dirfd;                        /* file descriptor for directory file */

char  buf[512];                     /* misc buffer */

char  option;                       /* user selected option */
/*
 * This function prints the help messages and exits.
 */
help() {
   printf("Use:\n");
   printf("   ALIB  option  library filelist \n");
   printf("Where\n");
   printf("   option may be one of the following:\n");
   printf("      D  =  delete object modules from library\n");
   printf("      R  =  replace object modules (or add if not defined) \n");
   printf("      L  =  directory of library\n");
   printf("   library is the name of the library file to be created or modified.\n");
   printf("      if no .lib extension is provided on the command line, it is \n");
   printf("      appended.  Also note that a file library.dir is created for\n");
   printf("      each library file and is managed by this program. \n");
   printf("   filelist is a list of object module filenames separated by spaces\n");
   printf("      or commas.  Note that if .o is not present in the filenames, \n");
   printf("      .o will be appended. \n");
   exit(1);
   }

/*
 * The following function opens the library and directory file.  If the
 * file does not exist, then the user is asked if he wants to create it.
 * If the library exists, then it is read in and built in a linked list.
 */
open_library() {
   LL    *lp;           /* pointer to module node */

   /*
    * Open the library file and directory file.
    */
   libfd = open(libname, O_RDONLY | O_RAW);
   if (libfd == -1) {
      /*
       * Library does not exist, prompt user for creation.
       */
      while (1) {
         printf("%s does not exist, create it? (Y/N): ", libname);
         gets(buf);
         if (buf[0] == 'y' || buf[0] == 'Y')
            return !0;
         if (buf[0] == 'n' || buf[0] == 'N') {
            printf("Alib abandoned\n");
            exit(1);
            }
         }
      }
   dirfd = open(dirname, O_RDONLY | O_RAW);
   if (dirfd == -1) {
      printf("%s has been corrupted\n", dirname);
      printf("Alib abandoned\n");
      exit(1);
      }
   /*
    * library and directory files are open, read in the library.
    */
   while (1) {
      /*
       * Allocate a node to hold the object module.
       */
      lp = (LL *)getmem(sizeof(LL));
      if (lp == NULL) {
         printf("Not enough memory.\nAlib abandoned\n");
         exit(1);
         }
      /*
       * Check for end of file.
       */
      if (read(dirfd, (char *)&lp->dir_entry, sizeof(LD)) != sizeof(LD))
         break;
      /*
       * Allocate a buffer to hold the object file image.
       */
      lp->object_module = getmem(lp->dir_entry.module_size);
      if (lp->object_module == NULL) {
         printf("Not enough memory.\nAlib abandoned\n");
         exit(1);
         }
      /*
       * Read in the object module
       */
      if (read(libfd, lp->object_module, lp->dir_entry.module_size) != 
                 lp->dir_entry.module_size) {
         printf("Library file is corrupted.\nAlib abandoned\n");
         exit(1);
         }
      /*
       * Add module to linked list.
       */
      lp->next = objlist;
      objlist = lp;
      }
   /*
    * Library has been read in ok.  Close files and return.
    */
   close(libfd);
   close(dirfd);
   printf("Library file read in ok\n");
   }

/*
 * The following function removes an object module from the linked list
 * and frees up any memory used by it.
 */
kill_module(name)
char  *name;
{
   LL    *lp;
   LL    *lp2;

   if (objlist == NULL)
      return 0;
   strcpy(objname, name);
   strcat(objname, ".o");
   if (strcmp(objlist->dir_entry.object_filename,  objname) == 0) {
      lp = objlist->next;
      rlsmem(objlist->object_module, objlist->dir_entry.module_size);
      rlsmem((char *)objlist, sizeof(LL));
      objlist = lp;
      return !0;
      }
   for (lp = objlist; lp->next != NULL; lp=lp->next)
      if (strcmp(lp->next->dir_entry.object_filename, objname) == 0) {
         lp2 = lp->next;
         lp->next = lp2->next;
         rlsmem(lp2->object_module, lp2->dir_entry.module_size);
         rlsmem((char *)lp2, sizeof(LL));
         return !0;
         }
   return 0;
   }

/*
 * The following function
 */
add_module(name)
char  *name;
{
   LL    *lp;
   int   len;

   strcpy(objname, name);
   strcat(objname, ".o");
printf("adding %s\n", objname);

   /*
    * Open the object file
    */
   objfd = open(objname, O_RDONLY | O_RAW);
   if (objfd == -1) {
      printf("%s cannot be added\nAlib abandoned\n", objname);
      exit(1);
      }
   /*
    * Allocate a node to hold the file.
    */
   lp = (LL *)getmem(sizeof(LL));
   if (lp == NULL) {
      printf("Not enough memory.\nAlib abandoned\n");
      exit(1);
      }
   /*
    * Initialize it.
    */
   strcpy(lp->dir_entry.object_filename, objname);
   lp->dir_entry.module_size = lseek(objfd, 0, 2);
printf("module size = %d\n", lp->dir_entry.module_size);
   lseek(objfd, 0, 0);
   /*
    * Allocate the buffer to read the object module into.
    */
   lp->object_module = getmem(lp->dir_entry.module_size);
   if (lp->object_module == NULL) {
      printf("Not enough memory.\nAlib abandoned\n");
      exit(1);
      }
   /*
    * Read in the file.
    */
   if (read(objfd, lp->object_module, lp->dir_entry.module_size) != 
             lp->dir_entry.module_size) {
      printf("Library file is corrupted.\nAlib abandoned\n");
      exit(1);
      }
   /*
    * Add module to linked list.
    */
   lp->next = objlist;
   objlist = lp;
   /*
    * Close object file and return.
    */
   close(objfd);
   printf("%s added\n", name);
   }

/*
 * The following routine writes the library from memory and exits.
 */
close_library() {
   LL    *lp;
   long  file_position;
   long  length;

   /*
    * Open the library
    */
   libfd = open(libname, O_CREAT | O_RAW | O_WRONLY);
   if (libfd == -1) {
      printf("Error openning %s for output\nAlib abandoned", libname);
      exit(1);
      }
   dirfd = open(dirname, O_CREAT | O_RAW | O_WRONLY);
   if (dirfd == -1) {
      printf("Error openning %s for output\nAlib abandoned", dirname);
      exit(1);
      }
   /*
    * Write out the individual modules and directory records.
    */
   file_position = 0;
   for (lp = objlist; lp != NULL; lp = lp->next) {
      length = write(libfd, lp->object_module, lp->dir_entry.module_size);
      if (length != lp->dir_entry.module_size) {
         printf("Error writing to library file\nAlib abandoned\n");
         exit(1);
         }
      lp->dir_entry.module_offset = file_position;
      file_position += length;
      if (write(dirfd, (char *)&lp->dir_entry, sizeof(LD)) != sizeof(LD)) {
         printf("Error writing to directory file\nAlib abandoned\n");
         exit(1);
         }
      }
   /*
    * Close the library files
    */
   close(libfd);
   close(dirfd);
   printf("library is %ld bytes\nAlib complete\n", file_position);
   exit(0);
   }


/*
 * Main program.
 */
main(argc, argv)
int   argc;
char  *argv[];
{
   int   count;

   printf("ALIB Rev. 1.0 \n");
   printf("Amiga Object Module Librarian\n");
   printf("Programmed by Mike Schwartz\n");
   printf("(C)1985 MS Software, all rights reserved\n");

   /*
    * check for command line parameters present.
    */
   if (argc < 3)
      help();
   /*
    * setup the option and filenames
    */
   option = argv[1][0];
   strcpy(libname, argv[2]);
   strcpy(dirname, argv[2]);
   strcat(libname, ".lib");
   strcat(dirname, ".dir");
   if (option == 'l' || option == 'L') {
      directory();
      exit(0);
      }
   if (option != 'd' && option != 'D' && option != 'r' && option != 'R')
      help();
   /*
    * Open the library file
    */
   open_library();
   for (count = 3; count < argc; count++) {
      switch(option) {
         case 'd':
         case 'D':
            printf("deleting %s from library\n", argv[count]);
            if (!kill_module(argv[count]))
               printf("%s not defined in library\n", argv[count]);
            else
               printf("deleted.\n");
            break;
         case 'r':
         case 'R':
            kill_module(argv[count]);
            add_module(argv[count]);
            break;
         }
      }
   close_library();
   }

directory() {
   LD    d_entry;
   int   count;
   int   bytes;

   dirfd = open(dirname, O_RDONLY | O_RAW);
   if (dirfd == -1) {
      printf("%s not found\n", dirname);
      exit(1);
      }
   printf("Directory of library %s\n", libname);
   printf("Module Name                    Size   Offset\n");
   count = bytes = 0;
   while (read(dirfd, (char *)&d_entry, sizeof(LD)) == sizeof(LD)) {
      printf("%-30.30s %-6d %-6d\n", d_entry.object_filename, 
               d_entry.module_size, d_entry.module_offset);
      count++;
      bytes += d_entry.module_size;
      }
   printf("Library consists of %d entries totalling %d bytes\n", count,
            bytes);
   close(dirfd);
   }
