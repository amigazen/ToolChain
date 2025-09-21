/*
 * Libraries and headers for PDC release 3.3 (C) 1989 Lionel D. Hummel.
 * PDC Software Distribution (C) 1989 Lionel Hummel and Paul Petersen.
 * PDC I/O Library (C) 1987 by J.A. Lydiatt.
 *
 * This code is freely redistributable upon the conditions that this notice
 * remains intact and that modified versions of this file not be included
 * as part of the PDC Software Distribution without the express consent of
 * the copyright holders.
 */

/*
 *    strndup.c
 *
 *    strndup() returns a duplicate string in a buffer n bytes long
 */

char *strndup(string, n)
char *string;
int   n;
{
    char *retval;

    retval = malloc(n);

    if (retval != 0L)
        strncpy(retval, string, n);

    return(retval);
}
