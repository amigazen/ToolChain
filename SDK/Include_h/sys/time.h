/*
 * Libraries and headers for PDC release 3.3 (C) 1989 Lionel Hummel.
 * PDC Software Distribution (C) 1989 Lionel Hummel and Paul Petersen.
 * PDC I/O Library (C) 1987 by J.A. Lydiatt.
 *
 * This code is freely redistributable upon the conditions that this 
 * notice remains intact and that modified versions of this file not
 * be included as part of the PDC Software Distribution without the
 * express consent of the copyright holders.  No warrantee of any
 * kind is provided with this code.  For further information, contact:
 *
 *  PDC Software Distribution    Internet:                     BIX:
 *  P.O. Box 4006             or hummel@cs.uiuc.edu            lhummel
 *  Urbana, IL  61801-8801       petersen@uicsrd.csrd.uiuc.edu
 */

/* time.h - standard C time functions and definitions */

#define CLK_TCK 50  
typedef long time_t;
typedef long clock_t;

struct tm {
    int tm_sec;   /* seconds; range 0..59 */
    int tm_min;   /* minutes; range 0..59 */
    int tm_hour;  /* hours since midnight; range 0..23 */
    int tm_mday;  /* day of month; range 1..31 */
    int tm_mon;   /* month; range 0..11 */
    int tm_year;  /* year; with 0==1900 */
    int tm_wday;  /* day of week; range Sun=0..6 */
    int tm_yday;  /* day of year; range 0..365 */
    int tm_isdst; /* nonzero implies daylight savings */
    };

extern clock_t    clock();
extern time_t     time();
extern char      *asctime();
extern char      *ctime();
extern struct tm *gmtime();
extern struct tm *localtime();
extern time_t     mktime();
extern double     difftime();

extern double     jday();
extern int        dayofw();
