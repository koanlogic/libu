/*
 * Copyright (c) 2005-2012 by KoanLogic s.r.l.
 */ 

#include <toolbox/carpal.h>
#include <toolbox/misc.h>
#include <toolbox/pqueue.h>

typedef struct u_pq_item_s
{
    double key;
    void *val;
} u_pq_item_t;

struct u_pq_s
{
    size_t nelems, nitems;
    u_pq_item_t *q;
};

static int pq_item_comp (u_pq_item_t *pi, u_pq_item_t *pj);
static void pq_item_swap (u_pq_item_t *pi, u_pq_item_t *pj);
static void bubble_up (u_pq_item_t *pi, size_t k);
static void bubble_down (u_pq_item_t *pi, size_t k, size_t n);

/**
    \defgroup pq Priority Queues
    \{
        The \ref pq module implements a fixed length priority queue using a 
        heap.  Elements (together with their associated numerical priority)
        are pushed to the queue via (::u_pq_push) until free slots are 
        available.

        The top element can be evicted (::u_pq_delmax) or just peeked at 
        (::u_pq_peekmax).  Eviction and insertion are performed in O(lg(N)) 
        where N is the queue cardinality.

        The following example describes the use of a priority queue to 
        efficiently extract the bottom 10 out of 10 million random values:
    \code
    {
        enum { EMAX = 10 };
        size_t i;
        double key, keymax = DBL_MAX;
        u_pq_t *pq = NULL;

        srandom((unsigned long) getpid());

        con_err_if (u_pq_create(EMAX, &pq));

        // fill the pqueue 
        for (i = 0; i < EMAX; i++)
            con_err_if (u_pq_push(pq, (double) random(), NULL));

        // del-push cycle
        for (i = EMAX; i < 10000000; i++)
        {
            (void) u_pq_peekmax(pq, &keymax);

            if (keymax > (key = (double) random()))
            {
                (void) u_pq_delmax(pq, NULL);
                con_err_if (u_pq_push(pq, key, NULL));
            }
        }

        // print results
        for (i = 0; !u_pq_empty(pq); i++)
        {
            (void) u_pq_delmax(pq, &key);
            u_con("%zu: %.0lf", EMAX - i, key);
        }
    }
    \endcode
 */

/**
 *  \brief  Create a new priority queue
 *
 *  Create a new ::u_u_pq_t object with \p nitems elements and return its 
 *  reference as a result value at \p *ppq
 *
 *  \param  nitems  maximum number of elements (at least 2) in queue
 *  \param  ppq     the newly created ::u_u_pq_t object as a result argument
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */ 
int u_pq_create (size_t nitems, u_pq_t **ppq)
{
    u_pq_t *pq = NULL;

    dbg_return_if (ppq == NULL, ~0);
    dbg_return_if (nitems < 2, ~0); /* Expect at least 2 elements. */

    /* Make room for both the queue head and items' array. */
    dbg_err_sif ((pq = u_zalloc(sizeof *pq)) == NULL);
    dbg_err_sif ((pq->q = u_calloc(nitems + 1, sizeof(u_pq_item_t))) == NULL);

    /* Init the index of last element in array: valid elements are stored 
     * at index'es [1..nitems]. */
    pq->nelems = 0;
    pq->nitems = nitems;

    *ppq = pq;

    return 0;
err:
    u_pq_free(pq);
    return ~0;
}

/**
 *  \brief  Tell if a the supplied queue is empty
 *
 *  \param  pq  queue handler
 *
 *  \return \c 0 when there is some element in \p pq, non-zero otherwise.
 */ 
int u_pq_empty (u_pq_t *pq)
{
    return (pq->nelems == 0);
}

/**
 *  \brief  Tell if a the supplied queue is full
 *
 *  \param  pq  queue handler
 *
 *  \return \c 0 when there is no room left in \p pq, non-zero otherwise.
 */ 
int u_pq_full (u_pq_t *pq)
{
    return (pq->nelems == pq->nitems);
}

/** 
 *  \brief  Dispose the queue
 *
 *  Dispose the supplied ::u_pq_t object \p q
 *
 *  \param  pq  a previously allocated ::u_pq_t object
 *
 *  \return nothing
 */
void u_pq_free (u_pq_t *pq)
{
    dbg_return_if (pq == NULL, );

    if (pq->q)
        u_free(pq->q);
    u_free(pq);

    return;
}

/**
 *  \brief  Push an element with the given priority
 *
 *  Push the element \p val into the ::u_pq_t object \p pq with the given
 *  priority \p key
 *  
 *  \param  pq      an ::u_pq_t object
 *  \param  key     the priority tag for \p ptr
 *  \param  val     the element to be pushed into \p q
 *
 *  \retval  0  on success
 *  \retval -1  on failure
 *  \retval -2  if the insertion can't proceed because the queue is full
 */
int u_pq_push (u_pq_t *pq, double key, const void *val)
{
    u_pq_item_t *pi;

    dbg_return_if (pq == NULL, -1);

    /* Queue full, would overflow. */
    if (u_pq_full(pq))
        return -2;

    pq->nelems += 1;

    /* Get next free slot (from heap bottom). */
    pi = &pq->q[pq->nelems];

    /* Assign element. */
    pi->key = key;
    memcpy(&pi->val, &val, sizeof(void **));

    /* Fix heap condition bottom-up. */
    bubble_up(pq->q, pq->nelems);

    return 0;
}

/** 
 *  \brief  Return the top element without evicting it
 *
 *  \param  pq      an ::u_pq_t object
 *  \param  pkey    if non-NULL, it will store the priority of the top element
 *
 *  \return the element value
 *
 *  \note   If performed on an empty queue the result is unpredictable.
 */
void *u_pq_peekmax (u_pq_t *pq, double *pkey)
{
    u_pq_item_t *pmax = &pq->q[1];

    if (pkey)
        *pkey = pmax->key;

    return pmax->val;
}

/** 
 *  \brief  Evict the top element from the supplied queue
 *
 *  \param  pq      an ::u_pq_t object
 *  \param  pkey    if non-NULL, it will store the priority of the top element
 *
 *  \return the evicted element value
 *
 *  \note   If performed on an empty queue the result is unpredictable.
 */
void *u_pq_delmax (u_pq_t *pq, double *pkey) 
{
    u_pq_item_t *pmax;

    dbg_return_if (pq == NULL, NULL);

    /* Empty queue. XXX NULL is legitimate ... */
    if (pq->nelems == 0)
        return NULL;

    /* Exchange top and bottom items. */
    pq_item_swap(&pq->q[1], &pq->q[pq->nelems]);

    /* Fix heap condition top-down excluding the evicted item. */
    bubble_down(pq->q, 1, pq->nelems - 1);

    pmax = &pq->q[pq->nelems];

    pq->nelems -= 1;

    /* Copy out the deleted key if requested. */
    if (pkey)
        *pkey = pmax->key;

    return pmax->val;
}

/**
 *  \}
 */

static void pq_item_swap (u_pq_item_t *pi, u_pq_item_t *pj)
{
    void *tmpval = pi->val;
    double tmpkey = pi->key;

    pi->key = pj->key;
    pi->val = pj->val;

    pj->key = tmpkey;
    pj->val = tmpval;

    return;
}

static int pq_item_comp (u_pq_item_t *pi, u_pq_item_t *pj)
{
    return (pi->key < pj->key);
}

static void bubble_up (u_pq_item_t *pi, size_t k)
{
    /* Move from bottom to top exchanging child with its parent node until
     * child is higher than parent, or we've reached the top. */
    while (k > 1 && pq_item_comp(&pi[k / 2], &pi[k]))
    {
        pq_item_swap(&pi[k], &pi[k / 2]);
        k = k / 2;
    }

    return;
}

static void bubble_down (u_pq_item_t *pi, size_t k, size_t n)
{
    size_t j;

    /* Move from top to bottom exchanging the parent node with the highest
     * of its children, until the "moving" node is higher then both of them -
     * or we've reached the bottom. */
    while (2 * k <= n)
    {
        j = 2 * k;

        /* Choose to go left or right depending on who's bigger. */
        if (j < n && pq_item_comp(&pi[j], &pi[j + 1]))
            j++;

        if (!pq_item_comp(&pi[k], &pi[j]))
            break;

        pq_item_swap(&pi[k], &pi[j]);

        k = j;
    }

    return;
}
