/* 
 * Copyright (c) 2005-2010 by KoanLogic s.r.l. - All rights reserved.  
 */

#include <stdint.h>
#include <u/libu.h>
#include <toolbox/pqueue.h>

#define u_pq_test_score(q, prio, t1, t2) \
    ((q->policy == U_PQ_KEEP_HIGHEST) ? \
        prio > q->items[t1].priority : prio < q->items[t2].priority)

struct u_pqitem_s
{
    double priority;
    const void *ptr;
    int prev, next;
};

struct u_pq_s
{
    u_pq_policy_t policy;
    size_t nelems;  /* number of alloc'd u_pqitem_t slots */
    size_t count;   /* number of elements appende into the queue */
    u_pqitem_t *items;
    int last, first;
};

static int u_pq_append (u_pq_t *q, int pos, double priority, const void *ptr);

/**
 *  \defgroup   pq  Priority Queue
 *  \{
 *      The \ref pq module implements a fixed width array of generic elements
 *      (pointers) each of which is tagged with a given priority (chosen at
 *      the time of insertion of the element) by which the queue is explicitly 
 *      ordered.
 *
 *      Pushing an element will always succeed if the queue is not full.
 *
 *      If the queue is full the pushed element will be inserted only if its
 *      score is better then the score of one or more of the existing 
 *      elements. It such case the element having the worst score will be 
 *      discarded.
 *
 *      Use ::U_PQ_KEEP_LOWEST as the second parameter to ::u_pq_create if 
 *      you want to keep elements with lowest priorities, ::U_PQ_KEEP_HIGHEST 
 *      otherwise.
 *
 *      The set of available primitives is very simple: creation (via
 *      ::u_pq_create), insertion (::u_pq_push), iteration (the ::u_pq_foreach
 *      macro) and destruction (::u_pq_free): clearly there is no explicit 
 *      deletion interface since dropping an element is an implicit outcome
 *      of the insertion op when the queue is full.
 */

/**
 *  \brief  Push an element with the given priority
 *
 *  Push the element \p ptr into the ::u_pq_t object \p q with the given
 *  priority \p prio
 *  
 *  \param  q       an ::u_pq_t object
 *  \param  prio    the priority tag for \p ptr
 *  \param  ptr     the element to be pushed into \p q
 *
 *  \retval  0  on success
 *  \retval ~0  on failure (append failed or element explicitly discarded)
 */ 
int u_pq_push (u_pq_t *q, double prio, const void *ptr)
{
    int waslast;

    dbg_return_if (q == NULL, ~0);
    dbg_return_if (ptr == NULL, ~0);

    /* in case we have free slots, just append 'ptr' at 'q->count' position */
    if (q->count < q->nelems)
        return u_pq_append(q, q->count, prio, ptr);

    /* if the queue is full, test supplied element priority against
     * last element priority and queue policy to choose if we need to 
     * skip insertion or proceed by overwriting the last element */
    if (u_pq_test_score(q, prio, q->last, q->last))
    {
        waslast = q->last;

        q->count--;
        q->last = q->items[q->last].prev;
        q->items[q->last].next = -1;

        return u_pq_append(q, waslast, prio, ptr);
    } 

    /* element is discarded */
    return ~0;
}

/**
 *  \brief  Return the number of elements actually in list
 *
 *  Return the number of elements in the supplied ::u_pq_t object
 *
 *  \param  q   an ::u_pq_t object
 *
 *  \return the number of elements actually in list
 */ 
size_t u_pq_count (u_pq_t *q)
{
    return q->count;
}

/**
 *  \brief  Create a new priority queue
 *
 *  Create a new ::u_pq_t object with \p nelems elements and return its 
 *  reference as a result value at \p *pq
 *
 *  \param  nelems  maximum number of elements (at least 2) in queue
 *  \param  policy  say if you want to keep the highest (::U_PQ_KEEP_HIGHEST) 
 *                  or lowest (::U_PQ_KEEP_LOWEST) priority values when 
 *                  ::u_pq_push'ing to a full queue
 *  \param  pq      the newly created ::u_pq_t object as a result argument
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */ 
int u_pq_create (size_t nelems, u_pq_policy_t policy, u_pq_t **pq)
{
    u_pq_t *q = NULL;

    dbg_return_if (pq == NULL, ~0); 
    dbg_return_if (nelems < 2, ~0);

    dbg_err_sif ((q = u_zalloc(sizeof *q)) == NULL);

    q->nelems = nelems;
    q->policy = policy; /* keep highest or lowest values in the queue */
    q->last = q->first = -1;

    /* make room for the requested number of elements */
    dbg_err_sif ((q->items = u_calloc(nelems, sizeof(u_pqitem_t))) == NULL);

    *pq = q;

    return 0;
err:
    if (q)
        u_pq_free(q);
    return ~0;
}

/** 
 *  \brief  Dispose the queue
 *
 *  Dispose the supplied ::u_pq_t object \p q
 *
 *  \param  q   a previously allocated ::u_pq_t object
 *
 *  \return nothing
 */
void u_pq_free (u_pq_t *q)
{
    dbg_return_if (q == NULL, );

    if (q->items)
        u_free(q->items);

    u_free(q);

    return;
}

/** \brief  Return first element position */
int u_pq_first (u_pq_t *q) { return q->first; }

/** \brief  Return last element position */
int u_pq_last (u_pq_t *q) { return q->last; }

/** \brief  Return the predecessor of the element at the given position */
int u_pq_prev (u_pq_t *q, size_t t) { return q->items[t].prev; }

/** \brief  Return the element next to the given position */
int u_pq_next (u_pq_t *q, size_t t) { return q->items[t].next; }

/** \brief  Return the priority of the element at the given position */
int u_pq_prio (u_pq_t *q, size_t t) { return q->items[t].priority; }

/** \brief  Return the reference to the items array */
u_pqitem_t *u_pq_items (u_pq_t *q) { return q->items; }

/**
 *  \}
 */ 

static int u_pq_append (u_pq_t *q, int pos, double prio, const void *ptr)
{
    int t, b, prevt = -1;

    dbg_return_if (q == NULL, ~0);
    dbg_return_if (ptr == NULL, ~0);

    /* attach element at the given position */
    q->items[pos].ptr = ptr;
    q->items[pos].priority = prio;

    /* first element goes like this */
    if (q->count == 0)
    {
        q->items[pos].prev = q->items[pos].next = -1;
        q->first = q->last = pos;
        goto end;
    }

    if (!u_pq_test_score(q, prio, q->last, q->last))
    {
        q->items[pos].prev = q->last;
        q->items[pos].next = -1;
        q->items[q->last].next = pos;
        q->last = pos;
    } 
    else 
    {
        for (t = q->last; t != -1; prevt = t, t = q->items[t].prev)
        {
            if (!u_pq_test_score(q, prio, t, t))
                break;
        }

        b = q->items[prevt].prev;

        /* prev */
        q->items[prevt].prev = pos;
        q->items[pos].prev = b;

        /* next */
        if (b != -1)
            q->items[b].next = pos;
        q->items[pos].next = prevt;

        if (t == -1)
            q->first = pos;
    }

end:
    q->count++;

    return 0;
}

