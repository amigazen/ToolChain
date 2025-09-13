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

/* time.c - standard C time functions
 */

#include <libraries/dos.h>
#include <time.h>
#include <math.h>

struct _MonthTable {
    char *name;
    int   days;
    } _MonthTable[12] = {
        { "Jan", 31 }, { "Feb", 28 }, { "Mar", 31 }, { "Apr", 30 },
        { "May", 31 }, { "Jun", 30 }, { "Jul", 31 }, { "Aug", 31 },
        { "Sep", 30 }, { "Oct", 31 }, { "Nov", 30 }, { "Dec", 31 }
        };

char *(_DayTable[7]) = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };

struct DateStamp StartStamp = { 0, 0, 0 };

char *ctime(timptr)
time_t *timptr;
{
    return(asctime(localtime(timptr)));
}

char *asctime(ts)
struct tm *ts;
{
    char *buffer;

    buffer = malloc(64);   /* 64 is ample space to hold a date string */
    if (!buffer)
        return(0L);

    sprintf(buffer, "%s %s %d %d:%02d:%02d %d\n",
           _DayTable[ts->tm_wday], _MonthTable[ts->tm_mon].name,
           ts->tm_mday, ts->tm_hour, ts->tm_min,
           ts->tm_sec, ts->tm_year+1900 );

    return(buffer);
}

time_t time(tptr)
time_t *tptr;
{
    struct DateStamp dos_date;
    long  secs;

    DateStamp(&dos_date);
    secs = dos_date.ds_Days * (60 * 60 * 24);
    secs += dos_date.ds_Minute * 60;
    secs += (dos_date.ds_Tick/CLK_TCK);

    if (tptr)
        *tptr = (time_t) secs;

    return( (time_t) secs);
}

double jday(t)
time_t *t;
{
    /*  Julian date at epoch (12am Jan. 1, 1978) was 2,443,509.5 */
    return ( ((double) (*t)) / (60.0 * 60.0 * 24.0) + 2443509.5);
}

int dayofw(date)
time_t *date;
{
    double dummy;

    return((int) rint(modf((jday(date)+1.0)/7.0, &dummy) * 7.0) % 7);
}

struct tm *localtime(t)
time_t *t;
{
    struct tm *tmval;
    long secs,
         days,
         year;
    int no_leap;
    int month;
    int hour;
    int min;

    tmval = malloc(sizeof(struct tm));
    if (!tmval)
        return(NULL);

    secs = *t;
    tmval->tm_wday = dayofw(t);

    days = secs / (60 * 60 * 24);
    secs %= (60 * 60 * 24);

    year = 1978;
    while (days > 365)    {
        days -= ( year%4 ? 365 : 366 );
        year++;
        }

/* Special case: Enough days to make it through an additional non-leap year.
 */
    if ( (no_leap=(year%4)) && (days==365) )    {
        year++;
        no_leap = (year%4);
        days = 0;
        }

    tmval->tm_year = year - 1900;    /* Now know year and day of year */
    tmval->tm_yday = days;

    if (no_leap)
        _MonthTable[1].days = 28;
    else
        _MonthTable[1].days = 29;

    for (month = 0; month < 12; month++)    {
        if (days < _MonthTable[month].days)
            break;
        days -= _MonthTable[month].days;
        }

    tmval->tm_mon = month;        /* Now know month and day */
    tmval->tm_mday = ++days;

    tmval->tm_hour = secs / (60 * 60);
    secs %= (60 * 60);
    tmval->tm_min = secs / 60;
    tmval->tm_sec = secs % 60;

    tmval->tm_isdst = isdst(tmval);

    return(tmval);
}

clock_t clock()
{
    struct DateStamp CurrStamp;
    clock_t ticks;

    if (!StartStamp.ds_Days && !StartStamp.ds_Minute && !StartStamp.ds_Tick)
        DateStamp(&StartStamp);

    DateStamp(&CurrStamp);

/* Beware of wraparound (e.g., when program has been running 497 days!-) */
    ticks = (CurrStamp.ds_Days - StartStamp.ds_Days) * (86400 * CLK_TCK);
    ticks += (CurrStamp.ds_Minute - StartStamp.ds_Minute) * (60 * CLK_TCK);
    ticks += (CurrStamp.ds_Tick - StartStamp.ds_Tick);

    return(ticks);
}

/* Timezones and Daylight Savings Time can really be a pain.  Perhaps an
 * Amiga port of zoneinfo will show up in a future PDC release!
 */
int isdst(tmptr)
struct tm *tmptr;
{
    return 0;
}

struct tm *gmtime(t)
time_t *t;
{
    return 0;
}

/* mktime() returns -1 for illegal input.  Arguments prior to the epoch are 
 * illegal.
 */
time_t mktime(tmptr)
struct tm *tmptr;
{
    time_t retval = (time_t) -1;
    long year, secs;

    if (tmptr != 0L && (year = tmptr->tm_year+1900) >= 1978) {
        secs = tmptr->tm_sec;
        secs += tmptr->tm_min * 60;
        secs += tmptr->tm_hour * 60 * 60;
        secs += tmptr->tm_yday * 60 * 60 * 24;
        while (year-- > 1978) {
            if (year % 4)
                secs += (60 * 60 * 24 * 365);
            else
                secs += (60 * 60 * 24 * 366);
            }
        retval = (time_t) secs;
        }
}

double difftime(t1, t0)
time_t t1, t0;
{
    return ((double) t1) - (double) t0;
}
