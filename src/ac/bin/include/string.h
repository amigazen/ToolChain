
/*
 * String functions.
 */

char *memcpy(char *, char *, int );
char *memccpy(char *, char *, int , int );
char *strcpy(char *, char *);
char *strncpy(char *, char *, int );
char *strcat(char *, char *);
char *strncat(char *, char *, int );
int memcmp(char *, char *, int );
int strcmp(char *, char *);
int strncmp(char *, char *, int );
char *memchr(char *, int , int );
char *strchr(char *, int );
int strcspn(char *, char *);
char *strpbrk(char *, char *);
char *strrchr(char *, int );
int strspn(char *, char *);
char *strstr(char *, char *);
char *strtok(char *, char *);
char *memset(char *, int , int );
int strlen(char *);

/*
 * V7 and Berklix compatibility.
 */
char *index(char *, int );
char *rindex(char *, int );
int bcopy(char *, char *, int );
int bcmp(char *, char *, int );
int bzero(char *, int );

/*
 * Putting this in here is really silly, but who am I to argue with X3J11?
 */
char *strerror(int );
