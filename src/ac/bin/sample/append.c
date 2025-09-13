/* append.c
 *
 * this is a quick and dirty little concatentaion program whose only claim
 * to fame is that it APPENDS to its destination rather than truncating it
 * on open.  for an example of how it can be used, see the makefile in the
 * directory :lib/src, where it is used to concatenate object modules when
 * building amiga.lib.
 *
 * the usage of this program is:
 *
 *	append <target> <file> ...
 *
 * the contents of all of the files listed after <target> get appended to
 * the file <target>.  If the file <target> doesn't exist, it is created.
 *
 * written by Lionel Hummel (12 July 89)
 * this code is in the public domain
 */

#include <stdio.h>

#define BUFFER_SIZE	10240
char buffer[BUFFER_SIZE];

extern FILE *fopen();
extern int   fread();
extern int   fwrite();

main(argc, argv)
int argc;
char *argv[];
{
    FILE *output;
    FILE *input;
    int   bytes_read = 0;
    int   bytes_written = 0;

    if (--argc)  {

        output = fopen(*++argv, "a+");
        if (output == (FILE *) NULL) {
            fprintf(stderr, "Open error on %s; aborting\n", *argv);
            exit(1);
        }

        while (--argc) {

            input = fopen(*++argv, "r");
            if (input == (FILE *) NULL) {
                fprintf(stderr, "Open error on %s; aborting\n", *argv);
                exit(1);
            }

            while (!feof(input) && (bytes_written == bytes_read)) {
                bytes_read = fread(buffer, 1, BUFFER_SIZE, input);
                if (bytes_read != 0)
                    bytes_written = fwrite(buffer , 1, bytes_read, output);
                else {
                    fprintf(stderr, "Read error in file %s; aborting\n", *argv);
                    exit(1);
                }
            }

            if (ferror(output)) {
                fprintf(stderr, "Write error on file %s; aborting\n", *argv);
                exit(1);
            }

            fclose(input);
        }

        fclose(output);
    }

    else
        fprintf(stderr, "Usage:\t%s <target> <file1> ...\n", argv[0]);
}
