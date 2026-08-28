/*
 * PpToken.c - heap preprocessing-token lists for AC PreProc.
 *
 * C89.  Allocations go through xalloc (caller holds global_flag when
 * building durable tokens).  No large stack buffers.
 */

#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>

#include    "C.h"
#include    "PpToken.h"

extern char    *xalloc();
extern char    *litlate();
extern int      global_flag;

int
pp_is_white(c)
    int             c;
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\f' || c == '\v'
        || c == '\n';
}

int
pp_is_idstart(c)
    int             c;
{
    return c == '_' || c == '$'
        || (c >= 'A' && c <= 'Z')
        || (c >= 'a' && c <= 'z');
}

int
pp_is_idchar(c)
    int             c;
{
    return pp_is_idstart(c) || (c >= '0' && c <= '9');
}

int
pp_is_digit(c)
    int             c;
{
    return c >= '0' && c <= '9';
}

static char *
pp_heap_str(s, n)
    char           *s;
    int             n;
{
    char           *p;
    int             i;

    p = (char *) xalloc(n + 1);
    for (i = 0; i < n; i++)
        p[i] = s[i];
    p[n] = '\0';
    return p;
}

static char *
pp_heap_dup(s)
    char           *s;
{
    if (s == NULL)
        return NULL;
    return pp_heap_str(s, (int) strlen(s));
}

PPTOKEN *
pp_tok_new(kind, punct, text)
    int             kind;
    int             punct;
    char           *text;
{
    PPTOKEN        *t;

    t = (PPTOKEN *) xalloc(SZ_PPTOKEN);
    t->next = NULL;
    t->kind = kind;
    t->punct = punct;
    t->text = text;
    return t;
}

PPTOKEN *
pp_tok_dup(t)
    PPTOKEN        *t;
{
    if (t == NULL)
        return NULL;
    return pp_tok_new(t->kind, t->punct, pp_heap_dup(t->text));
}

PPTOKEN *
pp_list_dup(head)
    PPTOKEN        *head;
{
    PPTOKEN        *out;
    PPTOKEN        *tail;
    PPTOKEN        *p;
    PPTOKEN        *n;

    out = NULL;
    tail = NULL;
    for (p = head; p != NULL; p = p->next) {
        n = pp_tok_dup(p);
        pp_list_append(&out, &tail, n);
    }
    return out;
}

void
pp_list_free(head)
    PPTOKEN        *head;
{
    /*
     * Arena allocator: memory lives until release_global.  Clearing the
     * caller's pointer is enough; do not walk-free individual nodes.
     */
    (void) head;
}

void
pp_list_append(head, tail, t)
    PPTOKEN       **head;
    PPTOKEN       **tail;
    PPTOKEN        *t;
{
    if (t == NULL)
        return;
    t->next = NULL;
    if (*head == NULL)
        *head = t;
    else
        (*tail)->next = t;
    *tail = t;
}

int
pp_list_len(head)
    PPTOKEN        *head;
{
    int             n;

    n = 0;
    while (head != NULL) {
        n++;
        head = head->next;
    }
    return n;
}

int
pp_is_defined_kw(text)
    char           *text;
{
    if (text == NULL)
        return 0;
    return text[0] == 'd' && text[1] == 'e' && text[2] == 'f'
        && text[3] == 'i' && text[4] == 'n' && text[5] == 'e'
        && text[6] == 'd' && text[7] == '\0';
}

/*
 * Growable scratch for tokenize -- heap, not stack.
 * Reused across calls; contents copied into per-token xalloc strings.
 */
static char    *pp_lexbuf;
static int      pp_lexbuf_sz;

static void
pp_lexbuf_need(n)
    int             n;
{
    char           *neu;
    int             nsz;
    int             i;

    if (pp_lexbuf != NULL && pp_lexbuf_sz >= n)
        return;
    nsz = n + 64;
    ++global_flag;
    neu = (char *) xalloc(nsz);
    --global_flag;
    if (pp_lexbuf != NULL) {
        for (i = 0; i < pp_lexbuf_sz && i < nsz; i++)
            neu[i] = pp_lexbuf[i];
    }
    pp_lexbuf = neu;
    pp_lexbuf_sz = nsz;
}

static void
pp_lex_pushc(len, c)
    int            *len;
    int             c;
{
    pp_lexbuf_need(*len + 2);
    pp_lexbuf[(*len)++] = (char) c;
}

static unsigned char *
pp_skip_comment(p)
    unsigned char  *p;
{
    if (p[0] == '/' && p[1] == '*') {
        p += 2;
        while (*p != '\0') {
            if (p[0] == '*' && p[1] == '/') {
                p += 2;
                break;
            }
            p++;
        }
        return p;
    }
    if (p[0] == '/' && p[1] == '/') {
        p += 2;
        while (*p != '\0' && *p != '\n')
            p++;
        return p;
    }
    return p;
}

static int
pp_multi_punct(p, punct_out, n_out)
    unsigned char  *p;
    int            *punct_out;
    int            *n_out;
{
    if (p[0] == '#' && p[1] == '#') {
        *punct_out = PP_HASHHASH;
        *n_out = 2;
        return 1;
    }
    if (p[0] == '&' && p[1] == '&') {
        *punct_out = PP_LAND;
        *n_out = 2;
        return 1;
    }
    if (p[0] == '|' && p[1] == '|') {
        *punct_out = PP_LOR;
        *n_out = 2;
        return 1;
    }
    if (p[0] == '=' && p[1] == '=') {
        *punct_out = PP_EQ;
        *n_out = 2;
        return 1;
    }
    if (p[0] == '!' && p[1] == '=') {
        *punct_out = PP_NE;
        *n_out = 2;
        return 1;
    }
    if (p[0] == '<' && p[1] == '=') {
        *punct_out = PP_LE;
        *n_out = 2;
        return 1;
    }
    if (p[0] == '>' && p[1] == '=') {
        *punct_out = PP_GE;
        *n_out = 2;
        return 1;
    }
    if (p[0] == '<' && p[1] == '<') {
        *punct_out = PP_SHL;
        *n_out = 2;
        return 1;
    }
    if (p[0] == '>' && p[1] == '>') {
        *punct_out = PP_SHR;
        *n_out = 2;
        return 1;
    }
    return 0;
}

PPTOKEN *
pp_tokenize(src)
    char           *src;
{
    PPTOKEN        *head;
    PPTOKEN        *tail;
    unsigned char  *p;
    int             len;
    int             punct;
    int             n;
    int             quote;
    char           *text;

    head = NULL;
    tail = NULL;
    if (src == NULL)
        return NULL;

    ++global_flag;
    p = (unsigned char *) src;

    while (*p != '\0') {
        p = pp_skip_comment(p);
        if (*p == '\0')
            break;
        if (pp_is_white((int) *p)) {
            while (pp_is_white((int) *p))
                p++;
            continue;
        }

        if (pp_is_idstart((int) *p)) {
            len = 0;
            while (pp_is_idchar((int) *p)) {
                pp_lex_pushc(&len, (int) *p);
                p++;
            }
            pp_lexbuf[len] = '\0';
            text = pp_heap_str(pp_lexbuf, len);
            pp_list_append(&head, &tail,
                           pp_tok_new(PPT_IDENT, 0, text));
            continue;
        }

        if (pp_is_digit((int) *p)
            || (*p == '.' && pp_is_digit((int) p[1]))) {
            len = 0;
            if (*p == '0' && (p[1] == 'x' || p[1] == 'X')) {
                pp_lex_pushc(&len, (int) *p);
                p++;
                pp_lex_pushc(&len, (int) *p);
                p++;
                while ((*p >= '0' && *p <= '9')
                       || (*p >= 'a' && *p <= 'f')
                       || (*p >= 'A' && *p <= 'F')) {
                    pp_lex_pushc(&len, (int) *p);
                    p++;
                }
            }
            else {
                while (pp_is_digit((int) *p) || *p == '.') {
                    pp_lex_pushc(&len, (int) *p);
                    p++;
                }
                if (*p == 'e' || *p == 'E') {
                    pp_lex_pushc(&len, (int) *p);
                    p++;
                    if (*p == '+' || *p == '-') {
                        pp_lex_pushc(&len, (int) *p);
                        p++;
                    }
                    while (pp_is_digit((int) *p)) {
                        pp_lex_pushc(&len, (int) *p);
                        p++;
                    }
                }
            }
            /* Keep U/L suffixes in text; evaluator strips them. */
            while (*p == 'u' || *p == 'U' || *p == 'l' || *p == 'L') {
                pp_lex_pushc(&len, (int) *p);
                p++;
            }
            pp_lexbuf[len] = '\0';
            text = pp_heap_str(pp_lexbuf, len);
            pp_list_append(&head, &tail,
                           pp_tok_new(PPT_NUMBER, 0, text));
            continue;
        }

        if (*p == '"' || *p == '\'') {
            quote = *p;
            len = 0;
            pp_lex_pushc(&len, (int) *p);
            p++;
            while (*p != '\0' && *p != quote) {
                if (*p == '\\' && p[1] != '\0') {
                    pp_lex_pushc(&len, (int) *p);
                    p++;
                }
                pp_lex_pushc(&len, (int) *p);
                p++;
            }
            if (*p == quote) {
                pp_lex_pushc(&len, (int) *p);
                p++;
            }
            pp_lexbuf[len] = '\0';
            text = pp_heap_str(pp_lexbuf, len);
            pp_list_append(&head, &tail,
                           pp_tok_new(quote == '"' ? PPT_STRING : PPT_CHAR,
                                      0, text));
            continue;
        }

        if (pp_multi_punct(p, &punct, &n)) {
            len = 0;
            while (n-- > 0) {
                pp_lex_pushc(&len, (int) *p);
                p++;
            }
            pp_lexbuf[len] = '\0';
            text = pp_heap_str(pp_lexbuf, len);
            pp_list_append(&head, &tail,
                           pp_tok_new(PPT_PUNCT, punct, text));
            continue;
        }

        /* Single-character punctuator. */
        punct = (int) *p;
        len = 0;
        pp_lex_pushc(&len, punct);
        p++;
        pp_lexbuf[len] = '\0';
        text = pp_heap_str(pp_lexbuf, len);
        pp_list_append(&head, &tail,
                       pp_tok_new(PPT_PUNCT, punct, text));
    }

    --global_flag;
    return head;
}

static int
pp_needs_space(a, b)
    PPTOKEN        *a;
    PPTOKEN        *b;
{
    char            ca;
    char            cb;

    if (a == NULL || b == NULL)
        return 0;
    if (a->kind == PPT_PLACEMARKER || b->kind == PPT_PLACEMARKER)
        return 0;
    if (a->text == NULL || b->text == NULL)
        return 1;
    if (a->text[0] == '\0' || b->text[0] == '\0')
        return 0;
    ca = a->text[strlen(a->text) - 1];
    cb = b->text[0];
    if (pp_is_idchar((int) ca) && pp_is_idchar((int) cb))
        return 1;
    if (ca == '#' && cb == '#')
        return 1;
    return 0;
}

int
pp_list_to_text(head, dst, dstsz)
    PPTOKEN        *head;
    char           *dst;
    int             dstsz;
{
    int             pos;
    int             n;
    PPTOKEN        *prev;
    char           *s;

    if (dst == NULL || dstsz <= 0)
        return -1;
    pos = 0;
    prev = NULL;
    dst[0] = '\0';
    for (; head != NULL; prev = head, head = head->next) {
        if (head->kind == PPT_PLACEMARKER)
            continue;
        s = head->text;
        if (s == NULL)
            continue;
        if (pp_needs_space(prev, head)) {
            if (pos + 1 >= dstsz)
                return -1;
            dst[pos++] = ' ';
        }
        n = (int) strlen(s);
        if (pos + n >= dstsz)
            return -1;
        {
            int             i;

            for (i = 0; i < n; i++)
                dst[pos + i] = s[i];
        }
        pos += n;
    }
    dst[pos] = '\0';
    return pos;
}

char *
pp_stringize(arg)
    PPTOKEN        *arg;
{
    char           *buf;
    char           *p;
    int             need;
    PPTOKEN        *t;
    char           *s;
    int             first;

    need = 3;
    for (t = arg; t != NULL; t = t->next) {
        if (t->kind == PPT_PLACEMARKER || t->text == NULL)
            continue;
        need += (int) strlen(t->text) * 2 + 1;
    }
    buf = (char *) xalloc(need);
    p = buf;
    *p++ = '"';
    first = 1;
    for (t = arg; t != NULL; t = t->next) {
        if (t->kind == PPT_PLACEMARKER || t->text == NULL)
            continue;
        if (!first)
            *p++ = ' ';
        first = 0;
        for (s = t->text; *s; s++) {
            if (*s == '"' || *s == '\\')
                *p++ = '\\';
            *p++ = *s;
        }
    }
    *p++ = '"';
    *p = '\0';
    return buf;
}

char *
pp_paste_text(left, right)
    char           *left;
    char           *right;
{
    int             n;
    char           *r;

    if (left == NULL)
        left = "";
    if (right == NULL)
        right = "";
    n = (int) strlen(left) + (int) strlen(right) + 1;
    r = (char *) xalloc(n);
    strcpy(r, left);
    strcat(r, right);
    return r;
}
