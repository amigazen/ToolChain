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


/* defines for dbx(1)	*/

#define	N_GSYM		0x20			/* Global symbol		*/
#define N_FUN		0x24			/* Function name		*/
#define N_STSYM		0x26			/* Static symbol		*/
#define N_RSYM		0x40			/* register symbol		*/
#define N_SLINE		0x44			/* Line number			*/
#define N_SO		0x64			/* Source file			*/
#define N_LSYM		0x80			/* local symbol			*/
#define N_BINCL		0x82			/* Begin of include		*/
#define N_LBRAC		0xc0			/* Left  bracketing tag	*/
#define N_RBRAC		0xe0			/* Right bracketing tag	*/
#define N_PSYM		0xa0			/* Parameter			*/
#define N_EINCL		0xa2			/* End of include		*/

