/*
 * Copyright (c) 2005-2012 by KoanLogic s.r.l.
 */ 

#include <u/libu.h>
#include "pqueue.h"

struct pq_item_s
{
    double key;
    void *val;
};

struct pq_s
{
    size_t nelems, nitems;
    pq_item_t *q;
};

static int pq_item_comp (pq_item_t *pi, pq_item_t *pj);
static void pq_item_swap (pq_item_t *pi, pq_item_t *pj);
static void bubble_up (pq_item_t *pi, size_t k);
static void bubble_down (pq_item_t *pi, size_t k, size_t n);

int pq_create (size_t nitems, pq_t **ppq)
{
    pq_t *pq = NULL;

    dbg_return_if (ppq == NULL, ~0);
    dbg_return_if (nitems < 2, ~0); /* Expect at least 2 elements. */

    /* Make room for both the queue head and items' array. */
    dbg_err_sif ((pq = u_zalloc(sizeof *pq)) == NULL);
    dbg_err_sif ((pq->q = u_zalloc((nitems + 1) * sizeof(pq_item_t))) == NULL);

    /* Init the index of last element in array: valid elements are stored 
     * at index'es [1..nitems]. */
    pq->nelems = 0;
    pq->nitems = nitems;

    *ppq = pq;

    return 0;
err:
    pq_free(pq);
    return ~0;
}

int pq_empty (pq_t *pq)
{
    return (pq->nelems == 0);
}

int pq_full (pq_t *pq)
{
    return (pq->nelems == pq->nitems);
}

void pq_free (pq_t *pq)
{
    dbg_return_if (pq == NULL, );

    if (pq->q)
        u_free(pq->q);
    u_free(pq);

    return;
}

/* -1: error
 * -2: would overflow
 *  0: ok */
int pq_push (pq_t *pq, double key, const void *val)
{
    pq_item_t *pi;

    dbg_return_if (pq == NULL, -1);

    /* Queue full, would overflow. */
    if (pq_full(pq))
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

/* Is meaningful only when !pq_empty(pq) */
void *pq_peekmax (pq_t *pq, double *pkey)
{
    pq_item_t *pmax = &pq->q[pq->nelems];

    if (pkey)
        *pkey = pmax->key;

    return pmax->val;
}

/* Is meaningful only when !pq_empty(pq) */
void *pq_delmax (pq_t *pq, double *pkey) 
{
    pq_item_t *pmax;

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

static void pq_item_swap (pq_item_t *pi, pq_item_t *pj)
{
    void *tmpval = pi->val;
    double tmpkey = pi->key;

    pi->key = pj->key;
    pi->val = pj->val;

    pj->key = tmpkey;
    pj->val = tmpval;

    return;
}

static int pq_item_comp (pq_item_t *pi, pq_item_t *pj)
{
    return (pi->key > pj->key);
}

static void bubble_up (pq_item_t *pi, size_t k)
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

static void bubble_down (pq_item_t *pi, size_t k, size_t n)
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
