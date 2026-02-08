/*
 * aout2hunk - Convert a.out object files to Amiga DOS hunk format.
 * C89, standard library only. Inverse of hunk2aout.
 *
 * Usage: aout2hunk <input.aout> [output.o]
 *   If output is omitted, writes to stdout or input with .o suffix.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aout.h"

/* Amiga hunk types (subset of doshunks.h) */
#define HUNK_UNIT    999
#define HUNK_NAME    1000
#define HUNK_CODE    1001
#define HUNK_DATA    1002
#define HUNK_BSS     1003
#define HUNK_RELOC32 1004
#define HUNK_RELOC16 1005
#define HUNK_RELOC8  1006
#define HUNK_EXT     1007
#define HUNK_END     1010

#define EXT_DEF    1
#define EXT_ABS    2
#define EXT_REF32  129
#define EXT_REF16  131
#define EXT_REF8   132

#define HUNK_EXEC_HEADER_SIZE 32

static void write_be32(FILE *f, unsigned long v)
{
    unsigned char b[4];
    b[0] = (unsigned char)(v >> 24);
    b[1] = (unsigned char)(v >> 16);
    b[2] = (unsigned char)(v >> 8);
    b[3] = (unsigned char)v;
    fwrite(b, 1, 4, f);
}

/* Write EXT entry: type|namelen_lw, name (padded), value; or for refs: type|namelen, name, num_offsets, offset... */
static int write_ext_def(FILE *f, unsigned char type, const char *name, unsigned long value)
{
    unsigned long len;
    unsigned long len_lw;
    unsigned long i;

    len = strlen(name);
    len_lw = (len + 4) / 4;
    if (len_lw > 0xFFFFFFUL)
        return -1;
    write_be32(f, (unsigned long)type << 24 | len_lw);
    fwrite(name, 1, len, f);
    for (i = (len_lw * 4) - len; i > 0; i--)
        fputc(0, f);
    write_be32(f, value);
    return 0;
}

static int write_ext_ref(FILE *f, unsigned char type, const char *name, unsigned long *offsets, int n_offsets)
{
    unsigned long len;
    unsigned long len_lw;
    unsigned long i;
    int j;

    len = strlen(name);
    len_lw = (len + 4) / 4;
    if (len_lw > 0xFFFFFFUL)
        return -1;
    write_be32(f, (unsigned long)type << 24 | len_lw);
    fwrite(name, 1, len, f);
    for (i = (len_lw * 4) - len; i > 0; i--)
        fputc(0, f);
    write_be32(f, (unsigned long)n_offsets);
    for (j = 0; j < n_offsets; j++)
        write_be32(f, offsets[j]);
    return 0;
}

/* Map a.out segment (N_TEXT=2, N_DATA=3, N_BSS=4) to hunk index (0, 1, 2) */
static int seg_to_hunk(int seg)
{
    if (seg == N_TEXT)
        return 0;
    if (seg == N_DATA)
        return 1;
    if (seg == N_BSS)
        return 2;
    return -1;
}

static int convert(const unsigned char *buf, unsigned long size, FILE *out)
{
    struct aout_exec exec;
    unsigned long a_text;
    unsigned long a_data;
    unsigned long a_bss;
    unsigned long a_syms;
    unsigned long a_trsize;
    unsigned long a_drsize;
    unsigned long off_text;
    unsigned long off_data;
    unsigned long off_treloc;
    unsigned long off_dreloc;
    unsigned long off_sym;
    unsigned long off_str;
    unsigned long strtab_len;
    const unsigned char *text_ptr;
    const unsigned char *data_ptr;
    const unsigned char *str_ptr;
    unsigned int n_treloc;
    unsigned int n_dreloc;
    unsigned int n_syms;
    unsigned int i;
    unsigned long len_lw;
    unsigned long reloc_count;
    unsigned long reloc_buf[1024];
    int reloc_idx;
    int to_hunk;
    unsigned long addr;
    const struct aout_nlist *nl;
    unsigned int ntype;
    const char *sym_name;
    unsigned long sym_value;
    int j;
    unsigned long *ext_offsets;
    int next_off;

    if (size < HUNK_EXEC_HEADER_SIZE) {
        fprintf(stderr, "aout2hunk: file too small\n");
        return -1;
    }
    memcpy(&exec, buf, sizeof(exec));
    if (AOUT_GET_MAGIC(exec) != OMAGIC && AOUT_GET_MAGIC(exec) != NMAGIC) {
        fprintf(stderr, "aout2hunk: not an OMAGIC/NMAGIC a.out file\n");
        return -1;
    }
    a_text = AOUT_GET_TEXT(exec);
    a_data = AOUT_GET_DATA(exec);
    a_bss = AOUT_GET_BSS(exec);
    a_syms = AOUT_GET_SYMS(exec);
    a_trsize = AOUT_GET_TRSIZE(exec);
    a_drsize = AOUT_GET_DRSIZE(exec);

    off_text = HUNK_EXEC_HEADER_SIZE;
    off_data = off_text + a_text;
    off_treloc = off_data + a_data;
    off_dreloc = off_treloc + a_trsize;
    off_sym = off_dreloc + a_drsize;
    off_str = off_sym + a_syms;
    if (off_str > size) {
        fprintf(stderr, "aout2hunk: file truncated\n");
        return -1;
    }
    strtab_len = (size >= off_str + 4) ? AOUT_READ_BE32(buf + off_str) : 0;
    if (off_str + 4 + strtab_len > size)
        strtab_len = size - off_str - 4;

    text_ptr = buf + off_text;
    data_ptr = buf + off_data;
    str_ptr = buf + off_str + 4;
    n_treloc = (unsigned int)(a_trsize / 8);
    n_dreloc = (unsigned int)(a_drsize / 8);
    n_syms = (unsigned int)(a_syms / 12);

    /* HUNK_UNIT */
    write_be32(out, HUNK_UNIT);
    write_be32(out, 0);
    /* HUNK_NAME (empty) */
    write_be32(out, HUNK_NAME);
    write_be32(out, 0);
    /* HUNK_CODE */
    write_be32(out, HUNK_CODE);
    len_lw = (a_text + 3) / 4;
    write_be32(out, len_lw);
    if (a_text)
        fwrite(text_ptr, 1, a_text, out);
    for (i = (len_lw * 4) - a_text; i > 0; i--)
        fputc(0, out);

    /* RELOC32 for code: only local relocs, group by target hunk (0, 1, 2) */
    for (to_hunk = 0; to_hunk <= 2; to_hunk++) {
        reloc_count = 0;
        reloc_idx = 0;
        for (i = 0; i < n_treloc && reloc_idx < 1024; i++) {
            struct aout_reloc r;
            memcpy(&r, buf + off_treloc + i * 8, 8);
            if (AOUT_RELOC_EXTERN(r))
                continue;
            if (seg_to_hunk((int)AOUT_RELOC_SYMNUM(r)) != to_hunk)
                continue;
            addr = AOUT_RELOC_ADDR(r);
            reloc_buf[reloc_idx++] = addr;
            reloc_count++;
        }
        if (reloc_count > 0) {
            write_be32(out, HUNK_RELOC32);
            write_be32(out, reloc_count);
            write_be32(out, (unsigned long)to_hunk);
            for (i = 0; i < reloc_count; i++)
                write_be32(out, reloc_buf[i]);
        }
    }

    /* HUNK_DATA */
    write_be32(out, HUNK_DATA);
    len_lw = (a_data + 3) / 4;
    write_be32(out, len_lw);
    if (a_data)
        fwrite(data_ptr, 1, a_data, out);
    for (i = (len_lw * 4) - a_data; i > 0; i--)
        fputc(0, out);

    /* RELOC32 for data: only local relocs */
    for (to_hunk = 0; to_hunk <= 2; to_hunk++) {
        reloc_count = 0;
        reloc_idx = 0;
        for (i = 0; i < n_dreloc && reloc_idx < 1024; i++) {
            struct aout_reloc r;
            memcpy(&r, buf + off_dreloc + i * 8, 8);
            if (AOUT_RELOC_EXTERN(r))
                continue;
            if (seg_to_hunk((int)AOUT_RELOC_SYMNUM(r)) != to_hunk)
                continue;
            addr = AOUT_RELOC_ADDR(r);
            reloc_buf[reloc_idx++] = addr;
            reloc_count++;
        }
        if (reloc_count > 0) {
            write_be32(out, HUNK_RELOC32);
            write_be32(out, reloc_count);
            write_be32(out, (unsigned long)to_hunk);
            for (i = 0; i < reloc_count; i++)
                write_be32(out, reloc_buf[i]);
        }
    }

    /* HUNK_BSS */
    write_be32(out, HUNK_BSS);
    len_lw = (a_bss + 3) / 4;
    write_be32(out, len_lw);

    /* HUNK_EXT: definitions first, then references (collect ref offsets per symbol) */
    ext_offsets = (unsigned long *)malloc(4096 * sizeof(unsigned long));
    if (!ext_offsets) {
        fprintf(stderr, "aout2hunk: out of memory\n");
        return -1;
    }
    for (i = 0; i < n_syms; i++) {
        nl = (const struct aout_nlist *)(buf + off_sym + i * 12);
        ntype = AOUT_NLIST_TYPE(*nl) & (N_EXT | 0x1F);
        sym_value = AOUT_NLIST_STRX(*nl);
        if (sym_value >= strtab_len)
            continue;
        sym_name = (const char *)str_ptr + sym_value;
        if (sym_name[0] == '\0')
            continue;
        if (strcmp(sym_name, "__chip_data") == 0 || strcmp(sym_name, "__chip_bss") == 0)
            continue;
        if ((ntype & 0x1F) == N_UNDF && (ntype & N_EXT)) {
            next_off = 0;
            for (j = 0; j < (int)n_treloc && next_off < 4096; j++) {
                struct aout_reloc r;
                memcpy(&r, buf + off_treloc + j * 8, 8);
                if (!AOUT_RELOC_EXTERN(r) || AOUT_RELOC_SYMNUM(r) != i)
                    continue;
                ext_offsets[next_off++] = AOUT_RELOC_ADDR(r);
            }
            for (j = 0; j < (int)n_dreloc && next_off < 4096; j++) {
                struct aout_reloc r;
                memcpy(&r, buf + off_dreloc + j * 8, 8);
                if (!AOUT_RELOC_EXTERN(r) || AOUT_RELOC_SYMNUM(r) != i)
                    continue;
                ext_offsets[next_off++] = AOUT_RELOC_ADDR(r);
            }
            if (next_off > 0) {
                write_be32(out, HUNK_EXT);
                write_ext_ref(out, (unsigned char)EXT_REF32, sym_name, ext_offsets, next_off);
            }
        } else if ((ntype & N_EXT) && (ntype & 0x1F) == N_ABS) {
            write_be32(out, HUNK_EXT);
            write_ext_def(out, EXT_ABS, sym_name, AOUT_NLIST_VALUE(*nl));
        } else if ((ntype & N_EXT) && ((ntype & 0x1F) == N_TEXT || (ntype & 0x1F) == N_DATA || (ntype & 0x1F) == N_BSS)) {
            sym_value = AOUT_NLIST_VALUE(*nl);
            if ((ntype & 0x1F) == N_TEXT)
                ;
            else if ((ntype & 0x1F) == N_DATA)
                sym_value -= a_text;
            else
                sym_value -= a_text + a_data;
            write_be32(out, HUNK_EXT);
            write_ext_def(out, EXT_DEF, sym_name, sym_value);
        }
    }
    free(ext_offsets);
    write_be32(out, 0);
    write_be32(out, HUNK_END);
    return 0;
}

int main(int argc, char **argv)
{
    FILE *in;
    FILE *out;
    unsigned char *buf;
    unsigned long size;
    char *out_name;
    size_t nread;
    int ret;

    if (argc < 2) {
        fprintf(stderr, "usage: aout2hunk <input.aout> [output.o]\n");
        return 1;
    }
    in = fopen(argv[1], "rb");
    if (!in) {
        fprintf(stderr, "aout2hunk: cannot open '%s'\n", argv[1]);
        return 1;
    }
    fseek(in, 0, SEEK_END);
    size = (unsigned long)ftell(in);
    fseek(in, 0, SEEK_SET);
    buf = (unsigned char *)malloc(size);
    if (!buf) {
        fprintf(stderr, "aout2hunk: out of memory\n");
        fclose(in);
        return 1;
    }
    nread = fread(buf, 1, (size_t)size, in);
    fclose(in);
    if (nread != (size_t)size) {
        fprintf(stderr, "aout2hunk: read error\n");
        free(buf);
        return 1;
    }
    if (argc >= 3)
        out_name = argv[2];
    else {
        char *dot;
        out_name = (char *)malloc(strlen(argv[1]) + 4);
        if (!out_name) {
            free(buf);
            return 1;
        }
        strcpy(out_name, argv[1]);
        dot = strrchr(out_name, '.');
        if (dot && dot != out_name)
            *dot = '\0';
        strcat(out_name, ".o");
    }
    out = fopen(out_name, "wb");
    if (!out) {
        fprintf(stderr, "aout2hunk: cannot create '%s'\n", out_name);
        free(buf);
        if (argc < 3)
            free(out_name);
        return 1;
    }
    ret = convert(buf, size, out);
    fclose(out);
    free(buf);
    if (argc < 3)
        free(out_name);
    return ret != 0 ? 1 : 0;
}
