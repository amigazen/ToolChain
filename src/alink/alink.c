/*
 * alink - Amiga DOS object linker.
 * C89, standard library only.
 * Options follow slink (SAS/C) style; see ALINK_VS_SLINK.md.
 *
 * Usage: alink to <output> [options] from <objects...> library <libs...>
 *    or: alink -o <output> [options] <object files|libs|@file...>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "alink.h"

static char* verstag = "$VER: alink 1.1 (20/2/2026)";
static char* stack_cookie = "8192";

static int is_slink_keyword(const char *s)
{
    if (!s || !*s)
        return 0;
    if (strcmp(s, "to") == 0 || strcmp(s, "from") == 0 || strcmp(s, "root") == 0)
        return 1;
    if (strcmp(s, "library") == 0 || strcmp(s, "lib") == 0 || strcmp(s, "with") == 0)
        return 1;
    if (strcmp(s, "ver") == 0 || strcmp(s, "ovlymgr") == 0 || strcmp(s, "xref") == 0)
        return 1;
    if (strcmp(s, "bufsize") == 0 || strcmp(s, "maxhunk") == 0)
        return 1;
    if (strcmp(s, "smallcode") == 0 || strcmp(s, "sc") == 0)
        return 1;
    if (strcmp(s, "smalldata") == 0 || strcmp(s, "sd") == 0)
        return 1;
    if (strcmp(s, "onedata") == 0 || strcmp(s, "od") == 0)
        return 1;
    if (strcmp(s, "chip") == 0 || strcmp(s, "fast") == 0 || strcmp(s, "faster") == 0)
        return 1;
    if (strcmp(s, "stripdebug") == 0 || strcmp(s, "nodebug") == 0 || strcmp(s, "nd") == 0)
        return 1;
    if (strcmp(s, "noicons") == 0 || strcmp(s, "quiet") == 0 || strcmp(s, "verbose") == 0)
        return 1;
    if (strcmp(s, "libfd") == 0 || strcmp(s, "libprefix") == 0 || strcmp(s, "libid") == 0)
        return 1;
    if (strcmp(s, "prelink") == 0 || strcmp(s, "addsym") == 0 || strcmp(s, "noalvs") == 0)
        return 1;
    if (strcmp(s, "define") == 0 || strcmp(s, "batch") == 0 || strcmp(s, "map") == 0)
        return 1;
    if (strcmp(s, "plain") == 0 || strcmp(s, "fancy") == 0)
        return 1;
    if (strcmp(s, "hwidth") == 0 || strcmp(s, "height") == 0 || strcmp(s, "width") == 0)
        return 1;
    if (strcmp(s, "fwidth") == 0 || strcmp(s, "pwidth") == 0 || strcmp(s, "swidth") == 0)
        return 1;
    if (strcmp(s, "fndatam") == 0 || strcmp(s, "newocv") == 0)
        return 1;
    if (strcmp(s, "librevision") == 0 || strcmp(s, "libversion") == 0)
        return 1;
    if (strcmp(s, "overlay") == 0 || strcmp(s, "#") == 0)
        return 1;
    return 0;
}

static void usage(void)
{
    fprintf(stderr, "usage: alink [from|root] <files...> [to <file>] [with <file>] [ver <file>]\n");
    fprintf(stderr, "            [library|lib <files...>] [xref <file>] [bufsize n] [maxhunk n]\n");
    fprintf(stderr, "            [nodebug|nd] [noicons] [smallcode|sc] [smalldata|sd] [onedata|od]\n");
    fprintf(stderr, "            [fast] [chip] [libfd <file>] [libprefix <prefix>] [libid <id>]\n");
    fprintf(stderr, "            [quiet] [prelink] [addsym] [noalvs] [define sym=val] [verbose]\n");
    fprintf(stderr, "            [faster] [batch] [map <file> [H,X,S,F,L,O]] [plain] [fancy]\n");
    fprintf(stderr, "            [hwidth n] [height n] [width n] [fwidth n] [pwidth n] [swidth n]\n");
    fprintf(stderr, "            [fndatam] [newocv] [ovlymgr <file>] [librevision n] [libversion n]\n");
    fprintf(stderr, "  -o <file>   same as to <file>\n");
    fprintf(stderr, "  -e   preserve empty sections   -r   resident   -f   fragmentation\n");
}

int add_linker_xdefs(struct LinkerContext *ctx)
{
    struct LinkerDefine *d;
    char def_buf[256];
    unsigned int name_len_lw;
    size_t len;
    size_t pad;

    add_linker_xdef(ctx, "__DATA_BAS_\0\0", 3, EXT_DEF, 0);
    add_linker_xdef(ctx, "__DATA_LEN_\0\0", 3, EXT_ABS, 0);
    add_linker_xdef(ctx, "__BSS_LEN_\0\0\0", 3, EXT_ABS, 0);
    add_linker_xdef(ctx, "__RESIDENT\0\0\0", 3, EXT_ABS, 0);

    d = ctx->define_list;
    while (d && d->name) {
        len = strlen(d->name);
        if (len < sizeof(def_buf)) {
            memcpy(def_buf, d->name, len + 1);
            name_len_lw = (unsigned int)((len + 4) / 4);
            pad = (size_t)name_len_lw * 4;
            if (pad > len) {
                memset(def_buf + len, 0, pad - len);
            }
            add_linker_xdef(ctx, def_buf, name_len_lw, EXT_ABS, d->value);
        }
        d = d->next;
    }
    return 0;
}

void alink_init(struct LinkerContext *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->hash_tab_size = ALINK_DEF_HASHTAB;
    ctx->hash_tab_mask = (unsigned short)(ctx->hash_tab_size - 1);
    ctx->hunks_per_unit = ALINK_DEF_HUNKS_PER_UNIT;
    ctx->return_code = 1;
    ctx->rem_empty = 1;
    ctx->xdef_hash_tab = (struct XDEF **)alink_alloc_clear(ctx,
        (unsigned long)ctx->hash_tab_size * sizeof(struct XDEF *));
}

void alink_clear(struct LinkerContext *ctx)
{
    struct LinkerDefine *d;
    struct LinkerDefine *next;

    ctx->units = (struct Unit *)0;
    ctx->code_sections = (struct Section *)0;
    ctx->data_sections = (struct Section *)0;
    ctx->bss_sections = (struct Section *)0;
    ctx->xref_list = (struct XREF *)0;
    ctx->xref_list_tail = (struct XREF *)0;
    ctx->objects = (struct ObjectFile *)0;
    d = ctx->define_list;
    while (d) {
        next = d->next;
        if (d->name)
            free(d->name);
        free(d);
        d = next;
    }
    ctx->define_list = (struct LinkerDefine *)0;
    if (ctx->msg_file && ctx->msg_file != stderr) {
        fclose(ctx->msg_file);
        ctx->msg_file = (FILE *)0;
    }
    alink_free_all(ctx);
}

int alink_link(struct LinkerContext *ctx, int argc, char **argv)
{
    int i;
    int ninputs;
    int cap;
    char **inputs;
    char **to_free;
    int n_to_free;
    int to_free_cap;
    FILE *atf;
    char buf[256];
    char *p;
    char *end;
    char **new_inp;
    char *eq;
    char *endp;
    long lval;
    struct LinkerDefine *def;
    int ret;
    char **expanded_argv;
    int expanded_cap;
    int expanded_argc;
    int ei;
    char *start;
    char save;
    char **av;
    int ac;
    char *q;
    char *seg;
    int *overlay_spec_levels;
    char **overlay_spec_paths;
    int overlay_spec_cap;
    int overlay_spec_n;
    int k;
    int lev;
    int par;
    int last_at_level[32];
    int ord_count[32];
    char **ordered_paths;
    int *ordered_nodes;
    int n_ordered;
    int oi;
    int ni;

    overlay_spec_levels = (int *)0;
    overlay_spec_paths = (char **)0;
    overlay_spec_cap = 0;
    overlay_spec_n = 0;

    if (argc < 2) {
        usage();
        return 1;
    }
    /* Expand WITH and @file: tokenize each line by spaces so "FROM lib:c.o" -> two args */
    expanded_cap = 256;
    expanded_argv = (char **)malloc((size_t)expanded_cap * sizeof(char *));
    if (!expanded_argv) {
        fprintf(stderr, "alink: out of memory\n");
        return 1;
    }
    to_free = (char **)malloc(512 * sizeof(char *));
    if (!to_free) {
        free(expanded_argv);
        return 1;
    }
    to_free_cap = 512;
    n_to_free = 0;
    expanded_argc = 0;
    i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "with") == 0) {
            i++;
            if (i >= argc) {
                fprintf(stderr, "alink: with requires an argument\n");
                for (ei = 0; ei < n_to_free; ei++)
                    free(to_free[ei]);
                free(to_free);
                free(expanded_argv);
                return 1;
            }
            atf = fopen(argv[i], "r");
            if (atf) {
                char line2[256];
                char *q;
                char *end2;
                while (fgets(buf, (int)sizeof(buf), atf) && n_to_free < to_free_cap) {
                    p = buf;
                    while (*p == ' ' || *p == '\t')
                        p++;
                    end = p + strlen(p);
                    while (end > p && (end[-1] == '\n' || end[-1] == '\r' || end[-1] == ' ' || end[-1] == '\t'))
                        end--;
                    *end = '\0';
                    /* Line continuation: * at end of line appends next line */
                    while (end > p && end[-1] == '*') {
                        end--;
                        *end = '\0';
                        if (fgets(line2, (int)sizeof(line2), atf)) {
                            q = line2;
                            while (*q == ' ' || *q == '\t')
                                q++;
                            end2 = q + strlen(q);
                            while (end2 > q && (end2[-1] == '\n' || end2[-1] == '\r'))
                                end2--;
                            *end2 = '\0';
                            if ((size_t)(end - buf + 1 + strlen(q) + 1) <= sizeof(buf)) {
                                *end = ' ';
                                strcpy(end + 1, q);
                                end = buf + strlen(buf);
                            } else {
                                break;
                            }
                        } else {
                            break;
                        }
                    }
                    p = buf;
                    while (*p) {
                        while (*p == ' ' || *p == '\t')
                            p++;
                        if (!*p)
                            break;
                        start = p;
                        while (*p && *p != ' ' && *p != '\t')
                            p++;
                        if (p > start) {
                            save = *p;
                            *p = '\0';
                            if (expanded_argc >= expanded_cap) {
                                expanded_cap *= 2;
                                new_inp = (char **)realloc(expanded_argv, (size_t)expanded_cap * sizeof(char *));
                                if (!new_inp) {
                                    for (ei = 0; ei < n_to_free; ei++)
                                        free(to_free[ei]);
                                    free(to_free);
                                    free(expanded_argv);
                                    return 1;
                                }
                                expanded_argv = new_inp;
                            }
                            to_free[n_to_free] = strdup(start);
                            if (to_free[n_to_free]) {
                                expanded_argv[expanded_argc++] = to_free[n_to_free];
                                n_to_free++;
                            }
                            *p = save;
                        }
                    }
                }
                fclose(atf);
            }
            i++;
            continue;
        }
        if (argv[i][0] == '@') {
            atf = fopen(argv[i] + 1, "r");
            if (atf) {
                while (fgets(buf, (int)sizeof(buf), atf) && n_to_free < to_free_cap) {
                    p = buf;
                    while (*p == ' ' || *p == '\t')
                        p++;
                    end = p + strlen(p);
                    while (end > p && (end[-1] == '\n' || end[-1] == '\r' || end[-1] == ' ' || end[-1] == '\t'))
                        end--;
                    *end = '\0';
                    while (*p) {
                        while (*p == ' ' || *p == '\t')
                            p++;
                        if (!*p)
                            break;
                        start = p;
                        while (*p && *p != ' ' && *p != '\t')
                            p++;
                        if (p > start) {
                            save = *p;
                            *p = '\0';
                            if (expanded_argc >= expanded_cap) {
                                expanded_cap *= 2;
                                new_inp = (char **)realloc(expanded_argv, (size_t)expanded_cap * sizeof(char *));
                                if (!new_inp) {
                                    for (ei = 0; ei < n_to_free; ei++)
                                        free(to_free[ei]);
                                    free(to_free);
                                    free(expanded_argv);
                                    return 1;
                                }
                                expanded_argv = new_inp;
                            }
                            to_free[n_to_free] = strdup(start);
                            if (to_free[n_to_free]) {
                                expanded_argv[expanded_argc++] = to_free[n_to_free];
                                n_to_free++;
                            }
                            *p = save;
                        }
                    }
                }
                fclose(atf);
            }
            i++;
            continue;
        }
        if (expanded_argc >= expanded_cap) {
            expanded_cap *= 2;
            new_inp = (char **)realloc(expanded_argv, (size_t)expanded_cap * sizeof(char *));
            if (!new_inp) {
                for (ei = 0; ei < n_to_free; ei++)
                    free(to_free[ei]);
                free(to_free);
                free(expanded_argv);
                return 1;
            }
            expanded_argv = new_inp;
        }
        expanded_argv[expanded_argc++] = argv[i++];
    }
    av = expanded_argv;
    ac = expanded_argc;

    ctx->out_name[0] = '\0';
    ninputs = 0;
    cap = 64;
    inputs = (char **)malloc((size_t)(cap + 1) * sizeof(char *));
    if (!inputs) {
        for (ei = 0; ei < n_to_free; ei++)
            free(to_free[ei]);
        free(to_free);
        free(expanded_argv);
        return 1;
    }

    i = 0;
    while (i < ac) {
        if (strcmp(av[i], "to") == 0) {
            i++;
            if (i >= ac) {
                fprintf(stderr, "alink: to requires an argument\n");
                free(to_free);
                free(inputs);
                free(expanded_argv);
                return 1;
            }
            strncpy(ctx->out_name, av[i], 255);
            ctx->out_name[255] = '\0';
            i++;
            continue;
        }
        if (strcmp(av[i], "from") == 0 || strcmp(av[i], "root") == 0) {
            i++;
            while (i < ac && !is_slink_keyword(av[i]) && av[i][0] != '-') {
                p = av[i];
                if (strchr(p, ',') || strchr(p, '+')) {
                    start = p;
                    while (*start && n_to_free < to_free_cap) {
                        q = start;
                        while (*q && *q != ',' && *q != '+')
                            q++;
                        if (q > start) {
                            seg = (char *)malloc((size_t)(q - start + 1));
                            if (seg) {
                                memcpy(seg, start, (size_t)(q - start));
                                seg[q - start] = '\0';
                                to_free[n_to_free++] = seg;
                                if (ninputs >= cap) {
                                    cap *= 2;
                                    new_inp = (char **)realloc(inputs, (size_t)(cap + 1) * sizeof(char *));
                                    if (!new_inp) {
                                        for (ei = 0; ei < n_to_free; ei++)
                                            free(to_free[ei]);
                                        free(to_free);
                                        free(expanded_argv);
                                        free(inputs);
                                        return 1;
                                    }
                                    inputs = new_inp;
                                }
                                inputs[ninputs++] = seg;
                            }
                        }
                        if (*q)
                            q++;
                        start = q;
                    }
                    i++;
                } else {
                    if (ninputs >= cap) {
                        cap *= 2;
                        new_inp = (char **)realloc(inputs, (size_t)(cap + 1) * sizeof(char *));
                        if (!new_inp) {
                            fprintf(stderr, "alink: out of memory\n");
                            for (ei = 0; ei < n_to_free; ei++)
                                free(to_free[ei]);
                            free(to_free);
                            free(expanded_argv);
                            free(inputs);
                            return 1;
                        }
                        inputs = new_inp;
                    }
                    inputs[ninputs++] = av[i++];
                }
            }
            continue;
        }
        if (strcmp(av[i], "library") == 0 || strcmp(av[i], "lib") == 0) {
            i++;
            while (i < ac && !is_slink_keyword(av[i]) && av[i][0] != '-') {
                p = av[i];
                if (strchr(p, ',') || strchr(p, '+')) {
                    start = p;
                    while (*start && n_to_free < to_free_cap) {
                        q = start;
                        while (*q && *q != ',' && *q != '+')
                            q++;
                        if (q > start) {
                            seg = (char *)malloc((size_t)(q - start + 1));
                            if (seg) {
                                memcpy(seg, start, (size_t)(q - start));
                                seg[q - start] = '\0';
                                to_free[n_to_free++] = seg;
                                if (ninputs >= cap) {
                                    cap *= 2;
                                    new_inp = (char **)realloc(inputs, (size_t)(cap + 1) * sizeof(char *));
                                    if (!new_inp) {
                                        for (ei = 0; ei < n_to_free; ei++)
                                            free(to_free[ei]);
                                        free(to_free);
                                        free(expanded_argv);
                                        free(inputs);
                                        return 1;
                                    }
                                    inputs = new_inp;
                                }
                                inputs[ninputs++] = seg;
                            }
                        }
                        if (*q)
                            q++;
                        start = q;
                    }
                    i++;
                } else {
                    if (ninputs >= cap) {
                        cap *= 2;
                        new_inp = (char **)realloc(inputs, (size_t)(cap + 1) * sizeof(char *));
                        if (!new_inp) {
                            for (ei = 0; ei < n_to_free; ei++)
                                free(to_free[ei]);
                            free(to_free);
                            free(expanded_argv);
                            free(inputs);
                            return 1;
                        }
                        inputs = new_inp;
                    }
                    inputs[ninputs++] = av[i++];
                }
            }
            continue;
        }
        if (strcmp(av[i], "overlay") == 0) {
            ctx->overlay_mode = 1;
            i++;
            overlay_spec_cap = 256;
            overlay_spec_levels = (int *)malloc((size_t)overlay_spec_cap * sizeof(int));
            overlay_spec_paths = (char **)malloc((size_t)overlay_spec_cap * sizeof(char *));
            if (overlay_spec_levels && overlay_spec_paths) {
                overlay_spec_n = 0;
                while (i < ac && strcmp(av[i], "#") != 0 && !is_slink_keyword(av[i])) {
                    p = av[i];
                    lev = 0;
                    while (*p == '*') {
                        lev++;
                        p++;
                    }
                    if (*p && overlay_spec_n < overlay_spec_cap && n_to_free < to_free_cap) {
                        to_free[n_to_free] = strdup(p);
                        if (to_free[n_to_free]) {
                            overlay_spec_paths[overlay_spec_n] = to_free[n_to_free];
                            overlay_spec_levels[overlay_spec_n] = lev;
                            overlay_spec_n++;
                            n_to_free++;
                        }
                    }
                    i++;
                }
                if (i < ac && strcmp(av[i], "#") == 0)
                    i++;
            }
            continue;
        }
        if (strcmp(av[i], "smallcode") == 0 || strcmp(av[i], "sc") == 0) {
            ctx->small_code = 1;
            i++;
            continue;
        }
        if (strcmp(av[i], "smalldata") == 0 || strcmp(av[i], "sd") == 0) {
            ctx->small_data = 1;
            i++;
            continue;
        }
        if (strcmp(av[i], "chip") == 0) {
            ctx->chip_opt = 1;
            ctx->fast_opt = 0;
            i++;
            continue;
        }
        if (strcmp(av[i], "fast") == 0) {
            ctx->fast_opt = 1;
            ctx->chip_opt = 0;
            i++;
            continue;
        }
        if (strcmp(av[i], "faster") == 0) {
            i++;
            continue;
        }
        if (strcmp(av[i], "stripdebug") == 0 || strcmp(av[i], "nodebug") == 0 || strcmp(av[i], "nd") == 0) {
            ctx->no_debug = 1;
            i++;
            continue;
        }
        if (strcmp(av[i], "verbose") == 0) {
            ctx->verbose = 1;
            i++;
            continue;
        }
        if (strcmp(av[i], "quiet") == 0) {
            ctx->quiet = 1;
            i++;
            continue;
        }
        if (strcmp(av[i], "onedata") == 0 || strcmp(av[i], "od") == 0) {
            ctx->small_data = 1;
            i++;
            continue;
        }
        if (strcmp(av[i], "ver") == 0) {
            i++;
            if (i >= ac) {
                fprintf(stderr, "alink: ver requires an argument\n");
                for (i = 0; i < n_to_free; i++)
                    free(to_free[i]);
                free(to_free);
                free(expanded_argv);
                free(inputs);
                return 1;
            }
            ctx->verify_file = av[i];
            i++;
            continue;
        }
        if (strcmp(av[i], "ovlymgr") == 0) {
            i++;
            if (i >= ac) {
                fprintf(stderr, "alink: ovlymgr requires an argument\n");
                for (i = 0; i < n_to_free; i++)
                    free(to_free[i]);
                free(to_free);
                free(expanded_argv);
                free(inputs);
                return 1;
            }
            ctx->ovlymgr_file = av[i];
            i++;
            continue;
        }
        if (strcmp(av[i], "xref") == 0) {
            i++;
            if (i >= ac) {
                fprintf(stderr, "alink: xref requires an argument\n");
                for (i = 0; i < n_to_free; i++)
                    free(to_free[i]);
                free(to_free);
                free(expanded_argv);
                free(inputs);
                return 1;
            }
            ctx->xref_file = av[i];
            i++;
            continue;
        }
        if (strcmp(av[i], "bufsize") == 0) {
            i++;
            if (i >= ac) {
                fprintf(stderr, "alink: bufsize requires an argument\n");
                for (i = 0; i < n_to_free; i++)
                    free(to_free[i]);
                free(to_free);
                free(expanded_argv);
                free(inputs);
                return 1;
            }
            lval = strtol(av[i], &endp, 0);
            if (endp != av[i] && lval >= 0)
                ctx->bufsize_val = (unsigned long)lval;
            i++;
            continue;
        }
        if (strcmp(av[i], "maxhunk") == 0) {
            i++;
            if (i >= ac) {
                fprintf(stderr, "alink: maxhunk requires an argument\n");
                for (i = 0; i < n_to_free; i++)
                    free(to_free[i]);
                free(to_free);
                free(expanded_argv);
                free(inputs);
                return 1;
            }
            lval = strtol(av[i], &endp, 0);
            if (endp != av[i] && lval > 0)
                ctx->maxhunk_val = (unsigned long)lval;
            i++;
            continue;
        }
        if (strcmp(av[i], "noicons") == 0) {
            ctx->noicons = 1;
            i++;
            continue;
        }
        if (strcmp(av[i], "prelink") == 0) {
            ctx->prelink = 1;
            i++;
            continue;
        }
        if (strcmp(av[i], "addsym") == 0) {
            ctx->addsym = 1;
            i++;
            continue;
        }
        if (strcmp(av[i], "noalvs") == 0) {
            ctx->noalvs = 1;
            i++;
            continue;
        }
        if (strcmp(av[i], "batch") == 0) {
            ctx->batch = 1;
            i++;
            continue;
        }
        if (strcmp(av[i], "define") == 0) {
            i++;
            if (i >= ac) {
                fprintf(stderr, "alink: define requires an argument\n");
                for (i = 0; i < n_to_free; i++)
                    free(to_free[i]);
                free(to_free);
                free(inputs);
                return 1;
            }
            eq = strchr(av[i], '=');
            if (eq && eq != av[i]) {
                def = (struct LinkerDefine *)malloc(sizeof(struct LinkerDefine));
                if (def) {
                    *eq = '\0';
                    def->name = strdup(av[i]);
                    def->value = (unsigned long)strtoul(eq + 1, &endp, 0);
                    *eq = '=';
                    def->next = ctx->define_list;
                    ctx->define_list = def;
                }
            }
            i++;
            continue;
        }
        if (strcmp(av[i], "map") == 0) {
            i++;
            if (i >= ac) {
                fprintf(stderr, "alink: map requires an argument\n");
                for (i = 0; i < n_to_free; i++)
                    free(to_free[i]);
                free(to_free);
                free(expanded_argv);
                free(inputs);
                return 1;
            }
            ctx->map_file = av[i];
            ctx->map_opts[0] = '\0';
            i++;
            if (i < ac && av[i][0] && av[i][0] != '-' && !is_slink_keyword(av[i])) {
                if (strlen(av[i]) < sizeof(ctx->map_opts))
                    strcpy(ctx->map_opts, av[i]);
                i++;
            }
            continue;
        }
        if (strcmp(av[i], "plain") == 0) {
            ctx->map_plain = 1;
            ctx->map_fancy = 0;
            i++;
            continue;
        }
        if (strcmp(av[i], "fancy") == 0) {
            ctx->map_fancy = 1;
            ctx->map_plain = 0;
            i++;
            continue;
        }
        if (strcmp(av[i], "hwidth") == 0) {
            i++;
            if (i < ac) {
                lval = strtol(av[i], &endp, 0);
                if (endp != av[i])
                    ctx->map_hwidth = (int)lval;
                i++;
            }
            continue;
        }
        if (strcmp(av[i], "height") == 0) {
            i++;
            if (i < ac) {
                lval = strtol(av[i], &endp, 0);
                if (endp != av[i])
                    ctx->map_height = (int)lval;
                i++;
            }
            continue;
        }
        if (strcmp(av[i], "width") == 0) {
            i++;
            if (i < ac) {
                lval = strtol(av[i], &endp, 0);
                if (endp != av[i])
                    ctx->map_width = (int)lval;
                i++;
            }
            continue;
        }
        if (strcmp(av[i], "fwidth") == 0) {
            i++;
            if (i < ac) {
                lval = strtol(av[i], &endp, 0);
                if (endp != av[i])
                    ctx->map_fwidth = (int)lval;
                i++;
            }
            continue;
        }
        if (strcmp(av[i], "pwidth") == 0) {
            i++;
            if (i < ac) {
                lval = strtol(av[i], &endp, 0);
                if (endp != av[i])
                    ctx->map_pwidth = (int)lval;
                i++;
            }
            continue;
        }
        if (strcmp(av[i], "swidth") == 0) {
            i++;
            if (i < ac) {
                lval = strtol(av[i], &endp, 0);
                if (endp != av[i])
                    ctx->map_swidth = (int)lval;
                i++;
            }
            continue;
        }
        if (strcmp(av[i], "fndatam") == 0) {
            ctx->fndatam = 1;
            i++;
            continue;
        }
        if (strcmp(av[i], "newocv") == 0) {
            ctx->newocv = 1;
            i++;
            continue;
        }
        if (strcmp(av[i], "libfd") == 0) {
            i++;
            if (i < ac) {
                ctx->libfd_file = av[i];
                i++;
            }
            continue;
        }
        if (strcmp(av[i], "libprefix") == 0) {
            i++;
            if (i < ac) {
                ctx->libprefix_str = av[i];
                i++;
            }
            continue;
        }
        if (strcmp(av[i], "libid") == 0) {
            i++;
            if (i < ac) {
                ctx->libid_str = av[i];
                i++;
            }
            continue;
        }
        if (strcmp(av[i], "librevision") == 0) {
            i++;
            if (i < ac) {
                lval = strtol(av[i], &endp, 0);
                if (endp != av[i])
                    ctx->librevision = (int)lval;
                i++;
            }
            continue;
        }
        if (strcmp(av[i], "libversion") == 0) {
            i++;
            if (i < ac) {
                lval = strtol(av[i], &endp, 0);
                if (endp != av[i])
                    ctx->libversion = (int)lval;
                i++;
            }
            continue;
        }
        if (strcmp(av[i], "-o") == 0) {
            i++;
            if (i >= ac) {
                fprintf(stderr, "alink: -o requires an argument\n");
                for (i = 0; i < n_to_free; i++)
                    free(to_free[i]);
                free(to_free);
                free(inputs);
                return 1;
            }
            strncpy(ctx->out_name, av[i], 255);
            ctx->out_name[255] = '\0';
            i++;
            continue;
        }
        if (strcmp(av[i], "-s") == 0) {
            ctx->small_code = 1;
            i++;
            continue;
        }
        if (strcmp(av[i], "-d") == 0) {
            ctx->small_data = 1;
            i++;
            continue;
        }
        if (strcmp(av[i], "-e") == 0) {
            ctx->rem_empty = 0;
            i++;
            continue;
        }
        if (strcmp(av[i], "-c") == 0) {
            ctx->chip_opt = 1;
            ctx->fast_opt = 0;
            i++;
            continue;
        }
        if (strcmp(av[i], "-r") == 0) {
            ctx->res_opt = 1;
            i++;
            continue;
        }
        if (strcmp(av[i], "-f") == 0) {
            ctx->frag_opt = 1;
            i++;
            continue;
        }
        if (strcmp(av[i], "-n") == 0) {
            ctx->no_debug = 1;
            i++;
            continue;
        }
        if (strcmp(av[i], "-v") == 0) {
            ctx->verbose = 1;
            i++;
            continue;
        }
        if (strcmp(av[i], "-q") == 0) {
            ctx->quiet = 1;
            i++;
            continue;
        }
        if (av[i][0] == '@') {
            atf = fopen(av[i] + 1, "r");
            if (atf) {
                while (fgets(buf, (int)sizeof(buf), atf) && ninputs < cap && n_to_free < to_free_cap) {
                    p = buf;
                    while (*p == ' ' || *p == '\t')
                        p++;
                    end = p + strlen(p);
                    while (end > p && (end[-1] == '\n' || end[-1] == '\r' || end[-1] == ' ' || end[-1] == '\t'))
                        end--;
                    *end = '\0';
                    if (*p) {
                        to_free[n_to_free] = strdup(p);
                        if (to_free[n_to_free]) {
                            inputs[ninputs++] = to_free[n_to_free];
                            n_to_free++;
                        }
                    }
                }
                fclose(atf);
            }
            i++;
            continue;
        }
        if (av[i][0] == '-') {
            fprintf(stderr, "alink: unknown option '%s'\n", av[i]);
            for (i = 0; i < n_to_free; i++)
                free(to_free[i]);
            free(to_free);
            free(expanded_argv);
            free(inputs);
            return 1;
        }
        if (ninputs >= cap) {
            cap *= 2;
            new_inp = (char **)realloc(inputs, (size_t)(cap + 1) * sizeof(char *));
            if (!new_inp) {
                fprintf(stderr, "alink: out of memory\n");
                for (i = 0; i < n_to_free; i++)
                    free(to_free[i]);
                free(to_free);
                free(expanded_argv);
                free(inputs);
                return 1;
            }
            inputs = new_inp;
        }
        inputs[ninputs++] = av[i++];
    }

    inputs[ninputs] = (char *)0;

    if (ninputs == 0) {
        fprintf(stderr, "alink: no input files\n");
        free(to_free);
        free(expanded_argv);
        free(inputs);
        return 1;
    }

    if (ctx->verify_file) {
        if (freopen(ctx->verify_file, "w", stderr) == (FILE *)0) {
            fprintf(stderr, "alink: cannot open verify file '%s'\n", ctx->verify_file);
            free(to_free);
            free(expanded_argv);
            free(inputs);
            return 1;
        }
    }

    ret = add_linker_xdefs(ctx);
    if (ret != 0) {
        for (i = 0; i < n_to_free; i++)
            free(to_free[i]);
        free(to_free);
        free(expanded_argv);
        free(inputs);
        return 1;
    }
    if (ctx->overlay_mode && overlay_spec_n > 0 && overlay_spec_levels && overlay_spec_paths) {
        for (k = 0; k < 32; k++) {
            last_at_level[k] = -1;
            ord_count[k] = 0;
        }
        ctx->n_overlay_nodes = overlay_spec_n;
        ctx->overlay_nodes = (struct OverlayNode *)alink_alloc_clear(ctx, (unsigned long)overlay_spec_n * sizeof(struct OverlayNode));
        for (k = 0; k < overlay_spec_n; k++) {
            lev = overlay_spec_levels[k];
            par = (lev > 0) ? last_at_level[lev - 1] : -1;
            ctx->overlay_nodes[k].level = lev;
            ctx->overlay_nodes[k].ordinate = ++ord_count[lev];
            ctx->overlay_nodes[k].parent_index = par;
            ctx->overlay_nodes[k].nfiles = 1;
            ctx->overlay_nodes[k].files = (char **)alink_alloc(ctx, sizeof(char *));
            ctx->overlay_nodes[k].files[0] = overlay_spec_paths[k];
            last_at_level[lev] = k;
        }
        free(overlay_spec_levels);
        free(overlay_spec_paths);
        overlay_spec_levels = (int *)0;
        overlay_spec_paths = (char **)0;
        n_ordered = (ctx->ovlymgr_file && ctx->ovlymgr_file[0]) ? 1 : 0;
        n_ordered += ninputs;
        for (k = 0; k < ctx->n_overlay_nodes; k++)
            n_ordered += ctx->overlay_nodes[k].nfiles;
        ordered_paths = (char **)malloc((size_t)(n_ordered + 1) * sizeof(char *));
        ordered_nodes = (int *)malloc((size_t)n_ordered * sizeof(int));
        if (ordered_paths && ordered_nodes) {
            oi = 0;
            if (ctx->ovlymgr_file && ctx->ovlymgr_file[0]) {
                ordered_paths[oi] = (char *)ctx->ovlymgr_file;
                ordered_nodes[oi] = -1;
                oi++;
            }
            for (ni = 0; ni < ninputs; ni++) {
                ordered_paths[oi] = inputs[ni];
                ordered_nodes[oi] = -1;
                oi++;
            }
            for (k = 0; k < ctx->n_overlay_nodes; k++) {
                for (par = 0; par < ctx->overlay_nodes[k].nfiles; par++) {
                    ordered_paths[oi] = ctx->overlay_nodes[k].files[par];
                    ordered_nodes[oi] = k;
                    oi++;
                }
            }
            ordered_paths[oi] = (char *)0;
            ret = read_units(ctx, ordered_paths, ordered_nodes, n_ordered);
            free(ordered_paths);
            free(ordered_nodes);
        } else {
            ret = read_units(ctx, inputs, (const int *)0, ninputs);
            if (ordered_paths)
                free(ordered_paths);
            if (ordered_nodes)
                free(ordered_nodes);
        }
    } else {
        if (overlay_spec_levels)
            free(overlay_spec_levels);
        if (overlay_spec_paths)
            free(overlay_spec_paths);
        ret = read_units(ctx, inputs, (const int *)0, ninputs);
    }
    for (i = 0; i < n_to_free; i++)
        free(to_free[i]);
    free(to_free);
    free(expanded_argv);
    free(inputs);
    if (ret != 0)
        return 1;

    select_units(ctx);
    kill_unselected(ctx);
    kill_zero_sections(ctx);
    resolve_common(ctx);
    combine_bss_onto_data(ctx);
    prepare_overlay_stubs(ctx);
    set_section_ids(ctx);
    calc_hunk_offsets(ctx);
    set_lnk_xdef(ctx);
    correction(ctx);

    ret = write_map_file(ctx);
    if (ret != 0)
        return 1;
    ret = write_xref_file(ctx);
    if (ret != 0)
        return 1;
    if (ctx->out_name[0]) {
        ret = write_load_file(ctx);
        if (ret != 0)
            return 1;
    }

    ctx->return_code = 0;
    return 0;
}

int main(int argc, char **argv)
{
    struct LinkerContext ctx;
    int ret;

    alink_init(&ctx);
    ret = alink_link(&ctx, argc, argv);
    alink_clear(&ctx);
    return ret;
}
