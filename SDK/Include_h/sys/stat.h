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

/*  Normally found in <sys/stat.h> */
#define S_IFMT         0170000        /* Mask for file type */
#define S_IEXEC        0000100        /* Owner Execute/search permission */
#define S_IWRITE       0000200        /* Owner Write permission */
#define S_IREAD        0000400        /* Owner Read permission */
#define S_ISVTX        0001000        /* Save swapped text after use */
#define S_ISGID        0002000        /* Set group id on execution */
#define S_ISUID        0004000        /* Set user id on execution */
#define S_IFIFO        0010000        /* A fifo */
#define S_IFCHR        0020000        /* A character special file */
#define S_IFDIR        0040000        /* A directory file */
#define S_IFBLK        0060000        /* A block special file */
#define S_IFREG        0100000        /* A a regular file */
#define S_IFLNK        0120000        /* A symbolic link (BSD) */

struct stat {
    ushort st_mode;    /* File mode as used by mknod */
    ino_t st_ino;      /* Inode number */
    dev_t st_dev;      /* Major device number of device containing file */
    dev_t st_rdev;     /* Minor device number of device containing file */
    short st_nlink;    /* Number of links */
    ushort st_uid;     /* File owner's user ID number */
    ushort st_gid;     /* File owner's group ID number */
    off_t st_size;     /* File size in bytes */
    time_t st_atime;   /* Timestamp of last access to file's contents */
    time_t st_mtime;   /* Timestamp of last modification of file */
    time_t st_ctime;   /* Timestamp of file creation */
};
