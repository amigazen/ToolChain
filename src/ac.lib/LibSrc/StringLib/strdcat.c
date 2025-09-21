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

/*    strdcat.c
 *
 *    strdcat() returns a concatenated copy of two strings
 */

char *strdcat(s1, s2)
char *s1;
char *s2;
{
    char *retval;

    retval = strndup(s1, strlen(s1) + strlen(s2) + 1 );

    if (retval != 0L)  {
        strcat(retval, s2);
        }

    return(retval);
}
