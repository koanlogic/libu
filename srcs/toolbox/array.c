/* 
 * Copyright (c) 2005-2008 by KoanLogic s.r.l. - All rights reserved.  
 */

#include <u/libu_conf.h>
#include <u/libu.h>
#include <toolbox/array.h>

typedef struct __slot_s { void *ptr; } __slot_t;

struct u_array_s
{
    __slot_t *base;
    size_t sz, nelem, top;
};

/**
 *  \defgroup array Dynamic Arrays
 *  \{
 */

/**
 *  \brief  Create a new array object
 *
 *  \param sz   the initial number of slots to be created  (set it to \c 0 if 
 *              you don't want to specify an initial array size)
 *  \param pa   the newly created array object as a result argument
 *
 *  \return \c 0 on success, \c ~0 on error
 */
int u_array_create (size_t sz, u_array_t **pa)
{
    u_array_t *a = NULL;

    dbg_return_if (pa == NULL, ~0); 

    a = u_zalloc(sizeof(u_array_t));
    warn_err_sif (a == NULL);

    /* if we've been requested to allocate some slot, do it now */
    if ((a->sz = sz) != 0)
        warn_err_sif ((a->base = u_zalloc(sz * sizeof(__slot_t))) == NULL);

    a->nelem = 0;

    *pa = a;
    
    return 0;
err:
    return ~0;
}

/**
 *  \brief  Free the array object: the array does not own the pointers in it,
 *          the client must free them explicitly
 *
 *  \param a    the array object that has to be disposed
 *
 *  \return nothing 
 */
void u_array_free (u_array_t *a)
{
    dbg_return_if (a == NULL, );

    U_FREE(a->base);
    u_free(a);

    return;
}

/**
 *  \brief  Add the supplied number of slots to the array
 *
 *  \param a    the array object
 *  \param more the number of elements that shall be added. 
 *              If \c U_ARRAY_GROW_AUTO is supplied, the default resize
 *              strategy is used (i.e. size is doubled)
 *
 *  \return \c 0 on success, \c ~0 on error
 */
int u_array_grow (u_array_t *a, size_t more)
{
    size_t new_sz;
    __slot_t *new_base;

    dbg_return_if (a == NULL, ~0);

    /* auto resize strategy is to double the current size */
    new_sz = (more == U_ARRAY_GROW_AUTO) ? a->sz * 2 : a->sz + more;

    /* if realloc fails the memory at a->base is still valid */
    new_base = u_realloc(a->base, new_sz * sizeof(__slot_t));
    warn_err_sif (new_base == NULL);

    a->base = new_base;
    a->sz = new_sz;

    return 0; 
err:
    return ~0;
}

/**
 *  \brief  Push an element on top of the array
 *
 *  \param a    the array object
 *  \param elem the element that has to be push'd
 *
 *  \return \c 0 on success, \c ~0 on error
 */
int u_array_push (u_array_t *a, void *elem)
{
    __slot_t *s;

    dbg_return_if (a == NULL, ~0);

    /* if no space is available, grow the array by the default factor */
    if (a->nelem == a->sz)
        warn_err_if (u_array_grow(a, U_ARRAY_GROW_AUTO));
    
    /* stick the supplied element on top and increment counters */
    s = a->base + a->top;
    s->ptr = elem;
    a->nelem += 1;
    a->top += 1;

    return 0;
err:
    return ~0;
}

/**
 *  \brief  Get an element at a given slot
 *
 *  \param a    the array object
 *  \param idx  the index of the element that should be retrieved
 *
 *  \return the pointer to the n-th element or \c NULL if no n-th element has
 *          been found
 */
void *u_array_get_n (u_array_t *a, size_t idx)
{
    __slot_t *s;

    dbg_return_if (a == NULL, NULL);

    warn_err_ifm (idx >= a->sz,
            "trying to get an element out of array boundaries");

    s = a->base + idx;

    return s->ptr;
err:
    return NULL;
}

/**
 *  \brief  Set an element at a given slot
 *
 *  \param a    the array object
 *  \param elem the element that shall be inserted
 *  \param idx  the index at which \p elem is to be inserted
 *
 *  \return \c 0 on success, \c ~0 on error
 */
int u_array_set_n (u_array_t *a, void *elem, size_t idx)
{
    __slot_t *s;

    dbg_return_if (a == NULL, ~0);

    warn_err_ifm (idx >= a->sz, 
            "trying to set an element past array boundaries");

    s = a->base + idx;

    /* catch override and wail */
    if (s->ptr != NULL)
        warn("overriding element (%p) at %p[%d]", s->ptr, a->base, idx);

    s->ptr = elem;
    a->nelem += 1;
    if (idx > a->top)   /* set 'top' indicator if needed */
       a->top = idx; 

    return 0;
err:
    return ~0;
}

/**
 *  \brief  Count elements in array
 *
 *  \param a    the array object
 *
 *  \return the number of elements in \p a
 */
size_t u_array_count (u_array_t *a)
{
    dbg_return_if (a == NULL, 0);
    return a->nelem;
}

/**
 *  \brief  Count the number of available slots in array
 *
 *  \param a    the array object
 *
 *  \return the number of available elements in \p a
 */
size_t u_array_avail (u_array_t *a)
{
    dbg_return_if (a == NULL, 0);
    return (a->sz - a->nelem);
}

/**
 *  \brief  Get the total number of slots (used and/or available) in array
 *
 *  \param a    the array object
 *
 *  \return the total number of slots in \p a
 */
size_t u_array_size (u_array_t *a)
{
    dbg_return_if (a == NULL, 0);
    return a->sz;
}

/**
 *  \brief  Get the index of the top element in the array
 *
 *  \param a    the array object
 *
 *  \return the total number of slots in \p a
 */
size_t u_array_top (u_array_t *a)
{
    dbg_return_if (a == NULL, 0);
    return a->top;
}

/**
 *  \}
 */

/* debug only */
void u_array_print (u_array_t *a)
{
    size_t j;

    dbg_return_if (a == NULL, );

    con("a(%p)", a);
    for (j = 0; j < u_array_size(a); ++j)
        con("   %p[%zu] = %p", a->base + j, j, u_array_get_n(a, j));

    return;
}
