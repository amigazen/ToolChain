/*
 * Portable a.out object format definitions (C89).
 * Matches BSD/Sun 68k a.out as produced by hunk2aout and Amiga GCC.
 * All multi-byte values are big-endian.
 */
#ifndef AOUT2HUNK_AOUT_H
#define AOUT2HUNK_AOUT_H

#define OMAGIC   0407
#define NMAGIC   0410

#define MID_SUN010  1
#define MID_SUN020  2

#define N_UNDF  0
#define N_ABS   1
#define N_TEXT  2
#define N_DATA  3
#define N_BSS   4
#define N_EXT   0x20

struct aout_exec {
    unsigned char a_midmag[4];
    unsigned char a_text[4];
    unsigned char a_data[4];
    unsigned char a_bss[4];
    unsigned char a_syms[4];
    unsigned char a_entry[4];
    unsigned char a_trsize[4];
    unsigned char a_drsize[4];
};

struct aout_nlist {
    unsigned char n_strx[4];
    unsigned char n_type;
    unsigned char n_other;
    unsigned char n_desc[2];
    unsigned char n_value[4];
};

struct aout_reloc {
    unsigned char r_address[4];
    unsigned char r_info[4];
};

#define AOUT_READ_BE32(p) \
    (((unsigned long)(unsigned char)(p)[0] << 24) | ((unsigned long)(unsigned char)(p)[1] << 16) | \
     ((unsigned long)(unsigned char)(p)[2] << 8) | (unsigned long)(unsigned char)(p)[3])

#define AOUT_GET_MAGIC(e)   (AOUT_READ_BE32((e).a_midmag) & 0xFFFF)
#define AOUT_GET_MID(e)     ((AOUT_READ_BE32((e).a_midmag) >> 16) & 0x3FF)
#define AOUT_GET_TEXT(e)    AOUT_READ_BE32((e).a_text)
#define AOUT_GET_DATA(e)    AOUT_READ_BE32((e).a_data)
#define AOUT_GET_BSS(e)     AOUT_READ_BE32((e).a_bss)
#define AOUT_GET_SYMS(e)    AOUT_READ_BE32((e).a_syms)
#define AOUT_GET_TRSIZE(e)  AOUT_READ_BE32((e).a_trsize)
#define AOUT_GET_DRSIZE(e)  AOUT_READ_BE32((e).a_drsize)

#define AOUT_NLIST_STRX(n)  AOUT_READ_BE32((n).n_strx)
#define AOUT_NLIST_TYPE(n)  ((unsigned char)(n).n_type)
#define AOUT_NLIST_VALUE(n) AOUT_READ_BE32((n).n_value)

#define AOUT_RELOC_ADDR(r)  AOUT_READ_BE32((r).r_address)
#define AOUT_RELOC_SYMNUM(r) (AOUT_READ_BE32((r).r_info) & 0x00FFFFFFUL)
#define AOUT_RELOC_PCREL(r) ((AOUT_READ_BE32((r).r_info) >> 24) & 1)
#define AOUT_RELOC_LENGTH(r) ((AOUT_READ_BE32((r).r_info) >> 25) & 3)
#define AOUT_RELOC_EXTERN(r) ((AOUT_READ_BE32((r).r_info) >> 27) & 1)
#define AOUT_RELOC_BASEREL(r) ((AOUT_READ_BE32((r).r_info) >> 28) & 1)

#endif
