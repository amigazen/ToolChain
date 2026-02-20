/*
 * alink - Amiga DOS object linker (C89, standard library only).
 * Internal types and context.
 */
#ifndef ALINK_ALINK_H
#define ALINK_ALINK_H

#include <stdio.h>
#include "doshunks.h"

#define ALINK_SECNAMESZ  240
#define ALINK_DEF_HASHTAB 4096
#define ALINK_DEF_HUNKS_PER_UNIT 16
#define ALINK_PHX_MAGIC  0x50485821UL  /* 'PHX!' */

struct Section {
    struct Section *next;
    unsigned long type;
    unsigned long section_id;
    unsigned long sec_size;
    unsigned long last_reloc32;
    struct Hunk *hunk_head;
    struct Hunk *hunk_tail;
    char section_name[ALINK_SECNAMESZ];
};

struct Hunk {
    struct Hunk *next;
    unsigned char *hunk_data;
    unsigned long data_size;
    struct Section *hunk_section;
    unsigned char *hunk_reloc8;
    unsigned char *hunk_reloc16;
    unsigned char *hunk_reloc32;
    unsigned char *hunk_reloc32d;
    unsigned char *hunk_reloc16d;
    unsigned char *hunk_reloc8d;
    unsigned char *hunk_near_reloc;
    unsigned char *hunk_extern;
    unsigned char *hunk_symbol;
    unsigned char *hunk_debug;
    short num_xrefs;
    unsigned char *xref_relocs;
    struct Unit *unit_struct;
    unsigned long sec_base_offset;
};

struct Unit {
    struct Unit *next;
    int select;
    int num_hunks;
    struct Hunk **hunk_ptr;
    struct Section **hunk_sec;
    unsigned long *hunk_offset;
    const char *obj_name;
    char *name;
    char name_buffer[ALINK_SECNAMESZ];
    int overlay_node;  /* -1 = root, 0.. = overlay node index */
};

/* One overlay node in the tree (level, ordinate, file list, section lists). */
struct OverlayNode {
    int level;
    int ordinate;
    int parent_index;
    char **files;
    int nfiles;
    struct Section *code_sections;
    struct Section *data_sections;
    struct Section *bss_sections;
    unsigned long first_hunk;
    unsigned long num_hunks;
};

struct XDEF {
    struct XDEF *next;
    struct Unit *xdef_unit;
    struct Section *xdef_sec;
    unsigned char *xdef_ptr;
};

struct XREF {
    struct XREF *next;
    struct XDEF *xref_xdef;
    struct Hunk *xref_hunk;
    unsigned char *xref_ptr;
    unsigned char xref_type;  /* EXT_REF32, EXT_REF16, etc. */
    int overlay_index;       /* -1 = normal ref; 0..n-1 = overlay table index for stub */
};

/* Cross-section RELOC16/8: ref patched to JMP.L stub; stub filled in correction. */
struct JumpStubRef {
    struct JumpStubRef *next;
    struct Hunk *ref_hunk;
    unsigned long ref_off;
    int target_hunk_num;
    int is_reloc16;       /* 1 = RELOC16, 0 = RELOC8 */
    struct Hunk *stub_hunk;
    unsigned long stub_off;
};

struct ObjectFile {
    struct ObjectFile *next;
    unsigned long size;
    unsigned char *data;
    char *path;
};

/* DEFINE symbol=value from command line (slink/blink) */
struct LinkerDefine {
    struct LinkerDefine *next;
    char *name;
    unsigned long value;
};

struct LinkerContext {
    char sec_name[ALINK_SECNAMESZ];
    char out_name[256];
    int small_code;
    int small_data;
    int no_debug;
    int rem_empty;
    int return_code;
    int lib_unit;
    int chip_opt;
    int fast_opt;
    int res_opt;
    int frag_opt;
    int verbose;
    int quiet;
    const char *verify_file;
    FILE *msg_file;
    const char *map_file;
    char map_opts[16];
    const char *xref_file;
    unsigned long bufsize_val;
    unsigned long maxhunk_val;
    int prelink;
    int addsym;
    int noalvs;
    int batch;
    int noicons;
    struct LinkerDefine *define_list;
    int map_plain;
    int map_fancy;
    int map_hwidth;
    int map_height;
    int map_width;
    int map_fwidth;
    int map_pwidth;
    int map_swidth;
    const char *ovlymgr_file;
    int overlay_mode;           /* 1 if OVERLAY was seen */
    struct OverlayNode *overlay_nodes;
    int n_overlay_nodes;
    unsigned long overlay_table_size;  /* max hunks in memory (root + one branch) */
    const char *libfd_file;
    const char *libprefix_str;
    const char *libid_str;
    int librevision;
    int libversion;
    int fndatam;
    int newocv;
    const char *obj_name;
    struct ObjectFile *objects;
    struct Section *code_sections;
    struct Section *data_sections;
    struct Section *bss_sections;
    struct Section *merged_data;
    struct Section *merged_bss;
    struct Section *data_bas_sec;  /* section used for __DATA_BAS_ */
    unsigned long data_len_lw;     /* data size in longwords for __DATA_LEN */
    unsigned long bss_len_lw;      /* bss size in longwords for __BSS_LEN */
    struct Unit *units;
    struct Unit *active_unit;
    unsigned short id_cnt;
    unsigned short hunk_cnt;
    unsigned short hash_tab_mask;
    struct XDEF **xdef_hash_tab;
    struct XREF *xref_list;
    struct XREF *xref_list_tail;
    struct Hunk **unit_info_hunk;
    struct Section **unit_info_sec;
    unsigned long hash_tab_size;
    unsigned short hunks_per_unit;
    FILE *out_file;
    void *alloc_list;
    struct Section *overlay_stub_sec;  /* root code section holding JSR @ovlyMgr + DC.W index stubs */
    struct JumpStubRef *jump_stub_refs;   /* cross-section RELOC16/8: ref -> stub mapping */
    struct JumpStubRef *jump_stub_refs_tail;
};

void alink_init(struct LinkerContext *ctx);
void alink_clear(struct LinkerContext *ctx);
int alink_link(struct LinkerContext *ctx, int argc, char **argv);

int read_units(struct LinkerContext *ctx, char **paths, const int *node_indices, int npaths);
int add_linker_xdefs(struct LinkerContext *ctx);
void add_linker_xdef(struct LinkerContext *ctx, const char *name, unsigned int name_len_lw,
    unsigned char type, unsigned long value);
void select_units(struct LinkerContext *ctx);
struct XDEF *alink_find_xdef(struct LinkerContext *ctx, const unsigned char *name_ptr,
    unsigned int name_len_lw);
unsigned long xdef_value(struct XDEF *xdef);
unsigned char xdef_type(struct XDEF *xdef);
void set_xdef_value(struct XDEF *xdef, unsigned long value);
void kill_unselected(struct LinkerContext *ctx);
void kill_zero_sections(struct LinkerContext *ctx);
struct Section *create_section(struct LinkerContext *ctx, unsigned long type, const char *name);
struct Section *create_section_overlay(struct LinkerContext *ctx, unsigned long type, const char *name, int node_index);
void combine_bss_onto_data(struct LinkerContext *ctx);
void prepare_overlay_stubs(struct LinkerContext *ctx);
void prepare_jump_stubs(struct LinkerContext *ctx);
void set_section_ids(struct LinkerContext *ctx);
void calc_hunk_offsets(struct LinkerContext *ctx);
void resolve_common(struct LinkerContext *ctx);
void set_lnk_xdef(struct LinkerContext *ctx);
void correction(struct LinkerContext *ctx);
int write_load_file(struct LinkerContext *ctx);
int write_map_file(struct LinkerContext *ctx);
int write_xref_file(struct LinkerContext *ctx);

void *alink_alloc(struct LinkerContext *ctx, unsigned long size);
void *alink_alloc_clear(struct LinkerContext *ctx, unsigned long size);
void alink_free_all(struct LinkerContext *ctx);

unsigned long read_be32(const unsigned char *p);
unsigned long read_be32_at(const unsigned char *base, unsigned long off);
void write_be32(unsigned char *p, unsigned long v);
int write_be32_file(FILE *f, unsigned long v);

#endif
