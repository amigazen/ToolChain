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
