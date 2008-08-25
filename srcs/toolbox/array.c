/* 
 * Copyright (c) 2005-2008 by KoanLogic s.r.l. - All rights reserved.  
 */

#include <u/libu_conf.h>
#include <u/libu.h>
#include <toolbox/array.h>

typedef struct __slot_s { int set; void *data; } __slot_t;

#define U_ARRAY_NSLOT_MAX       (((size_t) ~0) / sizeof(__slot_t))
#define U_ARRAY_DFLT_GROW       100
#define U_ARRAY_VALID_IDX(idx)  (idx < U_ARRAY_NSLOT_MAX)

struct u_array_s
{
    __slot_t *base;
    size_t nslot;               /* total # of slots (0..U_ARRAY_NSLOT_MAX) */
    size_t nelem;               /* # of busy slots (0..U_ARRAY_NSLOT_MAX) */
    size_t top;                 /* top element idx: (0..U_ARRAY_NSLOT_MAX-1) */
    void (*cb_free)(void *);    /* optional free function for hosted elements */
};

/* use fixed increments */
static size_t u_array_want_more (u_array_t *a)
{
    /* assume 'a' has been already sanitized */
    return U_MIN(U_ARRAY_DFLT_GROW, U_ARRAY_NSLOT_MAX - a->nslot);
}

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
    warn_return_ifm (nslot > U_ARRAY_NSLOT_MAX, ~0, 
            "the maximum number of slots an array can hold is %zu", 
            U_ARRAY_NSLOT_MAX);

    /* make root for the container */
    a = u_zalloc(sizeof(u_array_t));
    warn_err_sif (a == NULL);

    /* if we've been requested to allocate some slot, do it now */
    if ((a->nslot = nslot) != 0)
        warn_err_sif ((a->base = u_zalloc(nslot * sizeof(__slot_t))) == NULL);

    a->nelem = 0;
    a->top = U_ARRAY_TOP_INITIALIZER;
    a->cb_free = NULL;  /* can be set later on via u_array_set_cb_free() */
 
    /* array obj is ready */
    *pa = a;
    
    return 0;
err:
    if (a)
    {
        u_free(a);
        U_FREE(a->base);
    }
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
    dbg_return_if (!U_ARRAY_VALID_IDX(idx), ~0);

    /* check if we're on or after array bounds */
    if (idx >= a->nslot)
    {
        more = (idx >= a->nslot + u_array_want_more(a)) ?
            idx - a->nslot + 1 :    /* alloc exactly what is needed */
            U_ARRAY_GROW_AUTO ;     /* alloc a block of new slots */
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

    /* copy-in elem: scalar MUST fit sizeof(void *) - i.e. 'int' is ok, 
     * 'double' is not - pointers have no restriction */
    s->data = elem;
    s->set = 1;

    a->nelem += 1;
    /* adjust 'top' indicator if needed */
    a->top = (idx > a->top || a->top == U_ARRAY_TOP_INITIALIZER) ? idx : a->top;

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

    /* calc 'new_slot' depending on requested increment ('more') and actual 
     * number of slots ('a->nslots') */
    if (more == U_ARRAY_GROW_AUTO)
        new_nslot = a->nslot + u_array_want_more(a);
    else if (a->nslot <= U_ARRAY_NSLOT_MAX - more)
        new_nslot = a->nslot + more;
    else 
    {
        /* this branch intercepts overflows, i.e. the condition 
         * a->nslot + more > U_ARRAY_NSLOT_MAX.  in case it happens 
         * 'new_nslot' is rounded to U_ARRAY_NSLOT_MAX */
        warn("requested amount of slot (%zu) would overflow array bounds");
        new_nslot = U_ARRAY_NSLOT_MAX;
    }

    /* NOTE: if realloc fails the memory at a->base is still valid */
    new_base = u_realloc(a->base, new_nslot * sizeof(__slot_t));
    warn_err_sif (new_base == NULL);

    /* zero-ize newly alloc'd memory (needed because of s->set tests for
     * catching overrides) */
    memset(new_base + a->nslot, 0, (new_nslot - a->nslot) * sizeof(__slot_t));

    /* reassing base in case realloc has changed it */
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
 *  \return the index of the top element in \p a or \c U_ARRAY_TOP_INITIALIZER
 *          in case \p a is empty
 */
size_t u_array_top (u_array_t *a)
{
    /* don't check 'a', let it segfault */
    return a->top;
}

/**
 *  \brief  Get the maximum number of slots that an array object can 
 *          (theoretically) hold
 */
size_t u_array_max_nslot (void)
{
    return U_ARRAY_NSLOT_MAX;
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


