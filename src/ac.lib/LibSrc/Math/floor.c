/*
 * PDC I/O Library Copyright (C) 1987 by J.A. Lydiatt.
 * Modifications for PDC release 3.3 Copyright (C) 1988 Lionel D. Hummel.
 * PDC Software Distribution Copyright (C) 1988 Lionel Hummel and Paul Petersen.
 *
 * This code is freely redistributable upon the conditions that this notice
 * remains intact and that modified versions of this file not be included
 * as part of the PDC Software Distribution without the express consent of
 * the copyright holders.
 */

extern long IEEEDPFix();
extern double IEEEDPFlt();

double floor(dp_num)
double dp_num;
{
    return(IEEEDPFlt(IEEEDPFix(dp_num)));
}
