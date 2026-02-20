/*
 * Relocation and XREF resolution (Correction).
 * C89, standard library only.
 */
#include "alink.h"

unsigned long xdef_value(struct XDEF *xdef)
{
    unsigned char *p;
    unsigned long name_len_lw;

    p = xdef->xdef_ptr;
    name_len_lw = read_be32(p) & 0x00FFFFFFUL;
    return read_be32(p + 4 + name_len_lw * 4);
}

unsigned char xdef_type(struct XDEF *xdef)
{
    return (unsigned char)((read_be32(xdef->xdef_ptr) >> 24) & 0x7F);
}

static int xdef_is_abs(struct XDEF *xdef)
{
    return xdef_type(xdef) == EXT_ABS;
}

void set_xdef_value(struct XDEF *xdef, unsigned long value)
{
    unsigned char *p;
    unsigned long name_len_lw;

    p = xdef->xdef_ptr;
    name_len_lw = read_be32(p) & 0x00FFFFFFUL;
    write_be32(p + 4 + name_len_lw * 4, value);
}

void set_lnk_xdef(struct LinkerContext *ctx)
{
    struct Section *sec;
    struct XDEF *xdef;

    if (!ctx->data_bas_sec) {
        ctx->data_len_lw = 0;
        ctx->bss_len_lw = 0;
        sec = ctx->data_sections;
        if (sec)
            ctx->data_bas_sec = sec;
        while (sec) {
            ctx->data_len_lw += sec->sec_size / 4;
            sec = sec->next;
        }
        sec = ctx->bss_sections;
        while (sec) {
            ctx->bss_len_lw += sec->sec_size / 4;
            sec = sec->next;
        }
    }
    xdef = alink_find_xdef(ctx, (const unsigned char *)"__DATA_BAS_\0\0", 3);
    if (xdef) {
        xdef->xdef_sec = ctx->data_bas_sec;
        set_xdef_value(xdef, 0);
    }
    xdef = alink_find_xdef(ctx, (const unsigned char *)"__DATA_LEN_\0\0", 3);
    if (xdef)
        set_xdef_value(xdef, ctx->data_len_lw);
    xdef = alink_find_xdef(ctx, (const unsigned char *)"__BSS_LEN_\0\0\0", 3);
    if (xdef)
        set_xdef_value(xdef, ctx->bss_len_lw);
    xdef = alink_find_xdef(ctx, (const unsigned char *)"__RESIDENT\0\0\0", 3);
    if (xdef)
        set_xdef_value(xdef, (unsigned long)(ctx->res_opt ? 1 : 0));
}

void correction(struct LinkerContext *ctx)
{
    struct Unit *u;
    struct Hunk *h;
    unsigned char *data;
    unsigned char *rp;
    unsigned long count;
    unsigned long hunk_num;
    unsigned long target_sec_id;
    unsigned long target_offset;
    unsigned long my_offset;
    unsigned long patch_addr;
    int i;
    struct XREF *xref;
    struct XDEF *xdef;
    unsigned long name_len_lw;
    unsigned long value;
    unsigned long num_offsets;
    unsigned long off;
    unsigned char *patch;
    unsigned char *ext_p;
    struct XDEF *ovlymgr_xdef;
    unsigned long manager_addr;
    unsigned long stub_base;
    unsigned char *stub_data;
    unsigned long n_stubs;
    unsigned long idx;

    u = ctx->units;
    while (u) {
        for (i = 0; i < u->num_hunks; i++) {
            h = u->hunk_ptr[i];
            data = h->hunk_data;
            my_offset = h->sec_base_offset;
            if (!data)
                continue;

            /* RELOC32: add target section offset to each long at the given offsets */
            if (h->hunk_reloc32) {
                rp = h->hunk_reloc32;
                for (;;) {
                    count = read_be32(rp);
                    rp += 4;
                    if (count == 0)
                        break;
                    hunk_num = read_be32(rp);
                    target_sec_id = u->hunk_sec[hunk_num]->section_id;
                    target_offset = u->hunk_offset[hunk_num];
                    write_be32(rp, target_sec_id);
                    rp += 4;
                    while (count--) {
                        off = read_be32(rp);
                        patch_addr = read_be32(data + off) + target_offset;
                        write_be32(data + off, patch_addr);
                        rp += 4;
                    }
                }
            }

            /* HUNK_EXT XDEFs: add sec_base_offset to relocatable definitions */
            if (h->hunk_extern) {
                ext_p = h->hunk_extern;
                for (;;) {
                    value = read_be32(ext_p);
                    name_len_lw = value & 0x00FFFFFFUL;
                    ext_p += 4;
                    if (name_len_lw == 0)
                        break;
                    if (!(value & 0x80000000UL)) {
                        if ((value >> 24) == (unsigned long)EXT_DEF) {
                            patch = ext_p + name_len_lw * 4;
                            off = read_be32(patch) + my_offset;
                            write_be32(patch, off);
                        }
                        ext_p += name_len_lw * 4 + 4;
                    } else {
                        ext_p += name_len_lw * 4;
                        num_offsets = read_be32(ext_p);
                        ext_p += 4 + num_offsets * 4;
                    }
                }
            }

            /* RELOC16: PC-relative word; same section only */
            if (h->hunk_reloc16 && data) {
                long delta;
                rp = h->hunk_reloc16;
                for (;;) {
                    count = read_be32(rp);
                    rp += 4;
                    if (count == 0)
                        break;
                    hunk_num = read_be32(rp);
                    if (u->hunk_sec[hunk_num] != h->hunk_section)
                        break;
                    target_offset = u->hunk_offset[hunk_num];
                    rp += 4;
                    while (count--) {
                        off = read_be32(rp);
                        rp += 4;
                        delta = (long)target_offset - (long)my_offset - (long)off - 2;
                        delta += (short)((data[off] << 8) | (unsigned char)data[off + 1]);
                        data[off] = (unsigned char)((unsigned long)delta >> 8);
                        data[off + 1] = (unsigned char)delta;
                    }
                }
            }

            /* RELOC8: PC-relative byte; same section only */
            if (h->hunk_reloc8 && data) {
                long delta;
                rp = h->hunk_reloc8;
                for (;;) {
                    count = read_be32(rp);
                    rp += 4;
                    if (count == 0)
                        break;
                    hunk_num = read_be32(rp);
                    if (u->hunk_sec[hunk_num] != h->hunk_section)
                        break;
                    target_offset = u->hunk_offset[hunk_num];
                    rp += 4;
                    while (count--) {
                        off = read_be32(rp);
                        rp += 4;
                        delta = (long)target_offset - (long)my_offset - (long)off - 1;
                        delta += (signed char)data[off];
                        data[off] = (unsigned char)(delta & 0xFF);
                    }
                }
            }

            /* DREL32: add target section offset to long (data-relative) */
            if (h->hunk_reloc32d && data) {
                rp = h->hunk_reloc32d;
                for (;;) {
                    count = read_be32(rp);
                    rp += 4;
                    if (count == 0)
                        break;
                    hunk_num = read_be32(rp);
                    target_offset = u->hunk_offset[hunk_num];
                    write_be32(rp, u->hunk_sec[hunk_num]->section_id);
                    rp += 4;
                    while (count--) {
                        off = read_be32(rp);
                        patch_addr = read_be32(data + off) + target_offset;
                        write_be32(data + off, patch_addr);
                        rp += 4;
                    }
                }
            }

            /* DREL16: add target section offset to big-endian word (data-relative) */
            if (h->hunk_reloc16d && data) {
                unsigned long v16;
                rp = h->hunk_reloc16d;
                for (;;) {
                    count = read_be32(rp);
                    rp += 4;
                    if (count == 0)
                        break;
                    hunk_num = read_be32(rp);
                    target_sec_id = u->hunk_sec[hunk_num]->section_id;
                    target_offset = u->hunk_offset[hunk_num];
                    write_be32(rp, target_sec_id);
                    rp += 4;
                    while (count--) {
                        off = read_be32(rp);
                        rp += 4;
                        v16 = ((unsigned long)(unsigned char)data[off] << 8) |
                              (unsigned long)(unsigned char)data[off + 1];
                        v16 = (v16 + target_offset) & 0xFFFFUL;
                        data[off] = (unsigned char)(v16 >> 8);
                        data[off + 1] = (unsigned char)v16;
                    }
                }
            }

            /* DREL8: add target section offset to byte (data-relative) */
            if (h->hunk_reloc8d && data) {
                unsigned long v8;
                rp = h->hunk_reloc8d;
                for (;;) {
                    count = read_be32(rp);
                    rp += 4;
                    if (count == 0)
                        break;
                    hunk_num = read_be32(rp);
                    target_sec_id = u->hunk_sec[hunk_num]->section_id;
                    target_offset = u->hunk_offset[hunk_num];
                    write_be32(rp, target_sec_id);
                    rp += 4;
                    while (count--) {
                        off = read_be32(rp);
                        rp += 4;
                        v8 = ((unsigned long)(unsigned char)data[off]) + target_offset;
                        data[off] = (unsigned char)(v8 & 0xFFUL);
                    }
                }
            }
        }
        u = u->next;
    }

    /* Fill overlay stubs (JSR @ovlyMgr; DC.W index) and get stub base for overlay refs */
    stub_base = 0;
    if (ctx->overlay_stub_sec && ctx->overlay_stub_sec->hunk_head &&
        ctx->overlay_stub_sec->hunk_head->hunk_data) {
        ovlymgr_xdef = alink_find_xdef(ctx, (const unsigned char *)"@ovlyMgr", 2);
        if (!ovlymgr_xdef)
            ovlymgr_xdef = alink_find_xdef(ctx, (const unsigned char *)"_ovlyMgr", 2);
        if (ovlymgr_xdef) {
            manager_addr = xdef_value(ovlymgr_xdef);
            if (!xdef_is_abs(ovlymgr_xdef) && ovlymgr_xdef->xdef_sec && ovlymgr_xdef->xdef_sec->hunk_head)
                manager_addr += ovlymgr_xdef->xdef_sec->hunk_head->sec_base_offset;
            stub_base = ctx->overlay_stub_sec->hunk_head->sec_base_offset;
            stub_data = ctx->overlay_stub_sec->hunk_head->hunk_data;
            n_stubs = ctx->overlay_stub_sec->sec_size / 8;
            for (idx = 0; idx < n_stubs; idx++) {
                stub_data[idx * 8 + 0] = 0x4E;
                stub_data[idx * 8 + 1] = (unsigned char)0xB9;
                write_be32(stub_data + idx * 8 + 2, manager_addr);
                stub_data[idx * 8 + 6] = (unsigned char)(idx >> 8);
                stub_data[idx * 8 + 7] = (unsigned char)(idx & 0xFF);
            }
        }
    }

    /* XREF resolution: patch by ref type */
    xref = ctx->xref_list;
    while (xref) {
        xdef = xref->xref_xdef;
        if (xdef && xref->xref_hunk && xref->xref_hunk->hunk_data) {
            data = xref->xref_hunk->hunk_data;
            patch = xref->xref_ptr;
            name_len_lw = read_be32(patch) & 0x00FFFFFFUL;
            patch += 4 + name_len_lw * 4;
            num_offsets = read_be32(patch);
            patch += 4;
            if (xref->overlay_index >= 0 && stub_base != 0)
                value = stub_base + (unsigned long)xref->overlay_index * 8;
            else {
                value = xdef_value(xdef);
                if (!xdef_is_abs(xdef) && xdef->xdef_sec && xdef->xdef_sec->hunk_head)
                    value += xdef->xdef_sec->hunk_head->sec_base_offset;
            }
            for (off = 0; off < num_offsets; off++) {
                unsigned long addr = read_be32(patch + (unsigned long)off * 4);
                switch (xref->xref_type) {
                case EXT_REF32:
                    write_be32(data + addr, value);
                    break;
                case EXT_RELREF32:
                    write_be32(data + addr, value - addr - 4);
                    break;
                case EXT_REF16:
                case EXT_ABSREF16: {
                    unsigned long v16 = (xref->xref_type == EXT_ABSREF16)
                        ? (value & 0xFFFFUL)
                        : (value - addr - 2) & 0xFFFFUL;
                    data[addr] = (unsigned char)(v16 >> 8);
                    data[addr + 1] = (unsigned char)v16;
                    break;
                }
                case EXT_REF8:
                case EXT_ABSREF8: {
                    unsigned long v8 = (xref->xref_type == EXT_ABSREF8)
                        ? (value & 0xFFUL)
                        : (value - addr - 1) & 0xFFUL;
                    data[addr] = (unsigned char)v8;
                    break;
                }
                case EXT_DEXT32:
                case EXT_DEXT16:
                case EXT_DEXT8:
                    if (xdef->xdef_sec) {
                        unsigned long vo = xdef_value(xdef);
                        if (xref->xref_type == EXT_DEXT32) {
                            write_be32(data + addr, read_be32(data + addr) + vo);
                        } else if (xref->xref_type == EXT_DEXT16) {
                            value = read_be32(data + addr) & 0xFFFFUL;
                            value = (value + vo) & 0xFFFFUL;
                            data[addr] = (unsigned char)(value >> 8);
                            data[addr + 1] = (unsigned char)value;
                        } else {
                            value = (unsigned long)(unsigned char)data[addr];
                            data[addr] = (unsigned char)((value + vo) & 0xFFUL);
                        }
                    }
                    break;
                default:
                    write_be32(data + addr, value);
                    break;
                }
            }
        }
        xref = xref->next;
    }
}
