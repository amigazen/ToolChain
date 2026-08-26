/*
 * compinc/unistd.h - minimal declarations for bootstrapping AC on POSIX hosts.
 *
 * Only used when AC compiles itself with -DAC_HOST_POSIX.  Amiga self-host
 * builds omit this header (Cmain.c guards unistd with AC_HOST_POSIX).
 */

#ifndef AC_COMPINC_UNISTD_H
#define AC_COMPINC_UNISTD_H

#define R_OK 4
#define W_OK 2
#define X_OK 1
#define F_OK 0

int access(const char *path, int mode);

#endif /* AC_COMPINC_UNISTD_H */
