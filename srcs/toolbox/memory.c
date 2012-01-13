/*
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.
 */

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
    \defgroup alloc Memory allocation
    \{
        The \ref alloc module introduces a number of wrappers to basic memory
        management functions which allows to change the underlying memory
        allocator in a way that is transparent to the applications built on
        LibU.
        It is sufficient for an application to only use ::u_malloc/::u_free 
        and friends (which also include ::u_memdup, ::u_strndup and ::u_strdup 
        from the \ref misc module) when carrying out memory allocation and 
        deallocation operations.  
        In case it'd be needed to change the underlying memory management 
        facility with a custom one a call to ::u_memory_set_malloc and co. 
        would be enough to fix it up, provided that the new memory management 
        system has ISO C-like \c malloc(3) interfaces.
 */

/** \brief Set \c malloc(3) replacement */
void u_memory_set_malloc (void *(*f_malloc) (size_t))
{
    u_memory_fns.f_malloc = f_malloc;
}

/** \brief Set \c calloc(3) replacement */
void u_memory_set_calloc (void *(*f_calloc) (size_t, size_t))
{
    u_memory_fns.f_calloc = f_calloc;
}

/** \brief Set \c realloc(3) replacement */
void u_memory_set_realloc (void *(*f_realloc) (void *, size_t))
{
    u_memory_fns.f_realloc = f_realloc;
}

/** \brief Set \c free(3) replacement */
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
