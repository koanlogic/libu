/*
 * Copyright (c) 2005, KoanLogic s.r.l. - All rights reserved.
 */

static const char rcsid[] =
    "$Id: memory.c,v 1.1 2005/09/23 16:10:32 tho Exp $";

#include <stdlib.h>

#include <u/memory.h>

/**
 *  \defgroup alloc Memory
 *  \{
 */

/** \brief Wrapper for malloc(3) */
void *u_malloc (size_t sz)
{
    return malloc(sz);
}

/** \brief Wrapper for calloc(3) */
void *u_calloc (size_t cnt, size_t sz)
{
    return calloc(cnt, sz);
}

/** \brief Alloc a contiguous region of \p sz bytes and zero-fill it */
void *u_zalloc (size_t sz)
{
    return calloc(1, sz);
}

/** \brief Wrapper for realloc(3) */
void *u_realloc (void *ptr, size_t sz)
{
    return realloc(ptr, sz);
}

/** \brief Wrapper for free(3), sanity checks the supplied pointer */
void u_free (void *ptr)
{
    if (ptr)
        free(ptr);
}

/**
 *      \}
 */
