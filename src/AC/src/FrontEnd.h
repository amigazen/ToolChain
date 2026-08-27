/*
 * FrontEnd.h - argv[0] front-end mode (ac vs cc).
 * sc / SAS/C ReadArgs is future work (Amiga-only).
 */

#ifndef FRONTEND_H
#define FRONTEND_H

#define FE_AC	0
#define FE_CC	1
/* FE_SC reserved for future SAS/C ReadArgs front-end */

extern int frontend_mode;

void frontend_set_from_argv0(char *argv0);
void fe_add_define(char *arg);
void fe_add_undef(char *arg);
void fe_add_idir(char *arg);
void usage_cc(void);
int parse_cc_args(int argc, char **argv);

#endif
