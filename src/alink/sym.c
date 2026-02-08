/*
 * Symbol table: XDEF hash, SelectUnits, resolve_common.
 * C89, standard library only.
 */
#include <stdio.h>
#include <string.h>
#include "alink.h"

unsigned int alink_symbol_hash(const unsigned char *name_ptr, unsigned int name_len_lw);

static int xdef_same_name(struct XDEF *a, struct XDEF *b)
{
    unsigned long la;
    unsigned long lb;
    const unsigned char *pa;
    const unsigned char *pb;

    la = read_be32(a->xdef_ptr) & 0x00FFFFFFUL;
    lb = read_be32(b->xdef_ptr) & 0x00FFFFFFUL;
    if (la != lb)
        return 0;
    pa = a->xdef_ptr + 4;
    pb = b->xdef_ptr + 4;
    la *= 4;
    while (la-- > 0) {
        if (*pa++ != *pb++)
            return 0;
    }
    return 1;
}

void resolve_common(struct LinkerContext *ctx)
{
    struct Section *common_sec;
    struct XDEF *xdef;
    struct XDEF *x2;
    unsigned long i;
    unsigned long j;
    unsigned long name_len_lw;
    unsigned long max_size;
    unsigned long size_val;
    unsigned long offset;
    struct Hunk *h;
    int has_common;

    has_common = 0;
    for (i = 0; i < ctx->hash_tab_size; i++) {
        for (xdef = ctx->xdef_hash_tab[i]; xdef; xdef = xdef->next) {
            if (xdef_type(xdef) == EXT_COMMON) {
                has_common = 1;
                break;
            }
        }
        if (has_common)
            break;
    }
    if (!has_common)
        return;
    common_sec = create_section(ctx, HUNK_BSS, "COMMON");
    if (!common_sec)
        return;
    offset = 0;
    for (i = 0; i < ctx->hash_tab_size; i++) {
        for (xdef = ctx->xdef_hash_tab[i]; xdef; xdef = xdef->next) {
            if (xdef_type(xdef) != EXT_COMMON || xdef->xdef_sec == common_sec)
                continue;
            name_len_lw = read_be32(xdef->xdef_ptr) & 0x00FFFFFFUL;
            max_size = xdef_value(xdef);
            for (j = 0; j < ctx->hash_tab_size; j++) {
                for (x2 = ctx->xdef_hash_tab[j]; x2; x2 = x2->next) {
                    if (xdef_type(x2) == EXT_COMMON && xdef_same_name(xdef, x2)) {
                        size_val = xdef_value(x2);
                        if (size_val > max_size)
                            max_size = size_val;
                    }
                }
            }
            h = (struct Hunk *)alink_alloc_clear(ctx, sizeof(struct Hunk));
            if (!h)
                return;
            h->data_size = max_size * 4;
            h->hunk_section = common_sec;
            if (common_sec->hunk_tail) {
                common_sec->hunk_tail->next = h;
                common_sec->hunk_tail = h;
            } else {
                common_sec->hunk_head = common_sec->hunk_tail = h;
            }
            common_sec->sec_size += h->data_size;
            for (j = 0; j < ctx->hash_tab_size; j++) {
                for (x2 = ctx->xdef_hash_tab[j]; x2; x2 = x2->next) {
                    if (xdef_type(x2) == EXT_COMMON && xdef_same_name(xdef, x2)) {
                        x2->xdef_sec = common_sec;
                        set_xdef_value(x2, offset);
                    }
                }
            }
            offset += max_size * 4;
        }
    }
}

void add_linker_xdef(struct LinkerContext *ctx, const char *name, unsigned int name_len_lw,
    unsigned char type, unsigned long value)
{
    unsigned int hash;
    unsigned int bucket;
    struct XDEF *xdef;
    unsigned char *block;
    unsigned long blen;
    unsigned int i;

    blen = 4 + (unsigned long)name_len_lw * 4 + 4;
    block = (unsigned char *)alink_alloc(ctx, blen);
    if (!block)
        return;
    write_be32(block, (unsigned long)type << 24 | (unsigned long)name_len_lw);
    for (i = 0; i < name_len_lw * 4; i++)
        block[4 + i] = (unsigned char)(name[i] ? name[i] : 0);
    write_be32(block + 4 + (unsigned long)name_len_lw * 4, value);
    hash = alink_symbol_hash(block + 4, name_len_lw);
    bucket = hash & (unsigned int)ctx->hash_tab_mask;
    xdef = (struct XDEF *)alink_alloc(ctx, sizeof(struct XDEF));
    if (!xdef)
        return;
    xdef->next = ctx->xdef_hash_tab[bucket];
    xdef->xdef_unit = (struct Unit *)0;
    xdef->xdef_sec = (struct Section *)0;
    xdef->xdef_ptr = block;
    ctx->xdef_hash_tab[bucket] = xdef;
}

unsigned int alink_symbol_hash(const unsigned char *name_ptr, unsigned int name_len_lw)
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

struct XDEF *alink_find_xdef(struct LinkerContext *ctx, const unsigned char *name_ptr,
    unsigned int name_len_lw)
{
    unsigned int hash;
    unsigned int bucket;
    struct XDEF *chain;
    unsigned long len_bytes;
    const unsigned char *a;
    const unsigned char *b;

    hash = alink_symbol_hash(name_ptr, name_len_lw);
    bucket = hash & (unsigned int)ctx->hash_tab_mask;
    chain = ctx->xdef_hash_tab[bucket];
    while (chain) {
        len_bytes = (read_be32(chain->xdef_ptr) & 0x00FFFFFFUL) * 4;
        if (len_bytes == (unsigned long)name_len_lw * 4) {
            a = name_ptr;
            b = chain->xdef_ptr + 4;
            for (; len_bytes > 0 && *a == *b; len_bytes--, a++, b++)
                ;
            if (len_bytes == 0)
                return chain;
        }
        chain = chain->next;
    }
    return (struct XDEF *)0;
}

void select_units(struct LinkerContext *ctx)
{
    struct XREF *xref;
    unsigned long name_len_lw;
    struct XDEF *xdef;
    int changed;
    unsigned char *p;

    do {
        changed = 0;
        xref = ctx->xref_list;
        while (xref) {
            if (xref->xref_xdef) {
                xref = xref->next;
                continue;
            }
            p = xref->xref_ptr;
            name_len_lw = read_be32(p) & 0x00FFFFFFUL;
            if (name_len_lw == 0) {
                xref = xref->next;
                continue;
            }
            xdef = alink_find_xdef(ctx, p + 4, (unsigned int)name_len_lw);
            if (xdef) {
                xref->xref_xdef = xdef;
                if (xdef->xdef_unit && !xdef->xdef_unit->select) {
                    xdef->xdef_unit->select = 1;
                    changed = 1;
                }
            } else {
                fprintf(stderr, "alink: undefined symbol '");
                if (name_len_lw * 4 < 64) {
                    fwrite(p + 4, 1, (size_t)(name_len_lw * 4), stderr);
                }
                fprintf(stderr, "'\n");
            }
            xref = xref->next;
        }
    } while (changed);
}
