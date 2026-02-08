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

    if (argc < 2) {
        usage();
        return 1;
    }
    ctx->out_name[0] = '\0';
    ninputs = 0;
    cap = 64;
    to_free = (char **)0;
    n_to_free = 0;
    to_free_cap = 0;
    inputs = (char **)malloc((size_t)(cap + 1) * sizeof(char *));
    if (!inputs) {
        fprintf(stderr, "alink: out of memory\n");
        return 1;
    }
    to_free = (char **)malloc(512 * sizeof(char *));
    if (!to_free) {
        free(inputs);
        return 1;
    }
    to_free_cap = 512;

    i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "to") == 0) {
            i++;
            if (i >= argc) {
                fprintf(stderr, "alink: to requires an argument\n");
                free(to_free);
                free(inputs);
                return 1;
            }
            strncpy(ctx->out_name, argv[i], 255);
            ctx->out_name[255] = '\0';
            i++;
            continue;
        }
        if (strcmp(argv[i], "from") == 0 || strcmp(argv[i], "root") == 0) {
            i++;
            while (i < argc && !is_slink_keyword(argv[i]) && argv[i][0] != '-') {
                if (ninputs >= cap) {
                    cap *= 2;
                    new_inp = (char **)realloc(inputs, (size_t)(cap + 1) * sizeof(char *));
                    if (!new_inp) {
                        fprintf(stderr, "alink: out of memory\n");
                        for (i = 0; i < n_to_free; i++)
                            free(to_free[i]);
                        free(to_free);
                        free(inputs);
                        return 1;
                    }
                    inputs = new_inp;
                }
                inputs[ninputs++] = argv[i++];
            }
            continue;
        }
        if (strcmp(argv[i], "library") == 0 || strcmp(argv[i], "lib") == 0) {
            i++;
            while (i < argc && !is_slink_keyword(argv[i]) && argv[i][0] != '-') {
                if (ninputs >= cap) {
                    cap *= 2;
                    new_inp = (char **)realloc(inputs, (size_t)(cap + 1) * sizeof(char *));
                    if (!new_inp) {
                        for (i = 0; i < n_to_free; i++)
                            free(to_free[i]);
                        free(to_free);
                        free(inputs);
                        return 1;
                    }
                    inputs = new_inp;
                }
                inputs[ninputs++] = argv[i++];
            }
            continue;
        }
        if (strcmp(argv[i], "with") == 0) {
            i++;
            if (i >= argc) {
                fprintf(stderr, "alink: with requires an argument\n");
                for (i = 0; i < n_to_free; i++)
                    free(to_free[i]);
                free(to_free);
                free(inputs);
                return 1;
            }
            atf = fopen(argv[i], "r");
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
        if (strcmp(argv[i], "smallcode") == 0 || strcmp(argv[i], "sc") == 0) {
            ctx->small_code = 1;
            i++;
            continue;
        }
        if (strcmp(argv[i], "smalldata") == 0 || strcmp(argv[i], "sd") == 0) {
            ctx->small_data = 1;
            i++;
            continue;
        }
        if (strcmp(argv[i], "chip") == 0) {
            ctx->chip_opt = 1;
            ctx->fast_opt = 0;
            i++;
            continue;
        }
        if (strcmp(argv[i], "fast") == 0) {
            ctx->fast_opt = 1;
            ctx->chip_opt = 0;
            i++;
            continue;
        }
        if (strcmp(argv[i], "faster") == 0) {
            i++;
            continue;
        }
        if (strcmp(argv[i], "stripdebug") == 0 || strcmp(argv[i], "nodebug") == 0 || strcmp(argv[i], "nd") == 0) {
            ctx->no_debug = 1;
            i++;
            continue;
        }
        if (strcmp(argv[i], "verbose") == 0) {
            ctx->verbose = 1;
            i++;
            continue;
        }
        if (strcmp(argv[i], "quiet") == 0) {
            ctx->quiet = 1;
            i++;
            continue;
        }
        if (strcmp(argv[i], "onedata") == 0 || strcmp(argv[i], "od") == 0) {
            ctx->small_data = 1;
            i++;
            continue;
        }
        if (strcmp(argv[i], "ver") == 0) {
            i++;
            if (i >= argc) {
                fprintf(stderr, "alink: ver requires an argument\n");
                for (i = 0; i < n_to_free; i++)
                    free(to_free[i]);
                free(to_free);
                free(inputs);
                return 1;
            }
            ctx->verify_file = argv[i];
            i++;
            continue;
        }
        if (strcmp(argv[i], "ovlymgr") == 0) {
            i++;
            if (i >= argc) {
                fprintf(stderr, "alink: ovlymgr requires an argument\n");
                for (i = 0; i < n_to_free; i++)
                    free(to_free[i]);
                free(to_free);
                free(inputs);
                return 1;
            }
            ctx->ovlymgr_file = argv[i];
            i++;
            continue;
        }
        if (strcmp(argv[i], "xref") == 0) {
            i++;
            if (i >= argc) {
                fprintf(stderr, "alink: xref requires an argument\n");
                for (i = 0; i < n_to_free; i++)
                    free(to_free[i]);
                free(to_free);
                free(inputs);
                return 1;
            }
            ctx->xref_file = argv[i];
            i++;
            continue;
        }
        if (strcmp(argv[i], "bufsize") == 0) {
            i++;
            if (i >= argc) {
                fprintf(stderr, "alink: bufsize requires an argument\n");
                for (i = 0; i < n_to_free; i++)
                    free(to_free[i]);
                free(to_free);
                free(inputs);
                return 1;
            }
            lval = strtol(argv[i], &endp, 0);
            if (endp != argv[i] && lval >= 0)
                ctx->bufsize_val = (unsigned long)lval;
            i++;
            continue;
        }
        if (strcmp(argv[i], "maxhunk") == 0) {
            i++;
            if (i >= argc) {
                fprintf(stderr, "alink: maxhunk requires an argument\n");
                for (i = 0; i < n_to_free; i++)
                    free(to_free[i]);
                free(to_free);
                free(inputs);
                return 1;
            }
            lval = strtol(argv[i], &endp, 0);
            if (endp != argv[i] && lval > 0)
                ctx->maxhunk_val = (unsigned long)lval;
            i++;
            continue;
        }
        if (strcmp(argv[i], "noicons") == 0) {
            ctx->noicons = 1;
            i++;
            continue;
        }
        if (strcmp(argv[i], "prelink") == 0) {
            ctx->prelink = 1;
            i++;
            continue;
        }
        if (strcmp(argv[i], "addsym") == 0) {
            ctx->addsym = 1;
            i++;
            continue;
        }
        if (strcmp(argv[i], "noalvs") == 0) {
            ctx->noalvs = 1;
            i++;
            continue;
        }
        if (strcmp(argv[i], "batch") == 0) {
            ctx->batch = 1;
            i++;
            continue;
        }
        if (strcmp(argv[i], "define") == 0) {
            i++;
            if (i >= argc) {
                fprintf(stderr, "alink: define requires an argument\n");
                for (i = 0; i < n_to_free; i++)
                    free(to_free[i]);
                free(to_free);
                free(inputs);
                return 1;
            }
            eq = strchr(argv[i], '=');
            if (eq && eq != argv[i]) {
                def = (struct LinkerDefine *)malloc(sizeof(struct LinkerDefine));
                if (def) {
                    *eq = '\0';
                    def->name = strdup(argv[i]);
                    def->value = (unsigned long)strtoul(eq + 1, &endp, 0);
                    *eq = '=';
                    def->next = ctx->define_list;
                    ctx->define_list = def;
                }
            }
            i++;
            continue;
        }
        if (strcmp(argv[i], "map") == 0) {
            i++;
            if (i >= argc) {
                fprintf(stderr, "alink: map requires an argument\n");
                for (i = 0; i < n_to_free; i++)
                    free(to_free[i]);
                free(to_free);
                free(inputs);
                return 1;
            }
            ctx->map_file = argv[i];
            ctx->map_opts[0] = '\0';
            i++;
            if (i < argc && argv[i][0] && argv[i][0] != '-' && !is_slink_keyword(argv[i])) {
                if (strlen(argv[i]) < sizeof(ctx->map_opts))
                    strcpy(ctx->map_opts, argv[i]);
                i++;
            }
            continue;
        }
        if (strcmp(argv[i], "plain") == 0) {
            ctx->map_plain = 1;
            ctx->map_fancy = 0;
            i++;
            continue;
        }
        if (strcmp(argv[i], "fancy") == 0) {
            ctx->map_fancy = 1;
            ctx->map_plain = 0;
            i++;
            continue;
        }
        if (strcmp(argv[i], "hwidth") == 0) {
            i++;
            if (i < argc) {
                lval = strtol(argv[i], &endp, 0);
                if (endp != argv[i])
                    ctx->map_hwidth = (int)lval;
                i++;
            }
            continue;
        }
        if (strcmp(argv[i], "height") == 0) {
            i++;
            if (i < argc) {
                lval = strtol(argv[i], &endp, 0);
                if (endp != argv[i])
                    ctx->map_height = (int)lval;
                i++;
            }
            continue;
        }
        if (strcmp(argv[i], "width") == 0) {
            i++;
            if (i < argc) {
                lval = strtol(argv[i], &endp, 0);
                if (endp != argv[i])
                    ctx->map_width = (int)lval;
                i++;
            }
            continue;
        }
        if (strcmp(argv[i], "fwidth") == 0) {
            i++;
            if (i < argc) {
                lval = strtol(argv[i], &endp, 0);
                if (endp != argv[i])
                    ctx->map_fwidth = (int)lval;
                i++;
            }
            continue;
        }
        if (strcmp(argv[i], "pwidth") == 0) {
            i++;
            if (i < argc) {
                lval = strtol(argv[i], &endp, 0);
                if (endp != argv[i])
                    ctx->map_pwidth = (int)lval;
                i++;
            }
            continue;
        }
        if (strcmp(argv[i], "swidth") == 0) {
            i++;
            if (i < argc) {
                lval = strtol(argv[i], &endp, 0);
                if (endp != argv[i])
                    ctx->map_swidth = (int)lval;
                i++;
            }
            continue;
        }
        if (strcmp(argv[i], "fndatam") == 0) {
            ctx->fndatam = 1;
            i++;
            continue;
        }
        if (strcmp(argv[i], "newocv") == 0) {
            ctx->newocv = 1;
            i++;
            continue;
        }
        if (strcmp(argv[i], "libfd") == 0) {
            i++;
            if (i < argc) {
                ctx->libfd_file = argv[i];
                i++;
            }
            continue;
        }
        if (strcmp(argv[i], "libprefix") == 0) {
            i++;
            if (i < argc) {
                ctx->libprefix_str = argv[i];
                i++;
            }
            continue;
        }
        if (strcmp(argv[i], "libid") == 0) {
            i++;
            if (i < argc) {
                ctx->libid_str = argv[i];
                i++;
            }
            continue;
        }
        if (strcmp(argv[i], "librevision") == 0) {
            i++;
            if (i < argc) {
                lval = strtol(argv[i], &endp, 0);
                if (endp != argv[i])
                    ctx->librevision = (int)lval;
                i++;
            }
            continue;
        }
        if (strcmp(argv[i], "libversion") == 0) {
            i++;
            if (i < argc) {
                lval = strtol(argv[i], &endp, 0);
                if (endp != argv[i])
                    ctx->libversion = (int)lval;
                i++;
            }
            continue;
        }
        if (strcmp(argv[i], "-o") == 0) {
            i++;
            if (i >= argc) {
                fprintf(stderr, "alink: -o requires an argument\n");
                for (i = 0; i < n_to_free; i++)
                    free(to_free[i]);
                free(to_free);
                free(inputs);
                return 1;
            }
            strncpy(ctx->out_name, argv[i], 255);
            ctx->out_name[255] = '\0';
            i++;
            continue;
        }
        if (strcmp(argv[i], "-s") == 0) {
            ctx->small_code = 1;
            i++;
            continue;
        }
        if (strcmp(argv[i], "-d") == 0) {
            ctx->small_data = 1;
            i++;
            continue;
        }
        if (strcmp(argv[i], "-e") == 0) {
            ctx->rem_empty = 0;
            i++;
            continue;
        }
        if (strcmp(argv[i], "-c") == 0) {
            ctx->chip_opt = 1;
            ctx->fast_opt = 0;
            i++;
            continue;
        }
        if (strcmp(argv[i], "-r") == 0) {
            ctx->res_opt = 1;
            i++;
            continue;
        }
        if (strcmp(argv[i], "-f") == 0) {
            ctx->frag_opt = 1;
            i++;
            continue;
        }
        if (strcmp(argv[i], "-n") == 0) {
            ctx->no_debug = 1;
            i++;
            continue;
        }
        if (strcmp(argv[i], "-v") == 0) {
            ctx->verbose = 1;
            i++;
            continue;
        }
        if (strcmp(argv[i], "-q") == 0) {
            ctx->quiet = 1;
            i++;
            continue;
        }
        if (argv[i][0] == '@') {
            atf = fopen(argv[i] + 1, "r");
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
        if (argv[i][0] == '-') {
            fprintf(stderr, "alink: unknown option '%s'\n", argv[i]);
            for (i = 0; i < n_to_free; i++)
                free(to_free[i]);
            free(to_free);
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
                free(inputs);
                return 1;
            }
            inputs = new_inp;
        }
        inputs[ninputs++] = argv[i++];
    }

    inputs[ninputs] = (char *)0;

    if (ctx->out_name[0] == '\0') {
        fprintf(stderr, "alink: to <output> or -o <output> is required\n");
        for (i = 0; i < n_to_free; i++)
            free(to_free[i]);
        free(to_free);
        free(inputs);
        return 1;
    }
    if (ninputs == 0) {
        fprintf(stderr, "alink: no input files\n");
        free(to_free);
        free(inputs);
        return 1;
    }

    if (ctx->verify_file) {
        if (freopen(ctx->verify_file, "w", stderr) == (FILE *)0) {
            fprintf(stderr, "alink: cannot open verify file '%s'\n", ctx->verify_file);
            free(to_free);
            free(inputs);
            return 1;
        }
    }

    ret = add_linker_xdefs(ctx);
    if (ret != 0) {
        for (i = 0; i < n_to_free; i++)
            free(to_free[i]);
        free(to_free);
        free(inputs);
        return 1;
    }
    ret = read_units(ctx, inputs, ninputs);
    for (i = 0; i < n_to_free; i++)
        free(to_free[i]);
    free(to_free);
    free(inputs);
    if (ret != 0)
        return 1;

    select_units(ctx);
    kill_unselected(ctx);
    kill_zero_sections(ctx);
    resolve_common(ctx);
    combine_bss_onto_data(ctx);
    set_section_ids(ctx);
    calc_hunk_offsets(ctx);
    set_lnk_xdef(ctx);
    correction(ctx);

    ret = write_load_file(ctx);
    if (ret != 0)
        return 1;

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
