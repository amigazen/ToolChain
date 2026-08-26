/*
 * compinc/stdlib.h - bootstrap subset of stdlib.h for AC self-host builds.
 *
 * Omits function-pointer parameters (atexit, qsort, bsearch) that the
 * legacy parser still rejects.  Placed ahead of src/include via -Isrc/compinc.
 */

#ifndef AC_COMPINC_STDLIB_H
#define AC_COMPINC_STDLIB_H

#include <stddef.h>

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

void abort(void);
int abs(int j);
double atof(const char *nptr);
int atoi(const char *nptr);
long atol(const char *nptr);
void *calloc(size_t nmemb, size_t size);
void exit(int status);
void free(void *ptr);
char *getenv(const char *name);
void *malloc(size_t size);
int rand(void);
void *realloc(void *ptr, size_t size);
void srand(unsigned int seed);
double strtod(const char *nptr, char **endptr);
long strtol(const char *nptr, char **endptr, int base);
unsigned long strtoul(const char *nptr, char **endptr, int base);
int system(const char *string);

#endif /* AC_COMPINC_STDLIB_H */
