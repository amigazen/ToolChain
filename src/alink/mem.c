/*
 * Allocation wrapper for alink: all blocks are tracked and freed in alink_clear.
 * C89, standard library only.
 */
#include <stdlib.h>
#include "alink.h"

struct alloc_node {
    void *ptr;
    struct alloc_node *next;
};

void *alink_alloc(struct LinkerContext *ctx, unsigned long size)
{
    struct alloc_node *node;
    void *ptr;

    ptr = malloc((size_t)size);
    if (!ptr)
        return (void *)0;
    node = (struct alloc_node *)malloc(sizeof(struct alloc_node));
    if (!node) {
        free(ptr);
        return (void *)0;
    }
    node->ptr = ptr;
    node->next = (struct alloc_node *)ctx->alloc_list;
    ctx->alloc_list = node;
    return ptr;
}

void *alink_alloc_clear(struct LinkerContext *ctx, unsigned long size)
{
    unsigned long i;
    unsigned char *p;

    p = (unsigned char *)alink_alloc(ctx, size);
    if (!p)
        return (void *)0;
    for (i = 0; i < size; i++)
        p[i] = 0;
    return (void *)p;
}

void alink_free_all(struct LinkerContext *ctx)
{
    struct alloc_node *node;
    struct alloc_node *next;

    node = (struct alloc_node *)ctx->alloc_list;
    while (node) {
        next = node->next;
        if (node->ptr)
            free(node->ptr);
        free(node);
        node = next;
    }
    ctx->alloc_list = (void *)0;
}
