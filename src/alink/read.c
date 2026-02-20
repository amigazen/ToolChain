/*
 * Read Amiga object files and parse units/hunks.
 * C89, standard library only.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "alink.h"

static unsigned int symbol_hash(const unsigned char *name_ptr, unsigned int name_len_lw)
{
    unsigned int i;
    unsigned int j;
    unsigned int h = 0;
    unsigned long word;
    for (i = 0; i < name_len_lw; i++) {
        word = read_be32(name_ptr + (unsigned long)i * 4);
        for (j = 0; j < 4; j++)
            h = h * 3 + (unsigned int)((word >> (24 - j * 8)) & 0xFF);
    }
    return h;
}

static struct Section *get_section(struct LinkerContext *ctx, unsigned long type,
    const char *name)
{
    struct Section **head;
    struct Section *sec;
    struct Section *prev;
    const char *n;

    if (type == HUNK_CODE)
        head = &ctx->code_sections;
    else if (type == HUNK_DATA)
        head = &ctx->data_sections;
    else
        head = &ctx->bss_sections;

    n = (name && name[0]) ? name : "UNNAMED";

    if (ctx->small_code && type == HUNK_CODE && *head) {
        prev = (struct Section *)0;
        for (sec = *head; sec; prev = sec, sec = sec->next)
            ;
        if (prev)
            return prev;
    }
    if (ctx->small_data && (type == HUNK_DATA || type == HUNK_BSS) && *head) {
        prev = (struct Section *)0;
        for (sec = *head; sec; prev = sec, sec = sec->next)
            ;
        if (prev)
            return prev;
    }

    if (!ctx->frag_opt) {
        for (sec = *head; sec; sec = sec->next) {
            if (sec->type == type && strcmp(sec->section_name, n) == 0)
                return sec;
        }
    }

    sec = (struct Section *)alink_alloc_clear(ctx, sizeof(struct Section));
    if (!sec)
        return (struct Section *)0;
    sec->type = type;
    sec->section_id = (unsigned long)ctx->id_cnt++;
    strncpy(sec->section_name, n, ALINK_SECNAMESZ - 1);
    sec->section_name[ALINK_SECNAMESZ - 1] = '\0';
    sec->next = *head;
    *head = sec;
    return sec;
}

static void get_name(struct LinkerContext *ctx, unsigned long name_len_lw,
    unsigned char *name_buf, const unsigned char **ptr_inout,
    const unsigned char *end)
{
    unsigned long len_bytes;
    unsigned long to_copy;
    const unsigned char *p;

    p = *ptr_inout;
    len_bytes = name_len_lw * 4;
    if (len_bytes == 0) {
        strcpy((char *)name_buf, "UNNAMED");
        return;
    }
    to_copy = len_bytes;
    if (to_copy > ALINK_SECNAMESZ - 1)
        to_copy = ALINK_SECNAMESZ - 1;
    if (p + to_copy > end)
        to_copy = (unsigned long)(end - p);
    memcpy(name_buf, p, (size_t)to_copy);
    name_buf[to_copy] = '\0';
    *ptr_inout = p + len_bytes;
}

static int add_xdef(struct LinkerContext *ctx, struct Unit *unit,
    struct Section *sec, unsigned char *xdef_ptr);
static int add_xref(struct LinkerContext *ctx, struct Hunk *hunk,
    unsigned char *xref_ptr, unsigned char ref_type);

/*
 * Parse one hunk's trailing blocks (RELOC8/16/32, EXT, SYMBOL, DEBUG, END).
 * Returns pointer after the hunk, or NULL on error.
 */
static const unsigned char *parse_hunk_blocks(struct LinkerContext *ctx,
    struct Hunk *hunk, const unsigned char *p, const unsigned char *end)
{
    unsigned long block_type;
    unsigned long count;
    unsigned long i;
    unsigned long name_len_lw;
    unsigned char ext_type;
    unsigned long val;
    unsigned long num_offsets;
    unsigned long skip;

    for (;;) {
        if (p + 4 > end)
            return (const unsigned char *)0;
        block_type = read_be32(p);
        p += 4;

        if (block_type == HUNK_END)
            return p;

        if (block_type == HUNK_RELOC32) {
            if (hunk->hunk_reloc32)
                return (const unsigned char *)0;  /* duplicate */
            hunk->hunk_reloc32 = (unsigned char *)p;
            for (;;) {
                if (p + 4 > end)
                    return (const unsigned char *)0;
                count = read_be32(p);
                p += 4;
                if (count == 0)
                    break;
                if (p + 4 + (unsigned long)count * 4 > end)
                    return (const unsigned char *)0;
                p += 4 + count * 4;
            }
            continue;
        }

        if (block_type == HUNK_RELOC16) {
            if (hunk->hunk_reloc16)
                return (const unsigned char *)0;
            hunk->hunk_reloc16 = (unsigned char *)p;
            for (;;) {
                if (p + 4 > end)
                    return (const unsigned char *)0;
                count = read_be32(p);
                p += 4;
                if (count == 0)
                    break;
                if (p + 4 + (unsigned long)count * 4 > end)
                    return (const unsigned char *)0;
                p += 4 + count * 4;
            }
            continue;
        }

        if (block_type == HUNK_RELOC8) {
            if (hunk->hunk_reloc8)
                return (const unsigned char *)0;
            hunk->hunk_reloc8 = (unsigned char *)p;
            for (;;) {
                if (p + 4 > end)
                    return (const unsigned char *)0;
                count = read_be32(p);
                p += 4;
                if (count == 0)
                    break;
                if (p + 4 + (unsigned long)count * 4 > end)
                    return (const unsigned char *)0;
                p += 4 + count * 4;
            }
            continue;
        }

        if (block_type == HUNK_DREL32) {
            if (hunk->hunk_reloc32d)
                return (const unsigned char *)0;
            hunk->hunk_reloc32d = (unsigned char *)p;
            for (;;) {
                if (p + 4 > end)
                    return (const unsigned char *)0;
                count = read_be32(p);
                p += 4;
                if (count == 0)
                    break;
                if (p + 4 + (unsigned long)count * 4 > end)
                    return (const unsigned char *)0;
                p += 4 + count * 4;
            }
            continue;
        }
        if (block_type == HUNK_DREL16) {
            if (hunk->hunk_reloc16d)
                return (const unsigned char *)0;
            hunk->hunk_reloc16d = (unsigned char *)p;
            for (;;) {
                if (p + 4 > end)
                    return (const unsigned char *)0;
                count = read_be32(p);
                p += 4;
                if (count == 0)
                    break;
                if (p + 4 + (unsigned long)count * 4 > end)
                    return (const unsigned char *)0;
                p += 4 + count * 4;
            }
            continue;
        }
        if (block_type == HUNK_DREL8) {
            if (hunk->hunk_reloc8d)
                return (const unsigned char *)0;
            hunk->hunk_reloc8d = (unsigned char *)p;
            for (;;) {
                if (p + 4 > end)
                    return (const unsigned char *)0;
                count = read_be32(p);
                p += 4;
                if (count == 0)
                    break;
                if (p + 4 + (unsigned long)count * 4 > end)
                    return (const unsigned char *)0;
                p += 4 + count * 4;
            }
            continue;
        }

        if (block_type == HUNK_EXT) {
            if (hunk->hunk_extern)
                return (const unsigned char *)0;
            hunk->hunk_extern = (unsigned char *)p;
            for (;;) {
                if (p + 4 > end)
                    return (const unsigned char *)0;
                val = read_be32(p);
                name_len_lw = val & 0x00FFFFFFUL;
                p += 4;
                if (name_len_lw == 0)
                    break;
                if (p + (unsigned long)name_len_lw * 4 > end)
                    return (const unsigned char *)0;
                /* High bit set = XREF; else XDEF (type in high byte) */
                if (val & 0x80000000UL) {
                    ext_type = (unsigned char)((val >> 24) & 0x7F);
                    p += name_len_lw * 4;
                    if (p + 4 > end)
                        return (const unsigned char *)0;
                    num_offsets = read_be32(p);
                    p += 4;
                    if (p + (unsigned long)num_offsets * 4 > end)
                        return (const unsigned char *)0;
                    if (ext_type == EXT_REF32 || ext_type == EXT_RELREF32)
                        hunk->num_xrefs += (short)(num_offsets & 0xFFFF);
                    if (add_xref(ctx, hunk, (unsigned char *)p - 4 - 4 - (size_t)(name_len_lw * 4), ext_type) != 0)
                        return (const unsigned char *)0;
                    p += num_offsets * 4;
                } else {
                    ext_type = (unsigned char)(val >> 24);
                    p += name_len_lw * 4;
                    if (p + 4 > end)
                        return (const unsigned char *)0;
                    p += 4;
                    if (add_xdef(ctx, ctx->active_unit, hunk->hunk_section, (unsigned char *)p - 4 - 4 - (size_t)(name_len_lw * 4)) != 0)
                        return (const unsigned char *)0;
                }
            }
            continue;
        }

        if (block_type == HUNK_SYMBOL) {
            if (hunk->hunk_symbol)
                return (const unsigned char *)0;
            hunk->hunk_symbol = (unsigned char *)p;
            for (;;) {
                if (p + 4 > end)
                    return (const unsigned char *)0;
                name_len_lw = read_be32(p) & 0x00FFFFFFUL;
                p += 4;
                if (name_len_lw == 0)
                    break;
                p += name_len_lw * 4 + 4;
            }
            continue;
        }

        if (block_type == HUNK_DEBUG) {
            if (p + 4 > end)
                return (const unsigned char *)0;
            skip = read_be32(p);
            if (!ctx->no_debug && !hunk->hunk_debug)
                hunk->hunk_debug = (unsigned char *)p;
            p += 4 + (unsigned long)skip * 4;
            continue;
        }

        /* Unknown block - skip or error; for now treat as end of hunk */
        return p - 4;
    }
}

/*
 * Add one unit: parse hunks from *offset until HUNK_END; advance *offset.
 * Returns 1 if next block is HUNK_UNIT, 0 if PHX! or end.
 */
static int add_unit_inner(struct LinkerContext *ctx, const unsigned char *data,
    unsigned long size, unsigned long *offset)
{
    unsigned long type;
    unsigned long hunk_size_lw;
    unsigned long hunk_size;
    struct Section *sec;
    struct Hunk *hunk;
    const unsigned char *p;
    const unsigned char *end;
    unsigned long name_len_lw;
    int idx;
    struct Unit *unit;

    unit = ctx->active_unit;
    if (!unit)
        return 0;

    end = data + size;
    p = data + *offset;
    ctx->hunk_cnt = 0;

    while (p + 4 <= end) {
        type = read_be32(p);
        p += 4;

        if (type == HUNK_UNIT)
            return 1;
        if (type == ALINK_PHX_MAGIC)
            return 0;

        if (type == HUNK_NAME) {
            if (p + 4 > end)
                return 0;
            name_len_lw = read_be32(p);
            p += 4;
            if (p + (unsigned long)name_len_lw * 4 > end)
                return 0;
            get_name(ctx, name_len_lw, (unsigned char *)ctx->sec_name, &p, end);
            continue;
        }

        if (type == HUNK_DEBUG) {
            if (p + 4 > end)
                return 0;
            name_len_lw = read_be32(p);
            p += 4 + (unsigned long)name_len_lw * 4;
            continue;
        }

        if (type != HUNK_CODE && type != HUNK_DATA && type != HUNK_BSS) {
            *offset = (unsigned long)(p - data - 4);
            return 0;
        }

        sec = get_section(ctx, (type & 0xFFFF), ctx->sec_name);
        if (!sec)
            return 0;

        if (p + 4 > end)
            return 0;
        hunk_size_lw = read_be32(p);
        p += 4;
        hunk_size = hunk_size_lw * 4;

        if (ctx->hunk_cnt >= ctx->hunks_per_unit)
            return 0;

        hunk = (struct Hunk *)alink_alloc_clear(ctx, sizeof(struct Hunk));
        if (!hunk)
            return 0;
        hunk->hunk_section = sec;
        hunk->data_size = hunk_size;
        hunk->unit_struct = unit;
        if (sec->hunk_tail) {
            sec->hunk_tail->next = hunk;
            sec->hunk_tail = hunk;
        } else {
            sec->hunk_head = sec->hunk_tail = hunk;
        }
        sec->sec_size += hunk_size;

        if (type != HUNK_BSS && hunk_size > 0) {
            if (p + hunk_size > end)
                return 0;
            hunk->hunk_data = (unsigned char *)alink_alloc(ctx, hunk_size);
            if (!hunk->hunk_data)
                return 0;
            memcpy(hunk->hunk_data, p, (size_t)hunk_size);
            p += hunk_size;
        }

        p = parse_hunk_blocks(ctx, hunk, p, end);
        if (!p)
            return 0;

        idx = (int)ctx->hunk_cnt;
        ctx->unit_info_hunk[idx] = hunk;
        ctx->unit_info_sec[idx] = sec;
        ctx->hunk_cnt++;

        *offset = (unsigned long)(p - data);
    }
    return 0;
}

static int add_xdef(struct LinkerContext *ctx, struct Unit *unit,
    struct Section *sec, unsigned char *xdef_ptr)
{
    unsigned long name_len_lw;
    unsigned long chain_len_lw;
    unsigned int hash;
    unsigned int bucket;
    struct XDEF *xdef;
    struct XDEF *chain;
    unsigned long len_bytes;
    const unsigned char *a;
    const unsigned char *b;
    int cmp;
    unsigned char type_byte;
    unsigned long existing_size;
    unsigned long new_size;
    unsigned char *val_ptr;

    name_len_lw = (read_be32(xdef_ptr) & 0x00FFFFFFUL);
    if (name_len_lw == 0)
        return 0;
    type_byte = (unsigned char)((read_be32(xdef_ptr) >> 24) & 0x7F);
    hash = symbol_hash(xdef_ptr + 4, (unsigned int)name_len_lw);
    bucket = hash & (unsigned int)ctx->hash_tab_mask;
    chain = ctx->xdef_hash_tab[bucket];
    while (chain) {
        chain_len_lw = read_be32(chain->xdef_ptr) & 0x00FFFFFFUL;
        len_bytes = chain_len_lw * 4;
        if (len_bytes == (unsigned long)name_len_lw * 4) {
            a = xdef_ptr + 4;
            b = chain->xdef_ptr + 4;
            for (cmp = 0; len_bytes > 0 && *a == *b; len_bytes--, a++, b++)
                ;
            if (len_bytes == 0) {
                if (type_byte == EXT_COMMON) {
                    val_ptr = chain->xdef_ptr + 4 + (unsigned long)name_len_lw * 4;
                    existing_size = read_be32(val_ptr);
                    new_size = read_be32(xdef_ptr + 4 + name_len_lw * 4);
                    if (new_size > existing_size)
                        write_be32(val_ptr, new_size);
                    return 0;
                }
                if (!ctx->lib_unit)
                    return -1;
            }
        }
        chain = chain->next;
    }
    xdef = (struct XDEF *)alink_alloc(ctx, sizeof(struct XDEF));
    if (!xdef)
        return -1;
    xdef->next = ctx->xdef_hash_tab[bucket];
    xdef->xdef_unit = unit;
    xdef->xdef_sec = sec;
    xdef->xdef_ptr = xdef_ptr;
    ctx->xdef_hash_tab[bucket] = xdef;
    return 0;
}

static int add_xref(struct LinkerContext *ctx, struct Hunk *hunk,
    unsigned char *xref_ptr, unsigned char ref_type)
{
    struct XREF *xref;

    xref = (struct XREF *)alink_alloc(ctx, sizeof(struct XREF));
    if (!xref)
        return -1;
    xref->next = (struct XREF *)0;
    xref->xref_xdef = (struct XDEF *)0;
    xref->xref_hunk = hunk;
    xref->xref_ptr = xref_ptr;
    xref->xref_type = ref_type;
    if (ctx->xref_list_tail) {
        ctx->xref_list_tail->next = xref;
        ctx->xref_list_tail = xref;
    } else {
        ctx->xref_list = ctx->xref_list_tail = xref;
    }
    return 0;
}

static int load_file(struct LinkerContext *ctx, const char *path,
    unsigned char **data_out, unsigned long *size_out)
{
    FILE *f;
    unsigned long size;
    unsigned char *data;

    f = fopen(path, "rb");
    if (!f)
        return -1;
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return -1;
    }
    size = (unsigned long)ftell(f);
    if (size == 0 || size > 0x7FFFFFFFUL) {
        fclose(f);
        return -1;
    }
    rewind(f);
    data = (unsigned char *)alink_alloc(ctx, size);
    if (!data) {
        fclose(f);
        return -1;
    }
    if (fread(data, 1, (size_t)size, f) != (size_t)size) {
        fclose(f);
        return -1;
    }
    fclose(f);
    *data_out = data;
    *size_out = size;
    return 0;
}

int read_units(struct LinkerContext *ctx, char **paths, int npaths)
{
    int i;
    unsigned char *data;
    unsigned long size;
    unsigned long offset;
    unsigned long type;
    unsigned long name_len_lw;
    struct Unit *unit;
    int j;
    struct ObjectFile *ob;
    int ret;

    ctx->unit_info_hunk = (struct Hunk **)alink_alloc_clear(ctx,
        (unsigned long)ctx->hunks_per_unit * sizeof(struct Hunk *));
    ctx->unit_info_sec = (struct Section **)alink_alloc_clear(ctx,
        (unsigned long)ctx->hunks_per_unit * sizeof(struct Section *));
    if (!ctx->unit_info_hunk || !ctx->unit_info_sec)
        return -1;

    for (i = 0; i < npaths; i++) {
        int is_lib;
        size_t plen;

        if (load_file(ctx, paths[i], &data, &size) != 0) {
            fprintf(stderr, "alink: cannot read '%s'\n", paths[i]);
            return -1;
        }
        if (ctx->verbose && !ctx->quiet)
            fprintf(stderr, "%s\n", paths[i]);
        ob = (struct ObjectFile *)alink_alloc(ctx, sizeof(struct ObjectFile));
        if (!ob)
            return -1;
        plen = strlen(paths[i]);
        is_lib = 0;
        if (plen >= 2 && paths[i][plen - 1] == 'o' && paths[i][plen - 2] == '.')
            is_lib = 0;
        else if (plen >= 4 && paths[i][plen - 1] == 'j' && paths[i][plen - 2] == 'b'
            && paths[i][plen - 3] == 'o' && paths[i][plen - 4] == '.')
            is_lib = 0;
        else
            is_lib = 1;
        ob->next = ctx->objects;
        ob->data = data;
        ob->size = size;
        ob->path = (char *)paths[i];
        ctx->objects = ob;
        ctx->obj_name = paths[i];
        ctx->lib_unit = is_lib;

        offset = 0;
        while (offset + 4 <= size) {
            type = read_be32(data + offset);
            if (type == HUNK_LIB) {
                /* Skip HUNK_LIB magic; library is concatenated HUNK_UNITs */
                offset += 4;
                continue;
            }
            if (type != HUNK_UNIT) {
                if (type == ALINK_PHX_MAGIC)
                    break;
                fprintf(stderr, "alink: expected HUNK_UNIT in '%s'\n", paths[i]);
                return -1;
            }
            offset += 4;
            if (offset + 4 > size)
                return -1;
            name_len_lw = read_be32(data + offset);
            offset += 4 + name_len_lw * 4;
            if (offset > size)
                return -1;

            unit = (struct Unit *)alink_alloc_clear(ctx, sizeof(struct Unit));
            if (!unit)
                return -1;
            unit->next = ctx->units;
            unit->select = is_lib ? 0 : 1;  /* libraries: select by refs */
            unit->obj_name = paths[i];
            unit->name = unit->name_buffer;
            if (name_len_lw > 0 && name_len_lw * 4 < ALINK_SECNAMESZ) {
                memcpy(unit->name_buffer, data + offset - name_len_lw * 4,
                    (size_t)(name_len_lw * 4));
                unit->name_buffer[name_len_lw * 4] = '\0';
            }
            ctx->units = unit;
            ctx->active_unit = unit;

            ret = add_unit_inner(ctx, data, size, &offset);
            if (ret < 0)
                return -1;

            unit->num_hunks = (int)ctx->hunk_cnt;
            unit->hunk_ptr = (struct Hunk **)alink_alloc(ctx,
                (unsigned long)unit->num_hunks * sizeof(struct Hunk *));
            unit->hunk_sec = (struct Section **)alink_alloc(ctx,
                (unsigned long)unit->num_hunks * sizeof(struct Section *));
            unit->hunk_offset = (unsigned long *)alink_alloc_clear(ctx,
                (unsigned long)unit->num_hunks * sizeof(unsigned long));
            if (!unit->hunk_ptr || !unit->hunk_sec || !unit->hunk_offset)
                return -1;
            for (j = 0; j < unit->num_hunks; j++) {
                unit->hunk_ptr[j] = ctx->unit_info_hunk[j];
                unit->hunk_sec[j] = ctx->unit_info_sec[j];
            }
        }
    }
    return 0;
}
