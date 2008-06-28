/* 
 * Copyright (c) 2005-2008 by KoanLogic s.r.l. - All rights reserved.  
 */

#include <u/libu_conf.h>
#include <u/libu.h>
#include <toolbox/array.h>

typedef struct __slot_s { int set; void *data; } __slot_t;

struct u_array_s
{
    __slot_t *base;
    size_t nslot;               /* total number of slots (0..n=N) */
    size_t nelem;               /* number of busy slots (0..N)  */
    size_t top;                 /* top element index: (0..N-1)  */
    void (*cb_free)(void *);    /* optional free function for hosted elements */
};

/**
 *  \defgroup array Dynamic Arrays
 *  \{
 */

/**
 *  \brief  Create a new array object
 *
 *  \param nslot    the initial number of slots to be created  (set it to \c 0 
 *                  if you don't want to specify an initial array size)
 *  \param pa       the newly created array object as a result argument
 *
 *  \return \c 0 on success, \c ~0 on error
 */
int u_array_create (size_t nslot, u_array_t **pa)
{
    u_array_t *a = NULL;

    dbg_return_if (pa == NULL, ~0);

    a = u_zalloc(sizeof(u_array_t));
    warn_err_sif (a == NULL);

    /* if we've been requested to allocate some slot, do it now */
    if ((a->nslot = nslot) != 0)
        warn_err_sif ((a->base = u_zalloc(nslot * sizeof(__slot_t))) == NULL);

    a->nelem = 0;
    a->top = 0;
    a->cb_free = NULL;
 
    *pa = a;
    
    return 0;
err:
    return ~0;
}

/**
 *  \brief  Set an element at a given slot
 *
 *  \param a        the array object
 *  \param idx      the index at which \p elem is to be inserted
 *  \param elem     the element that shall be inserted
 *  \param oelem    contains a reference to the replaced element in case the
 *                  assigned slot were already in use (set it to \c NULL if 
 *                  you're sure that \p idx was empty)
 *
 *  \return \c 0 on success, \c ~0 on error
 */
int u_array_set_n (u_array_t *a, size_t idx, void *elem, void **oelem)
{
    __slot_t *s;
    size_t more;

    dbg_return_if (a == NULL, ~0);

    if (idx >= a->nslot)
    {
        /* next line assumes auto resize strategy is doubling the  
         * current number of slots */ 
        more = (idx >= a->nslot * 2) ? idx - a->nslot + 1 : U_ARRAY_GROW_AUTO ;
        warn_err_if (u_array_grow(a, more));
    }

    /* get slot location */
    s = a->base + idx;

    /* test if slot is busy, in which case the old value is returned via
     * 'oelem' */
    if (s->set)
    {
        if (oelem)
            *oelem = s->data;
        else
            warn("%p definitely lost at a[i]=%p[%zu]", s->data, a, idx);
    }

    /* copyin data (scalar - MUST fit sizeof(void*) ! - or pointer) */
    s->data = elem;
    s->set = 1;

    a->nelem += 1;
    /* adjust 'top' indicator if needed */
    a->top = (idx > a->top) ? idx : a->top;

    return 0;
err:
    return ~0;
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
    __slot_t *new_base;
    size_t new_nslot;

    dbg_return_if (a == NULL, ~0);

    /* auto resize strategy is to double the current size */
    /* XXX if the resize strategy is changed, also change the "auto-grow"
     * XXX branch in u_array_set_n (circa line 81) */
    new_nslot = (more == U_ARRAY_GROW_AUTO) ? a->nslot * 2 : a->nslot + more;

    /* NOTE: if realloc fails the memory at a->base is still valid */
    new_base = u_realloc(a->base, new_nslot * sizeof(__slot_t));
    warn_err_sif (new_base == NULL);

    /* zero-ize newly alloc'd memory (needed because of s->set tests for
     * catching overrides) */
    memset(new_base + a->nslot, 0, (new_nslot - a->nslot) * sizeof(__slot_t));

    a->base = new_base;
    a->nslot = new_nslot;

    return 0; 
err:
    return ~0;
} 

/**
 *  \brief  Get an element at a given slot
 *
 *  \param a        array object
 *  \param idx      index of the element that should be retrieved
 *  \param pelem    reference to the retrieved element
 *
 *  \return \c 0 on success (element was found), \c ~0 on failure (element not 
 *          set or bad parameter supplied)
 */
int u_array_get_n (u_array_t *a, size_t idx, void **pelem)
{
    __slot_t *s;

    dbg_return_if (a == NULL, ~0);
    dbg_return_if (pelem == NULL, ~0);
    dbg_return_if (idx >= a->nslot, ~0);

    s = a->base + idx;

    if (!s->set)
        return ~0;

    *pelem = s->data;

    return 0;
}

/**
 *  \brief  Set a free(3)-like function to delete array elements 
 *
 *  \param  a       an array object
 *  \param  cb_free the free-like function
 *
 *  \return \c 0 on success, \c ~0 on error (i.e. an invalid array object 
 *          were supplied)
 */
int u_array_set_cb_free (u_array_t *a, void (*cb_free)(void *))
{
    dbg_return_if (a == NULL, ~0);

    a->cb_free = cb_free;

    return 0;
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
    size_t j;

    dbg_return_if (a == NULL, );

    /* if user has set a free(3)-like function (via u_array_set_cb_free(), 
     * use it to delete elements */
    if (a->cb_free)
    {
        void *elem;

        for (j = 0; j < u_array_nslot(a); ++j)
        {
            if (u_array_get_n(a, j, &elem) == 0)
                a->cb_free(elem);
        }
    }

    U_FREE(a->base);
    u_free(a);

    return;
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
    return u_array_set_n(a, u_array_top(a) + 1, elem, NULL);
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
    /* don't check 'a', let it segfault */
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
    /* don't check 'a', let it segfault */
    return (a->nslot - a->nelem);
}

/**
 *  \brief  Get the total number of slots (used and/or available) in array
 *
 *  \param a    the array object
 *
 *  \return the total number of slots in \p a
 */
size_t u_array_nslot (u_array_t *a)
{
    /* don't check 'a', let it segfault */
    return a->nslot;
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
    /* don't check 'a', let it segfault */
    return a->top;
}

/* debug only */
void u_array_print (u_array_t *a)
{
    size_t j;

    dbg_return_if (a == NULL, );

    con("a(%p)", a);

    for (j = 0; j < u_array_nslot(a); ++j)
    {
        void *elem;

        con("   %p[%zu] = %p", a->base + j, j, 
                u_array_get_n(a, j, &elem) == 0 ? elem : 0x00);
    }

    return;
}

/**
 *  \}
 */
