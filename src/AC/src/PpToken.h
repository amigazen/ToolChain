/*
 * PpToken.h - heap-based preprocessing tokens for AC's PreProc.
 *
 * Internal to the preprocessor.  GetSym still receives expanded text via
 * prepdefine(); tokens do not escape into the C parser.
 *
 * All nodes and text are allocated with xalloc under global_flag (Amiga
 * arena).  pp_list_free only drops list heads; the arena owns the memory.
 */

#ifndef AC_PPTOKEN_H
#define AC_PPTOKEN_H

#include "host_posix.h"

/* Token kinds for Phase-3 style preprocessing. */
#define PPT_EOF         0
#define PPT_IDENT       1
#define PPT_NUMBER      2
#define PPT_CHAR        3
#define PPT_STRING      4
#define PPT_PUNCT       5
#define PPT_PLACEMARKER 6   /* empty ## operand */

/*
 * Punctuation codes stored in pptoken.punct when kind == PPT_PUNCT.
 * Single-char punct uses the character value itself (e.g. '+' == 43).
 * Multi-char ops use values above 255.
 */
#define PP_HASHHASH     256 /* ## */
#define PP_LAND         257 /* && */
#define PP_LOR          258 /* || */
#define PP_EQ           259 /* == */
#define PP_NE           260 /* != */
#define PP_LE           261 /* <= */
#define PP_GE           262 /* >= */
#define PP_SHL          263 /* << */
#define PP_SHR          264 /* >> */
#define PP_HASH         '#' /* single # (stringize) */

struct pptoken {
    struct pptoken *next;
    int             kind;
    int             punct;  /* PPT_PUNCT / PP_* code */
    char           *text;   /* ident/number/string/char spelling; may be NULL */
};

typedef struct pptoken PPTOKEN;

/*
 * Amiga ac-self must not use sizeof(PPTOKEN): compile-time sizeof can fold
 * to a poisoned (e_sc<<16)|n value.  Layout is four 32-bit fields.
 */
#if defined(AC_HOST_POSIX)
#define SZ_PPTOKEN  ((int) sizeof(struct pptoken))
#else
#define SZ_PPTOKEN  16
#endif

/* ASCII character classes -- no ctype / cclib _type. */
int             pp_is_white(int c);
int             pp_is_idstart(int c);
int             pp_is_idchar(int c);
int             pp_is_digit(int c);

PPTOKEN        *pp_tok_new(int kind, int punct, char *text);
PPTOKEN        *pp_tok_dup(PPTOKEN *t);
PPTOKEN        *pp_list_dup(PPTOKEN *head);
void            pp_list_free(PPTOKEN *head);    /* drop head; arena keeps bytes */
void            pp_list_append(PPTOKEN **head, PPTOKEN **tail, PPTOKEN *t);
int             pp_list_len(PPTOKEN *head);

/*
 * Tokenize a NUL-terminated preprocessing line (or macro body / argument).
 * Skips C comments.  Does not expand macros.
 */
PPTOKEN        *pp_tokenize(char *src);

/*
 * Emit tokens as text into dst (NUL-terminated).  Returns bytes written
 * excluding NUL, or -1 if truncated.  Adjacent tokens get a separating
 * space when needed so re-lexing stays safe.
 */
int             pp_list_to_text(PPTOKEN *head, char *dst, int dstsz);

/* #param -> "escaped spelling" (heap string). */
char           *pp_stringize(PPTOKEN *arg);

/* left##right -> single ident/number token text (heap). */
char           *pp_paste_text(char *left, char *right);

/* True if spelling is the identifier "defined". */
int             pp_is_defined_kw(char *text);

#endif /* AC_PPTOKEN_H */
