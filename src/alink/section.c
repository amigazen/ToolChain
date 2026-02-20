/*
 * Section and unit layout: KillUnselected, KillZeroSecs, CalcHunkOffsets.
 * C89, standard library only.
 */
#include <string.h>
#include "alink.h"

void kill_unselected(struct LinkerContext *ctx)
{
    struct Unit **prev;
    struct Unit *u;
    struct Hunk *h;
    struct Section *sec;
    int i;

    prev = &ctx->units;
    u = ctx->units;
    while (u) {
        if (!u->select) {
            *prev = u->next;
            for (i = 0; i < u->num_hunks; i++) {
                h = u->hunk_ptr[i];
                sec = h->hunk_section;
                if (sec->hunk_head == h) {
                    sec->hunk_head = h->next;
                    if (sec->hunk_tail == h)
                        sec->hunk_tail = (struct Hunk *)0;
                } else {
                    struct Hunk *t = sec->hunk_head;
                    while (t && t->next != h)
                        t = t->next;
                    if (t) {
                        t->next = h->next;
                        if (sec->hunk_tail == h)
                            sec->hunk_tail = t;
                    }
                }
                sec->sec_size -= h->data_size;
            }
            u = u->next;
            continue;
        }
        prev = &u->next;
        u = u->next;
    }
}

static void remove_section_from_list(struct Section **head, struct Section *sec)
{
    struct Section **p;

    p = head;
    while (*p) {
        if (*p == sec) {
            *p = sec->next;
            return;
        }
        p = &(*p)->next;
    }
}

struct Section *create_section(struct LinkerContext *ctx, unsigned long type, const char *name)
{
    return create_section_overlay(ctx, type, name, -1);
}

void prepare_overlay_stubs(struct LinkerContext *ctx)
{
    struct XREF *xref;
    struct XDEF *xdef;
    struct Section *sec;
    struct Section *tail;
    struct Hunk *hunk;
    int n_refs;
    int caller_node;
    int target_node;
    int idx;
    unsigned long stub_size;

    ctx->overlay_stub_sec = (struct Section *)0;
    if (!ctx->overlay_mode || !ctx->overlay_nodes || ctx->n_overlay_nodes <= 0)
        return;
    n_refs = 0;
    xref = ctx->xref_list;
    while (xref) {
        xdef = xref->xref_xdef;
        if (xdef && xdef->xdef_unit && xdef->xdef_unit->overlay_node >= 0) {
            target_node = xdef->xdef_unit->overlay_node;
            caller_node = (xref->xref_hunk && xref->xref_hunk->unit_struct)
                ? xref->xref_hunk->unit_struct->overlay_node : -1;
            if (caller_node < 0 ||
                (ctx->overlay_nodes[target_node].parent_index == caller_node))
                n_refs++;
        }
        xref = xref->next;
    }
    if (n_refs <= 0)
        return;
    idx = 0;
    xref = ctx->xref_list;
    while (xref) {
        xdef = xref->xref_xdef;
        if (xdef && xdef->xdef_unit && xdef->xdef_unit->overlay_node >= 0) {
            target_node = xdef->xdef_unit->overlay_node;
            caller_node = (xref->xref_hunk && xref->xref_hunk->unit_struct)
                ? xref->xref_hunk->unit_struct->overlay_node : -1;
            if (caller_node < 0 ||
                (ctx->overlay_nodes[target_node].parent_index == caller_node)) {
                xref->overlay_index = idx++;
            }
        }
        xref = xref->next;
    }
    sec = (struct Section *)alink_alloc_clear(ctx, sizeof(struct Section));
    if (!sec)
        return;
    sec->type = HUNK_CODE;
    strncpy(sec->section_name, "overlay_stubs", ALINK_SECNAMESZ - 1);
    sec->section_name[ALINK_SECNAMESZ - 1] = '\0';
    sec->next = (struct Section *)0;
    stub_size = (unsigned long)n_refs * 8;
    hunk = (struct Hunk *)alink_alloc_clear(ctx, sizeof(struct Hunk));
    if (!hunk)
        return;
    hunk->hunk_data = (unsigned char *)alink_alloc_clear(ctx, stub_size);
    if (!hunk->hunk_data)
        return;
    hunk->data_size = stub_size;
    hunk->hunk_section = sec;
    sec->hunk_head = sec->hunk_tail = hunk;
    sec->sec_size = stub_size;
    tail = ctx->code_sections;
    if (tail) {
        while (tail->next)
            tail = tail->next;
        tail->next = sec;
    } else {
        ctx->code_sections = sec;
    }
    ctx->overlay_stub_sec = sec;
}

/* Cross-section RELOC16/8: collect refs, create one stub hunk per code section that has refs. */
void prepare_jump_stubs(struct LinkerContext *ctx)
{
    struct Unit *u;
    struct Hunk *h;
    struct Section *sec;
    struct JumpStubRef *jr;
    unsigned char *rp;
    unsigned long count;
    unsigned long hunk_num;
    unsigned long off;
    unsigned long stub_size;
    int i;
    int n_refs;

    ctx->jump_stub_refs = (struct JumpStubRef *)0;
    ctx->jump_stub_refs_tail = (struct JumpStubRef *)0;
    u = ctx->units;
    while (u) {
        for (i = 0; i < u->num_hunks; i++) {
            h = u->hunk_ptr[i];
            if (h->hunk_reloc16) {
                rp = h->hunk_reloc16;
                for (;;) {
                    count = read_be32(rp);
                    rp += 4;
                    if (count == 0)
                        break;
                    hunk_num = read_be32(rp);
                    rp += 4;
                    while (count--) {
                        off = read_be32(rp);
                        rp += 4;
                        if (u->hunk_sec[hunk_num] != h->hunk_section) {
                            jr = (struct JumpStubRef *)alink_alloc(ctx, sizeof(struct JumpStubRef));
                            if (!jr)
                                return;
                            jr->next = (struct JumpStubRef *)0;
                            jr->ref_hunk = h;
                            jr->ref_off = off;
                            jr->target_hunk_num = (int)hunk_num;
                            jr->is_reloc16 = 1;
                            jr->stub_hunk = (struct Hunk *)0;
                            jr->stub_off = 0;
                            if (ctx->jump_stub_refs_tail)
                                ctx->jump_stub_refs_tail->next = jr;
                            else
                                ctx->jump_stub_refs = jr;
                            ctx->jump_stub_refs_tail = jr;
                        }
                    }
                }
            }
            if (h->hunk_reloc8) {
                rp = h->hunk_reloc8;
                for (;;) {
                    count = read_be32(rp);
                    rp += 4;
                    if (count == 0)
                        break;
                    hunk_num = read_be32(rp);
                    rp += 4;
                    while (count--) {
                        off = read_be32(rp);
                        rp += 4;
                        if (u->hunk_sec[hunk_num] != h->hunk_section) {
                            jr = (struct JumpStubRef *)alink_alloc(ctx, sizeof(struct JumpStubRef));
                            if (!jr)
                                return;
                            jr->next = (struct JumpStubRef *)0;
                            jr->ref_hunk = h;
                            jr->ref_off = off;
                            jr->target_hunk_num = (int)hunk_num;
                            jr->is_reloc16 = 0;
                            jr->stub_hunk = (struct Hunk *)0;
                            jr->stub_off = 0;
                            if (ctx->jump_stub_refs_tail)
                                ctx->jump_stub_refs_tail->next = jr;
                            else
                                ctx->jump_stub_refs = jr;
                            ctx->jump_stub_refs_tail = jr;
                        }
                    }
                }
            }
        }
        u = u->next;
    }
    if (!ctx->jump_stub_refs)
        return;
    /* One stub hunk per code section that has cross-section refs; append to that section. */
    sec = ctx->code_sections;
    while (sec) {
        n_refs = 0;
        jr = ctx->jump_stub_refs;
        while (jr) {
            if (jr->ref_hunk->hunk_section == sec)
                n_refs++;
            jr = jr->next;
        }
        if (n_refs > 0) {
            stub_size = (unsigned long)n_refs * 6;
            h = (struct Hunk *)alink_alloc_clear(ctx, sizeof(struct Hunk));
            if (!h)
                return;
            h->hunk_data = (unsigned char *)alink_alloc_clear(ctx, stub_size);
            if (!h->hunk_data)
                return;
            h->data_size = stub_size;
            h->hunk_section = sec;
            h->unit_struct = (struct Unit *)0;
            if (sec->hunk_tail) {
                sec->hunk_tail->next = h;
                sec->hunk_tail = h;
            } else {
                sec->hunk_head = sec->hunk_tail = h;
            }
            sec->sec_size += stub_size;
            n_refs = 0;
            jr = ctx->jump_stub_refs;
            while (jr) {
                if (jr->ref_hunk->hunk_section == sec) {
                    jr->stub_hunk = h;
                    jr->stub_off = (unsigned long)n_refs * 6;
                    n_refs++;
                }
                jr = jr->next;
            }
        }
        sec = sec->next;
    }
    /* Overlay code sections: same treatment. */
    if (ctx->overlay_mode && ctx->overlay_nodes) {
        for (i = 0; i < ctx->n_overlay_nodes; i++) {
            sec = ctx->overlay_nodes[i].code_sections;
            while (sec) {
                n_refs = 0;
                jr = ctx->jump_stub_refs;
                while (jr) {
                    if (jr->ref_hunk->hunk_section == sec)
                        n_refs++;
                    jr = jr->next;
                }
                if (n_refs > 0) {
                    stub_size = (unsigned long)n_refs * 6;
                    h = (struct Hunk *)alink_alloc_clear(ctx, sizeof(struct Hunk));
                    if (!h)
                        return;
                    h->hunk_data = (unsigned char *)alink_alloc_clear(ctx, stub_size);
                    if (!h->hunk_data)
                        return;
                    h->data_size = stub_size;
                    h->hunk_section = sec;
                    h->unit_struct = (struct Unit *)0;
                    if (sec->hunk_tail) {
                        sec->hunk_tail->next = h;
                        sec->hunk_tail = h;
                    } else {
                        sec->hunk_head = sec->hunk_tail = h;
                    }
                    sec->sec_size += stub_size;
                    n_refs = 0;
                    jr = ctx->jump_stub_refs;
                    while (jr) {
                        if (jr->ref_hunk->hunk_section == sec) {
                            jr->stub_hunk = h;
                            jr->stub_off = (unsigned long)n_refs * 6;
                            n_refs++;
                        }
                        jr = jr->next;
                    }
                }
                sec = sec->next;
            }
        }
    }
}

struct Section *create_section_overlay(struct LinkerContext *ctx, unsigned long type, const char *name, int node_index)
{
    struct Section **head;
    struct Section *sec;

    if (node_index >= 0 && ctx->overlay_nodes) {
        if (type == HUNK_CODE)
            head = &ctx->overlay_nodes[node_index].code_sections;
        else if (type == HUNK_DATA)
            head = &ctx->overlay_nodes[node_index].data_sections;
        else
            head = &ctx->overlay_nodes[node_index].bss_sections;
    } else {
        if (type == HUNK_CODE)
            head = &ctx->code_sections;
        else if (type == HUNK_DATA)
            head = &ctx->data_sections;
        else
            head = &ctx->bss_sections;
    }
    sec = (struct Section *)alink_alloc_clear(ctx, sizeof(struct Section));
    if (!sec)
        return (struct Section *)0;
    sec->type = type;
    sec->section_id = (unsigned long)ctx->id_cnt++;
    if (name)
        strncpy(sec->section_name, name, ALINK_SECNAMESZ - 1);
    sec->section_name[ALINK_SECNAMESZ - 1] = '\0';
    sec->next = *head;
    *head = sec;
    return sec;
}

void combine_bss_onto_data(struct LinkerContext *ctx)
{
    struct Section *data_sec;
    struct Section *bss_sec;
    struct Hunk *h;
    unsigned long data_only;

    if (ctx->frag_opt || (!ctx->small_data && !ctx->res_opt))
        return;
    data_sec = ctx->data_sections;
    if (!data_sec)
        return;
    while (data_sec->next)
        data_sec = data_sec->next;
    data_only = data_sec->sec_size;
    ctx->data_bas_sec = data_sec;
    bss_sec = ctx->bss_sections;
    while (bss_sec) {
        for (h = bss_sec->hunk_head; h; h = h->next) {
            if (data_sec->hunk_tail) {
                data_sec->hunk_tail->next = h;
                data_sec->hunk_tail = h;
            } else {
                data_sec->hunk_head = data_sec->hunk_tail = h;
            }
            h->hunk_section = data_sec;
            data_sec->sec_size += h->data_size;
        }
        bss_sec->hunk_head = bss_sec->hunk_tail = (struct Hunk *)0;
        bss_sec->sec_size = 0;
        bss_sec = bss_sec->next;
    }
    ctx->bss_sections = (struct Section *)0;
    ctx->data_len_lw = data_only / 4;
    ctx->bss_len_lw = (data_sec->sec_size - data_only) / 4;
}


void set_section_ids(struct LinkerContext *ctx)
{
    struct Section *sec;
    unsigned long id;
    int i;

    id = 0;
    sec = ctx->code_sections;
    while (sec) {
        sec->section_id = id++;
        sec = sec->next;
    }
    sec = ctx->data_sections;
    while (sec) {
        sec->section_id = id++;
        sec = sec->next;
    }
    sec = ctx->bss_sections;
    while (sec) {
        sec->section_id = id++;
        sec = sec->next;
    }
    if (ctx->overlay_mode && ctx->overlay_nodes) {
        for (i = 0; i < ctx->n_overlay_nodes; i++) {
            sec = ctx->overlay_nodes[i].code_sections;
            while (sec) {
                sec->section_id = id++;
                sec = sec->next;
            }
            sec = ctx->overlay_nodes[i].data_sections;
            while (sec) {
                sec->section_id = id++;
                sec = sec->next;
            }
            sec = ctx->overlay_nodes[i].bss_sections;
            while (sec) {
                sec->section_id = id++;
                sec = sec->next;
            }
        }
    }
}

void kill_zero_sections(struct LinkerContext *ctx)
{
    struct Section **head;
    struct Section *sec;
    struct Section *next;
    int i;

    if (!ctx->rem_empty)
        return;
    head = &ctx->code_sections;
    sec = *head;
    while (sec) {
        next = sec->next;
        if (sec->sec_size == 0)
            remove_section_from_list(head, sec);
        sec = next;
    }
    head = &ctx->data_sections;
    sec = *head;
    while (sec) {
        next = sec->next;
        if (sec->sec_size == 0)
            remove_section_from_list(head, sec);
        sec = next;
    }
    head = &ctx->bss_sections;
    sec = *head;
    while (sec) {
        next = sec->next;
        if (sec->sec_size == 0)
            remove_section_from_list(head, sec);
        sec = next;
    }
    if (ctx->overlay_mode && ctx->overlay_nodes) {
        for (i = 0; i < ctx->n_overlay_nodes; i++) {
            head = &ctx->overlay_nodes[i].code_sections;
            sec = *head;
            while (sec) {
                next = sec->next;
                if (sec->sec_size == 0)
                    remove_section_from_list(head, sec);
                sec = next;
            }
            head = &ctx->overlay_nodes[i].data_sections;
            sec = *head;
            while (sec) {
                next = sec->next;
                if (sec->sec_size == 0)
                    remove_section_from_list(head, sec);
                sec = next;
            }
            head = &ctx->overlay_nodes[i].bss_sections;
            sec = *head;
            while (sec) {
                next = sec->next;
                if (sec->sec_size == 0)
                    remove_section_from_list(head, sec);
                sec = next;
            }
        }
    }
}

void calc_hunk_offsets(struct LinkerContext *ctx)
{
    struct Unit *u;
    struct Hunk *h;
    struct Section *sec;
    unsigned long offset;
    unsigned long root_hunks;
    unsigned long run;
    int i;

    for (sec = ctx->code_sections; sec; sec = sec->next) {
        offset = 0;
        for (h = sec->hunk_head; h; h = h->next) {
            h->sec_base_offset = offset;
            offset += h->data_size;
        }
    }
    for (sec = ctx->data_sections; sec; sec = sec->next) {
        offset = 0;
        for (h = sec->hunk_head; h; h = h->next) {
            h->sec_base_offset = offset;
            offset += h->data_size;
        }
    }
    for (sec = ctx->bss_sections; sec; sec = sec->next) {
        offset = 0;
        for (h = sec->hunk_head; h; h = h->next) {
            h->sec_base_offset = offset;
            offset += h->data_size;
        }
    }
    if (ctx->overlay_mode && ctx->overlay_nodes) {
        for (i = 0; i < ctx->n_overlay_nodes; i++) {
            sec = ctx->overlay_nodes[i].code_sections;
            while (sec) {
                offset = 0;
                for (h = sec->hunk_head; h; h = h->next) {
                    h->sec_base_offset = offset;
                    offset += h->data_size;
                }
                sec = sec->next;
            }
            sec = ctx->overlay_nodes[i].data_sections;
            while (sec) {
                offset = 0;
                for (h = sec->hunk_head; h; h = h->next) {
                    h->sec_base_offset = offset;
                    offset += h->data_size;
                }
                sec = sec->next;
            }
            sec = ctx->overlay_nodes[i].bss_sections;
            while (sec) {
                offset = 0;
                for (h = sec->hunk_head; h; h = h->next) {
                    h->sec_base_offset = offset;
                    offset += h->data_size;
                }
                sec = sec->next;
            }
        }
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
        run = root_hunks;
        for (i = 0; i < ctx->n_overlay_nodes; i++) {
            ctx->overlay_nodes[i].first_hunk = run;
            ctx->overlay_nodes[i].num_hunks = 0;
            sec = ctx->overlay_nodes[i].code_sections;
            while (sec) {
                for (h = sec->hunk_head; h; h = h->next)
                    ctx->overlay_nodes[i].num_hunks++;
                sec = sec->next;
            }
            sec = ctx->overlay_nodes[i].data_sections;
            while (sec) {
                for (h = sec->hunk_head; h; h = h->next)
                    ctx->overlay_nodes[i].num_hunks++;
                sec = sec->next;
            }
            sec = ctx->overlay_nodes[i].bss_sections;
            while (sec) {
                for (h = sec->hunk_head; h; h = h->next)
                    ctx->overlay_nodes[i].num_hunks++;
                sec = sec->next;
            }
            run += ctx->overlay_nodes[i].num_hunks;
        }
        ctx->overlay_table_size = run;
    }
    u = ctx->units;
    while (u) {
        for (i = 0; i < u->num_hunks; i++)
            u->hunk_offset[i] = u->hunk_ptr[i]->sec_base_offset;
        u = u->next;
    }
}
