/*
 * Copyright (c) 2005-2007 by KoanLogic s.r.l. - All rights reserved.
 */

static const char rcsid[] =
    "$Id: memory.c,v 1.3 2007/02/12 08:32:27 tho Exp $";

#include <stdlib.h>
#include <toolbox/memory.h>

typedef struct u_memory_funcs_s
{
    void *(*f_malloc) (size_t);
    void *(*f_calloc) (size_t, size_t);
    void *(*f_realloc) (void *, size_t);
    void (*f_free) (void *);
} u_memory_fns_t;

/* defaults to LibC memory allocator */
static u_memory_fns_t u_memory_fns = { malloc, calloc, realloc, free };

/**
 *  \defgroup alloc Memory
 *  \{
 */

/** \brief Set malloc(3) replacement */
void u_memory_set_malloc (void *(*f_malloc) (size_t))
{
    u_memory_fns.f_malloc = f_malloc;
}

/** \brief Set calloc(3) replacement */
void u_memory_set_calloc (void *(*f_calloc) (size_t, size_t))
{
    u_memory_fns.f_calloc = f_calloc;
}

/** \brief Set realloc(3) replacement */
void u_memory_set_realloc (void *(*f_realloc) (void *, size_t))
{
    u_memory_fns.f_realloc = f_realloc;
}

/** \brief Set free(3) replacement */
void u_memory_set_free (void (*f_free) (void *))
{
    u_memory_fns.f_free = f_free;
}

/** \brief Wrapper for malloc-like function */
void *u_malloc (size_t sz)
{
    return u_memory_fns.f_malloc(sz);
}

/** \brief Wrapper for calloc-like function */
void *u_calloc (size_t cnt, size_t sz)
{
    return u_memory_fns.f_calloc(cnt, sz);
}

/** \brief Alloc a contiguous region of \p sz bytes and zero-fill it */
void *u_zalloc (size_t sz)
{
    return u_memory_fns.f_calloc(1, sz);
}

/** \brief Wrapper for realloc-like function */
void *u_realloc (void *ptr, size_t sz)
{
    return u_memory_fns.f_realloc(ptr, sz);
}

/** \brief Wrapper for free-like function, sanity checks the supplied pointer */
void u_free (void *ptr)
{
    if (ptr)
        u_memory_fns.f_free(ptr);
}

/**
 *      \}
 */
