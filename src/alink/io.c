/*
 * Big-endian read/write for Amiga hunk format.
 * C89, standard library only.
 */
#include "alink.h"

unsigned long read_be32(const unsigned char *p)
{
    return ((unsigned long)(unsigned char)p[0] << 24) |
           ((unsigned long)(unsigned char)p[1] << 16) |
           ((unsigned long)(unsigned char)p[2] << 8) |
           (unsigned long)(unsigned char)p[3];
}

void write_be32(unsigned char *p, unsigned long v)
{
    p[0] = (unsigned char)(v >> 24);
    p[1] = (unsigned char)(v >> 16);
    p[2] = (unsigned char)(v >> 8);
    p[3] = (unsigned char)v;
}

unsigned long read_be32_at(const unsigned char *base, unsigned long off)
{
    return read_be32(base + off);
}

int write_be32_file(FILE *f, unsigned long v)
{
    unsigned char buf[4];
    write_be32(buf, v);
    return fwrite(buf, 1, 4, f) == 4;
}
