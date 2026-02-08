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
    struct Section **head;
    struct Section *sec;

    if (type == HUNK_CODE)
        head = &ctx->code_sections;
    else if (type == HUNK_DATA)
        head = &ctx->data_sections;
    else
        head = &ctx->bss_sections;
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
}

void kill_zero_sections(struct LinkerContext *ctx)
{
    struct Section **head;
    struct Section *sec;
    struct Section *next;

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
}

void calc_hunk_offsets(struct LinkerContext *ctx)
{
    struct Unit *u;
    struct Hunk *h;
    struct Section *sec;
    unsigned long offset;
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
    u = ctx->units;
    while (u) {
        for (i = 0; i < u->num_hunks; i++)
            u->hunk_offset[i] = u->hunk_ptr[i]->sec_base_offset;
        u = u->next;
    }
}
