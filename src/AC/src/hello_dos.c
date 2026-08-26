/*
 * hello_dos.c ? ac-self2 + cclib.library smoke test
 *
 * Exercises CRT LVO stubs: printf/sprintf/fprintf tagcalls, fputs/fflush,
 * strlen/strcpy/strcmp, malloc/free, fopen/fwrite/fread/fclose.
 * Does not fclose stdout/stderr (cclib aliases them).
 *
 * Build (does not rebuild ac-self2):
 *   make hello-dos
 *   hello_dos
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main(void)
{
    char buf[64];
    char *heap;
    FILE *fp;
    size_t n;
    int rc;

    rc = 0;

    printf("hello_dos: printf ok %d\n", 1);

    sprintf(buf, "sprintf %s %d", "ok", 2);
    fputs(buf, stdout);
    fputs("\n", stdout);

    fprintf(stdout, "fprintf a=%d\n", 3);
    fprintf(stdout, "fprintf b=%s\n", "ok");
    fflush(stdout);

    if (strlen("abcd") != 4) {
        printf("FAIL strlen\n");
        rc = 1;
    }
    strcpy(buf, "xy");
    if (strcmp(buf, "xy") != 0) {
        printf("FAIL strcpy/strcmp\n");
        rc = 1;
    }

    heap = (char *) malloc(32);
    if (heap == NULL) {
        printf("FAIL malloc\n");
        rc = 1;
    } else {
        strcpy(heap, "heap-ok");
        printf("malloc: %s\n", heap);
        free(heap);
    }

    fp = fopen("RAM:ac_hello.tmp", "w");
    if (fp == NULL) {
        printf("FAIL fopen write\n");
        rc = 1;
    } else {
        n = fwrite("file-ok\n", 1, 8, fp);
        fclose(fp);
        if (n != 8) {
            printf("FAIL fwrite\n");
            rc = 1;
        }
    }

    fp = fopen("RAM:ac_hello.tmp", "r");
    if (fp == NULL) {
        printf("FAIL fopen read\n");
        rc = 1;
    } else {
        n = fread(buf, 1, 8, fp);
        buf[n] = '\0';
        fclose(fp);
        if (n != 8 || strcmp(buf, "file-ok\n") != 0) {
            printf("FAIL fread got '%s'\n", buf);
            rc = 1;
        } else
            printf("fopen/fread: %s", buf);
    }

    if (rc == 0)
        printf("hello_dos: all cclib checks passed\n");
    else
        printf("hello_dos: FAILED rc=%d\n", rc);

    return rc;
}
