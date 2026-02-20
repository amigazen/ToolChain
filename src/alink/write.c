/*
 * Write Amiga load file (HUNK_HEADER, sections, RELOC32, HUNK_END).
 * C89, standard library only.
 */
#include <stdio.h>
#include "alink.h"

static int write_section(struct LinkerContext *ctx, struct Section *sec)
{
    struct Hunk *h;
    unsigned long size_lw;
    unsigned char *rp;
    unsigned long count;
    unsigned long sec_id;
    unsigned long off;

    size_lw = sec->sec_size / 4;
    if (!write_be32_file(ctx->out_file, sec->type | (ctx->chip_opt ? HUNKF_CHIP : 0)))
        return -1;
    if (!write_be32_file(ctx->out_file, size_lw))
        return -1;

    h = sec->hunk_head;
    while (h) {
        if (h->hunk_data && h->data_size > 0) {
            if (fwrite(h->hunk_data, 1, (size_t)h->data_size, ctx->out_file)
                != (size_t)h->data_size)
                return -1;
        } else if (h->data_size > 0) {
            unsigned long z;
            unsigned char zero[4] = {0, 0, 0, 0};
            for (z = 0; z < h->data_size; z += 4) {
                if (fwrite(zero, 1, 4, ctx->out_file) != 4)
                    return -1;
            }
        }
        h = h->next;
    }

    /* One HUNK_RELOC32 block: (count, section_id, off1, off2, ...), ..., (0) */
    if (!write_be32_file(ctx->out_file, HUNK_RELOC32))
        return -1;
    h = sec->hunk_head;
    while (h) {
        if (h->hunk_reloc32) {
            rp = h->hunk_reloc32;
            for (;;) {
                count = read_be32(rp);
                rp += 4;
                if (count == 0)
                    break;
                sec_id = read_be32(rp);
                if (!write_be32_file(ctx->out_file, count))
                    return -1;
                if (!write_be32_file(ctx->out_file, sec_id))
                    return -1;
                while (count--) {
                    off = read_be32(rp);
                    rp += 4;
                    if (!write_be32_file(ctx->out_file, off))
                        return -1;
                }
            }
        }
        if (h->hunk_reloc32d) {
            rp = h->hunk_reloc32d;
            for (;;) {
                count = read_be32(rp);
                rp += 4;
                if (count == 0)
                    break;
                sec_id = read_be32(rp);
                if (!write_be32_file(ctx->out_file, count))
                    return -1;
                if (!write_be32_file(ctx->out_file, sec_id))
                    return -1;
                while (count--) {
                    off = read_be32(rp);
                    rp += 4;
                    if (!write_be32_file(ctx->out_file, off))
                        return -1;
                }
            }
        }
        h = h->next;
    }
    if (!write_be32_file(ctx->out_file, 0UL))
        return -1;

    /* HUNK_DREL16: (count, section_id, off1, off2, ...), ..., (0) - only if any hunk has 16d */
    h = sec->hunk_head;
    while (h) {
        if (h->hunk_reloc16d) {
            if (!write_be32_file(ctx->out_file, HUNK_DREL16))
                return -1;
            h = sec->hunk_head;
            while (h) {
                if (h->hunk_reloc16d) {
                    rp = h->hunk_reloc16d;
                    for (;;) {
                        count = read_be32(rp);
                        rp += 4;
                        if (count == 0)
                            break;
                        sec_id = read_be32(rp);
                        if (!write_be32_file(ctx->out_file, count))
                            return -1;
                        if (!write_be32_file(ctx->out_file, sec_id))
                            return -1;
                        while (count--) {
                            off = read_be32(rp);
                            rp += 4;
                            if (!write_be32_file(ctx->out_file, off))
                                return -1;
                        }
                    }
                }
                h = h->next;
            }
            if (!write_be32_file(ctx->out_file, 0UL))
                return -1;
            break;
        }
        h = h->next;
    }
    /* HUNK_DREL8: same format */
    h = sec->hunk_head;
    while (h) {
        if (h->hunk_reloc8d) {
            if (!write_be32_file(ctx->out_file, HUNK_DREL8))
                return -1;
            h = sec->hunk_head;
            while (h) {
                if (h->hunk_reloc8d) {
                    rp = h->hunk_reloc8d;
                    for (;;) {
                        count = read_be32(rp);
                        rp += 4;
                        if (count == 0)
                            break;
                        sec_id = read_be32(rp);
                        if (!write_be32_file(ctx->out_file, count))
                            return -1;
                        if (!write_be32_file(ctx->out_file, sec_id))
                            return -1;
                        while (count--) {
                            off = read_be32(rp);
                            rp += 4;
                            if (!write_be32_file(ctx->out_file, off))
                                return -1;
                        }
                    }
                }
                h = h->next;
            }
            if (!write_be32_file(ctx->out_file, 0UL))
                return -1;
            break;
        }
        h = h->next;
    }

    if (!ctx->no_debug) {
        unsigned char *sp;
        unsigned long sym_len;
        unsigned long nlen;
        h = sec->hunk_head;
        while (h) {
            if (h->hunk_symbol) {
                sp = h->hunk_symbol;
                sym_len = 0;
                for (;;) {
                    nlen = read_be32(sp + sym_len) & 0x00FFFFFFUL;
                    sym_len += 4;
                    if (nlen == 0)
                        break;
                    sym_len += nlen * 4 + 4;
                }
                if (!write_be32_file(ctx->out_file, HUNK_SYMBOL))
                    return -1;
                if (fwrite(h->hunk_symbol, 1, (size_t)sym_len, ctx->out_file) != (size_t)sym_len)
                    return -1;
            }
            if (h->hunk_debug) {
                unsigned long dlen = read_be32(h->hunk_debug);
                if (!write_be32_file(ctx->out_file, HUNK_DEBUG))
                    return -1;
                if (!write_be32_file(ctx->out_file, dlen))
                    return -1;
                if (fwrite(h->hunk_debug + 4, 1, (size_t)(dlen * 4), ctx->out_file) != (size_t)(dlen * 4))
                    return -1;
            }
            h = h->next;
        }
    }

    if (!write_be32_file(ctx->out_file, HUNK_END))
        return -1;
    return 0;
}

int write_load_file(struct LinkerContext *ctx)
{
    struct Section *sec;
    unsigned long first_id;
    unsigned long last_id;
    unsigned long count;

    if (!ctx->out_name[0]) {
        fprintf(stderr, "alink: no output file specified\n");
        return -1;
    }
    ctx->out_file = fopen(ctx->out_name, "wb");
    if (!ctx->out_file) {
        fprintf(stderr, "alink: cannot create '%s'\n", ctx->out_name);
        return -1;
    }

    count = 0;
    sec = ctx->code_sections;
    while (sec) {
        count++;
        sec = sec->next;
    }
    sec = ctx->data_sections;
    while (sec) {
        count++;
        sec = sec->next;
    }
    sec = ctx->bss_sections;
    while (sec) {
        count++;
        sec = sec->next;
    }

    if (count == 0) {
        fprintf(stderr, "alink: no sections to write\n");
        fclose(ctx->out_file);
        return -1;
    }

    first_id = 0;
    last_id = count - 1;
    if (!write_be32_file(ctx->out_file, HUNK_HEADER))
        return -1;
    if (!write_be32_file(ctx->out_file, first_id))
        return -1;
    if (!write_be32_file(ctx->out_file, last_id))
        return -1;
    if (!write_be32_file(ctx->out_file, count))
        return -1;

    sec = ctx->code_sections;
    while (sec) {
        if (write_section(ctx, sec) != 0) {
            fclose(ctx->out_file);
            return -1;
        }
        sec = sec->next;
    }
    sec = ctx->data_sections;
    while (sec) {
        if (write_section(ctx, sec) != 0) {
            fclose(ctx->out_file);
            return -1;
        }
        sec = sec->next;
    }
    sec = ctx->bss_sections;
    while (sec) {
        if (write_section(ctx, sec) != 0) {
            fclose(ctx->out_file);
            return -1;
        }
        sec = sec->next;
    }

    fclose(ctx->out_file);
    ctx->out_file = (FILE *)0;
    return 0;
}

/* Write map file: section and hunk layout (address, size). */
int write_map_file(struct LinkerContext *ctx)
{
    struct Section *sec;
    struct Hunk *h;
    FILE *mf;
    const char *type_str;
    unsigned long addr;
    unsigned int w;

    if (!ctx->map_file || !ctx->map_file[0])
        return 0;
    mf = fopen(ctx->map_file, "w");
    if (!mf) {
        fprintf(stderr, "alink: cannot create map file '%s'\n", ctx->map_file);
        return -1;
    }
    w = (unsigned int)(ctx->map_hwidth > 0 ? ctx->map_hwidth : 80);
    addr = 0;
    sec = ctx->code_sections;
    while (sec) {
        type_str = "CODE";
        fprintf(mf, "Section %lu %s\n", (unsigned long)sec->section_id, sec->section_name[0] ? sec->section_name : type_str);
        h = sec->hunk_head;
        while (h) {
            fprintf(mf, "  %08lX %8lu  ", (unsigned long)addr, (unsigned long)h->data_size);
            if (h->unit_struct && h->unit_struct->obj_name)
                fprintf(mf, " %s\n", h->unit_struct->obj_name);
            else
                fprintf(mf, "\n");
            addr += h->data_size;
            h = h->next;
        }
        sec = sec->next;
    }
    sec = ctx->data_sections;
    while (sec) {
        type_str = "DATA";
        fprintf(mf, "Section %lu %s\n", (unsigned long)sec->section_id, sec->section_name[0] ? sec->section_name : type_str);
        h = sec->hunk_head;
        while (h) {
            fprintf(mf, "  %08lX %8lu  ", (unsigned long)addr, (unsigned long)h->data_size);
            if (h->unit_struct && h->unit_struct->obj_name)
                fprintf(mf, " %s\n", h->unit_struct->obj_name);
            else
                fprintf(mf, "\n");
            addr += h->data_size;
            h = h->next;
        }
        sec = sec->next;
    }
    sec = ctx->bss_sections;
    while (sec) {
        type_str = "BSS";
        fprintf(mf, "Section %lu %s\n", (unsigned long)sec->section_id, sec->section_name[0] ? sec->section_name : type_str);
        h = sec->hunk_head;
        while (h) {
            fprintf(mf, "  %08lX %8lu  ", (unsigned long)addr, (unsigned long)h->data_size);
            if (h->unit_struct && h->unit_struct->obj_name)
                fprintf(mf, " %s\n", h->unit_struct->obj_name);
            else
                fprintf(mf, "\n");
            addr += h->data_size;
            h = h->next;
        }
        sec = sec->next;
    }
    (void)w;
    fclose(mf);
    return 0;
}

/* Write xref file: symbol -> list of (file, offset). */
int write_xref_file(struct LinkerContext *ctx)
{
    struct XREF *xref;
    FILE *xf;
    unsigned long name_len_lw;
    unsigned long num_offsets;
    unsigned long off;
    const unsigned char *patch;

    if (!ctx->xref_file || !ctx->xref_file[0])
        return 0;
    xf = fopen(ctx->xref_file, "w");
    if (!xf) {
        fprintf(stderr, "alink: cannot create xref file '%s'\n", ctx->xref_file);
        return -1;
    }
    xref = ctx->xref_list;
    while (xref) {
        patch = xref->xref_ptr;
        name_len_lw = read_be32(patch) & 0x00FFFFFFUL;
        if (name_len_lw > 0 && name_len_lw * 4 < 256) {
            fprintf(xf, "%.*s", (int)(name_len_lw * 4), (const char *)(patch + 4));
            patch += 4 + name_len_lw * 4;
            num_offsets = read_be32(patch);
            patch += 4;
            fprintf(xf, " ->");
            for (off = 0; off < num_offsets; off++) {
                unsigned long addr = read_be32(patch + off * 4);
                if (xref->xref_hunk && xref->xref_hunk->unit_struct && xref->xref_hunk->unit_struct->obj_name)
                    fprintf(xf, " %s+0x%lX", xref->xref_hunk->unit_struct->obj_name, (unsigned long)addr);
                else
                    fprintf(xf, " 0x%lX", (unsigned long)addr);
            }
            fprintf(xf, "\n");
        }
        xref = xref->next;
    }
    fclose(xf);
    return 0;
}
