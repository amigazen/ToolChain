/*
 * Write Amiga load file (HUNK_HEADER, sections, RELOC32, HUNK_END).
 * Overlay format: root + HUNK_OVERLAY + overlay nodes with HUNK_BREAK.
 * C89, standard library only.
 */
#include <stdio.h>
#include <stdlib.h>
#include "alink.h"
#include "doshunks.h"

static int write_section(struct LinkerContext *ctx, struct Section *sec, unsigned long end_marker)
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

    if (!write_be32_file(ctx->out_file, end_marker))
        return -1;
    return 0;
}

/* Find the hunk in section sec that contains ptr; return that hunk and offset within it. */
static struct Hunk *hunk_containing(struct Section *sec, unsigned char *ptr, unsigned long *out_offset)
{
    struct Hunk *hh;

    if (!sec || !ptr || !out_offset)
        return (struct Hunk *)0;
    *out_offset = 0;
    hh = sec->hunk_head;
    while (hh) {
        if (hh->hunk_data && ptr >= hh->hunk_data && ptr < hh->hunk_data + hh->data_size) {
            *out_offset = (unsigned long)(ptr - hh->hunk_data);
            return hh;
        }
        hh = hh->next;
    }
    return (struct Hunk *)0;
}

/* Return global hunk index for a hunk that belongs to overlay node node_index. */
static unsigned long overlay_hunk_global_index(struct LinkerContext *ctx, int node_index, struct Hunk *target)
{
    struct Section *sec;
    struct Hunk *hh;
    unsigned long idx;

    if (!ctx->overlay_nodes || node_index < 0 || node_index >= ctx->n_overlay_nodes || !target)
        return 0;
    idx = ctx->overlay_nodes[node_index].first_hunk;
    sec = ctx->overlay_nodes[node_index].code_sections;
    while (sec) {
        for (hh = sec->hunk_head; hh; hh = hh->next) {
            if (hh == target)
                return idx;
            idx++;
        }
        sec = sec->next;
    }
    sec = ctx->overlay_nodes[node_index].data_sections;
    while (sec) {
        for (hh = sec->hunk_head; hh; hh = hh->next) {
            if (hh == target)
                return idx;
            idx++;
        }
        sec = sec->next;
    }
    sec = ctx->overlay_nodes[node_index].bss_sections;
    while (sec) {
        for (hh = sec->hunk_head; hh; hh = hh->next) {
            if (hh == target)
                return idx;
            idx++;
        }
        sec = sec->next;
    }
    return idx;
}

struct overlay_ref_entry {
    int node_index;
    int level;
    int ordinate;
    unsigned long initial_hunk;
    unsigned long symbol_hunk;
    unsigned long symbol_offset;
};

int write_load_file(struct LinkerContext *ctx)
{
    struct Section *sec;
    struct Hunk *h;
    struct XREF *xref;
    struct XDEF *xdef;
    struct overlay_ref_entry *overlay_refs;
    int n_overlay_refs;
    int caller_node;
    int target_node;
    struct Hunk *sym_hunk;
    unsigned long sym_off;
    unsigned long first_id;
    unsigned long last_id;
    unsigned long count;
    unsigned long root_hunks;
    unsigned long size_lw;
    unsigned long ovlr_size;
    unsigned long tree_size;
    unsigned long *overlay_positions;
    long table_pos;
    int max_level;
    int i;
    int j;

    overlay_refs = (struct overlay_ref_entry *)0;
    n_overlay_refs = 0;
    overlay_positions = (unsigned long *)0;
    table_pos = 0;

    if (!ctx->out_name[0]) {
        fprintf(stderr, "alink: no output file specified\n");
        return -1;
    }
    ctx->out_file = fopen(ctx->out_name, "wb");
    if (!ctx->out_file) {
        fprintf(stderr, "alink: cannot create '%s'\n", ctx->out_name);
        return -1;
    }

    if (ctx->overlay_mode && ctx->overlay_nodes && ctx->n_overlay_nodes > 0) {
        /* Overlay format: root header (TABLE_SIZE, FIRST_HUNK=0, LAST_HUNK, SIZES), root sections, HUNK_END,
           HUNK_OVERLAY, then per-node HUNK_HEADER + sections + HUNK_BREAK. */
        root_hunks = 0;
        sec = ctx->code_sections;
        while (sec) {
            for (h = sec->hunk_head; h; h = h->next)
                root_hunks++;
            sec = sec->next;
        }
        sec = ctx->data_sections;
        while (sec) {
            for (h = sec->hunk_head; h; h = h->next)
                root_hunks++;
            sec = sec->next;
        }
        sec = ctx->bss_sections;
        while (sec) {
            for (h = sec->hunk_head; h; h = h->next)
                root_hunks++;
            sec = sec->next;
        }
        if (root_hunks == 0) {
            fprintf(stderr, "alink: no root sections to write\n");
            fclose(ctx->out_file);
            ctx->out_file = (FILE *)0;
            return -1;
        }
        if (!write_be32_file(ctx->out_file, HUNK_HEADER))
            goto fail;
        if (!write_be32_file(ctx->out_file, 0UL))
            goto fail;
        if (!write_be32_file(ctx->out_file, ctx->overlay_table_size))
            goto fail;
        if (!write_be32_file(ctx->out_file, 0UL))
            goto fail;
        if (!write_be32_file(ctx->out_file, root_hunks - 1))
            goto fail;
        sec = ctx->code_sections;
        while (sec) {
            for (h = sec->hunk_head; h; h = h->next) {
                size_lw = (h->data_size + 3) / 4;
                if (size_lw == 0 && h->data_size > 0)
                    size_lw = 1;
                size_lw |= (ctx->chip_opt ? HUNKF_CHIP : 0) | (ctx->fast_opt ? HUNKF_FAST : 0);
                if (!write_be32_file(ctx->out_file, size_lw))
                    goto fail;
                if (ctx->chip_opt && ctx->fast_opt && !write_be32_file(ctx->out_file, 0UL))  /* memory type for AllocMem */
                    goto fail;
            }
            sec = sec->next;
        }
        sec = ctx->data_sections;
        while (sec) {
            for (h = sec->hunk_head; h; h = h->next) {
                size_lw = (h->data_size + 3) / 4;
                if (size_lw == 0 && h->data_size > 0)
                    size_lw = 1;
                size_lw |= (ctx->chip_opt ? HUNKF_CHIP : 0) | (ctx->fast_opt ? HUNKF_FAST : 0);
                if (!write_be32_file(ctx->out_file, size_lw))
                    goto fail;
                if (ctx->chip_opt && ctx->fast_opt && !write_be32_file(ctx->out_file, 0UL))
                    goto fail;
            }
            sec = sec->next;
        }
        sec = ctx->bss_sections;
        while (sec) {
            for (h = sec->hunk_head; h; h = h->next) {
                size_lw = (h->data_size + 3) / 4;
                if (size_lw == 0 && h->data_size > 0)
                    size_lw = 1;
                size_lw |= (ctx->chip_opt ? HUNKF_CHIP : 0) | (ctx->fast_opt ? HUNKF_FAST : 0);
                if (!write_be32_file(ctx->out_file, size_lw))
                    goto fail;
                if (ctx->chip_opt && ctx->fast_opt && !write_be32_file(ctx->out_file, 0UL))
                    goto fail;
            }
            sec = sec->next;
        }
        sec = ctx->code_sections;
        while (sec) {
            if (write_section(ctx, sec, HUNK_END) != 0)
                goto fail;
            sec = sec->next;
        }
        sec = ctx->data_sections;
        while (sec) {
            if (write_section(ctx, sec, HUNK_END) != 0)
                goto fail;
            sec = sec->next;
        }
        sec = ctx->bss_sections;
        while (sec) {
            if (write_section(ctx, sec, HUNK_END) != 0)
                goto fail;
            sec = sec->next;
        }
        /* Collect overlay references: XREFs from root or parent overlay to symbol in an overlay node. */
        xref = ctx->xref_list;
        while (xref) {
            xdef = xref->xref_xdef;
            if (xdef && xdef->xdef_unit && xdef->xdef_unit->overlay_node >= 0) {
                target_node = xdef->xdef_unit->overlay_node;
                caller_node = (xref->xref_hunk && xref->xref_hunk->unit_struct)
                    ? xref->xref_hunk->unit_struct->overlay_node : -1;
                if (caller_node < 0 ||
                    (ctx->overlay_nodes[target_node].parent_index == caller_node)) {
                    n_overlay_refs++;
                }
            }
            xref = xref->next;
        }
        if (n_overlay_refs > 0) {
            overlay_refs = (struct overlay_ref_entry *)malloc((size_t)n_overlay_refs * sizeof(struct overlay_ref_entry));
            if (overlay_refs) {
                i = 0;
                xref = ctx->xref_list;
                while (xref && i < n_overlay_refs) {
                    xdef = xref->xref_xdef;
                    if (xdef && xdef->xdef_unit && xdef->xdef_unit->overlay_node >= 0) {
                        target_node = xdef->xdef_unit->overlay_node;
                        caller_node = (xref->xref_hunk && xref->xref_hunk->unit_struct)
                            ? xref->xref_hunk->unit_struct->overlay_node : -1;
                        if (caller_node < 0 ||
                            (ctx->overlay_nodes[target_node].parent_index == caller_node)) {
                            overlay_refs[i].node_index = target_node;
                            overlay_refs[i].level = ctx->overlay_nodes[target_node].level;
                            overlay_refs[i].ordinate = ctx->overlay_nodes[target_node].ordinate;
                            overlay_refs[i].initial_hunk = ctx->overlay_nodes[target_node].first_hunk;
                            sym_hunk = hunk_containing(xdef->xdef_sec, xdef->xdef_ptr, &sym_off);
                            if (sym_hunk) {
                                overlay_refs[i].symbol_hunk = overlay_hunk_global_index(ctx, target_node, sym_hunk);
                                overlay_refs[i].symbol_offset = sym_off;
                            } else {
                                overlay_refs[i].symbol_hunk = ctx->overlay_nodes[target_node].first_hunk;
                                overlay_refs[i].symbol_offset = 0;
                            }
                            i++;
                        }
                    }
                    xref = xref->next;
                }
            } else {
                n_overlay_refs = 0;
            }
        }
        /* HUNK_OVERLAY per Overlay.doc: h = longest path (root to leaf) including root = max_level+2;
           TREE_SIZE = h+1, TREE_PTRs = h-1 zero longwords (no entry for root), then terminator 0. */
        max_level = 0;
        for (i = 0; i < ctx->n_overlay_nodes; i++) {
            if (ctx->overlay_nodes[i].level > max_level)
                max_level = ctx->overlay_nodes[i].level;
        }
        /* h = path length = root + (levels 0..max_level) = max_level + 2 */
        tree_size = (unsigned long)(max_level + 3);   /* TREE_SIZE = h+1 */
        count = (unsigned long)(n_overlay_refs > 0 ? n_overlay_refs : ctx->n_overlay_nodes);
        ovlr_size = 1 + (unsigned long)(max_level + 1) + 1 + count * 8 - 1;  /* TREE_SIZE + TREE_PTRs(h-1) + term + table */
        if (!write_be32_file(ctx->out_file, HUNK_OVERLAY))
            goto fail;
        if (!write_be32_file(ctx->out_file, ovlr_size))
            goto fail;
        if (!write_be32_file(ctx->out_file, tree_size))
            goto fail;
        for (j = 0; j < max_level + 1; j++) {  /* h-1 = max_level+1 zero longwords */
            if (!write_be32_file(ctx->out_file, 0UL))
                goto fail;
        }
        if (!write_be32_file(ctx->out_file, 0UL))
            goto fail;
        overlay_positions = (unsigned long *)malloc((size_t)ctx->n_overlay_nodes * sizeof(unsigned long));
        if (!overlay_positions)
            goto fail;
        table_pos = ftell(ctx->out_file);
        for (j = 0; (unsigned long)j < count * 8; j++) {
            if (!write_be32_file(ctx->out_file, 0UL))
                goto fail;
        }

        /* Write each overlay node: HUNK_HEADER (FIRST_HUNK, LAST_HUNK, SIZES), sections, HUNK_BREAK on last. */
        for (i = 0; i < ctx->n_overlay_nodes; i++) {
            struct Section *last_sec;

            last_sec = (struct Section *)0;
            overlay_positions[i] = (unsigned long)ftell(ctx->out_file);
            if (!write_be32_file(ctx->out_file, HUNK_HEADER))
                goto fail;
            if (!write_be32_file(ctx->out_file, ctx->overlay_nodes[i].first_hunk))
                goto fail;
            if (!write_be32_file(ctx->out_file, ctx->overlay_nodes[i].first_hunk + (ctx->overlay_nodes[i].num_hunks ? ctx->overlay_nodes[i].num_hunks - 1 : 0)))
                goto fail;
            sec = ctx->overlay_nodes[i].code_sections;
            while (sec) {
                for (h = sec->hunk_head; h; h = h->next) {
                    size_lw = (h->data_size + 3) / 4;
                    if (size_lw == 0 && h->data_size > 0)
                        size_lw = 1;
                    size_lw |= (ctx->chip_opt ? HUNKF_CHIP : 0) | (ctx->fast_opt ? HUNKF_FAST : 0);
                    if (!write_be32_file(ctx->out_file, size_lw))
                        goto fail;
                    if (ctx->chip_opt && ctx->fast_opt && !write_be32_file(ctx->out_file, 0UL))
                        goto fail;
                }
                last_sec = sec;
                sec = sec->next;
            }
            sec = ctx->overlay_nodes[i].data_sections;
            while (sec) {
                for (h = sec->hunk_head; h; h = h->next) {
                    size_lw = (h->data_size + 3) / 4;
                    if (size_lw == 0 && h->data_size > 0)
                        size_lw = 1;
                    size_lw |= (ctx->chip_opt ? HUNKF_CHIP : 0) | (ctx->fast_opt ? HUNKF_FAST : 0);
                    if (!write_be32_file(ctx->out_file, size_lw))
                        goto fail;
                    if (ctx->chip_opt && ctx->fast_opt && !write_be32_file(ctx->out_file, 0UL))
                        goto fail;
                }
                last_sec = sec;
                sec = sec->next;
            }
            sec = ctx->overlay_nodes[i].bss_sections;
            while (sec) {
                for (h = sec->hunk_head; h; h = h->next) {
                    size_lw = (h->data_size + 3) / 4;
                    if (size_lw == 0 && h->data_size > 0)
                        size_lw = 1;
                    size_lw |= (ctx->chip_opt ? HUNKF_CHIP : 0) | (ctx->fast_opt ? HUNKF_FAST : 0);
                    if (!write_be32_file(ctx->out_file, size_lw))
                        goto fail;
                    if (ctx->chip_opt && ctx->fast_opt && !write_be32_file(ctx->out_file, 0UL))
                        goto fail;
                }
                last_sec = sec;
                sec = sec->next;
            }
            sec = ctx->overlay_nodes[i].code_sections;
            while (sec) {
                if (write_section(ctx, sec, sec == last_sec ? HUNK_BREAK : HUNK_END) != 0)
                    goto fail;
                sec = sec->next;
            }
            sec = ctx->overlay_nodes[i].data_sections;
            while (sec) {
                if (write_section(ctx, sec, sec == last_sec ? HUNK_BREAK : HUNK_END) != 0)
                    goto fail;
                sec = sec->next;
            }
            sec = ctx->overlay_nodes[i].bss_sections;
            while (sec) {
                if (write_section(ctx, sec, sec == last_sec ? HUNK_BREAK : HUNK_END) != 0)
                    goto fail;
                sec = sec->next;
            }
        }
        if (fseek(ctx->out_file, table_pos, SEEK_SET) != 0)
            goto fail;
        if (n_overlay_refs > 0 && overlay_refs) {
            for (i = 0; i < n_overlay_refs; i++) {
                if (!write_be32_file(ctx->out_file, overlay_positions[overlay_refs[i].node_index]))
                    goto fail;
                if (!write_be32_file(ctx->out_file, 0UL))
                    goto fail;
                if (!write_be32_file(ctx->out_file, 0UL))
                    goto fail;
                if (!write_be32_file(ctx->out_file, (unsigned long)overlay_refs[i].level + 1))
                    goto fail;
                if (!write_be32_file(ctx->out_file, (unsigned long)overlay_refs[i].ordinate))
                    goto fail;
                if (!write_be32_file(ctx->out_file, overlay_refs[i].initial_hunk))
                    goto fail;
                if (!write_be32_file(ctx->out_file, overlay_refs[i].symbol_hunk))
                    goto fail;
                if (!write_be32_file(ctx->out_file, overlay_refs[i].symbol_offset))
                    goto fail;
            }
        } else {
            for (i = 0; i < ctx->n_overlay_nodes; i++) {
                if (!write_be32_file(ctx->out_file, overlay_positions[i]))
                    goto fail;
                if (!write_be32_file(ctx->out_file, 0UL))
                    goto fail;
                if (!write_be32_file(ctx->out_file, 0UL))
                    goto fail;
                if (!write_be32_file(ctx->out_file, (unsigned long)ctx->overlay_nodes[i].level + 1))
                    goto fail;
                if (!write_be32_file(ctx->out_file, (unsigned long)ctx->overlay_nodes[i].ordinate))
                    goto fail;
                if (!write_be32_file(ctx->out_file, ctx->overlay_nodes[i].first_hunk))
                    goto fail;
                if (!write_be32_file(ctx->out_file, 0UL))
                    goto fail;
                if (!write_be32_file(ctx->out_file, 0UL))
                    goto fail;
            }
        }
        if (overlay_refs)
            free(overlay_refs);
        free(overlay_positions);
        fclose(ctx->out_file);
        ctx->out_file = (FILE *)0;
        return 0;
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
        if (write_section(ctx, sec, HUNK_END) != 0) {
            fclose(ctx->out_file);
            return -1;
        }
        sec = sec->next;
    }
    sec = ctx->data_sections;
    while (sec) {
        if (write_section(ctx, sec, HUNK_END) != 0) {
            fclose(ctx->out_file);
            return -1;
        }
        sec = sec->next;
    }
    sec = ctx->bss_sections;
    while (sec) {
        if (write_section(ctx, sec, HUNK_END) != 0) {
            fclose(ctx->out_file);
            return -1;
        }
        sec = sec->next;
    }

    fclose(ctx->out_file);
    ctx->out_file = (FILE *)0;
    return 0;
fail:
    if (overlay_refs)
        free(overlay_refs);
    if (overlay_positions)
        free(overlay_positions);
    fclose(ctx->out_file);
    ctx->out_file = (FILE *)0;
    return -1;
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
