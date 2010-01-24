/* 
 * Copyright (c) 2005-2010 by KoanLogic s.r.l. - All rights reserved.  
 */

#ifndef _U_PQ_H_
#define _U_PQ_H_

#include <u/libu_conf.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

struct u_pqitem_s;
struct u_pq_s;

/**
 *  \addtogroup pqueue
 *  \{
 */ 

/** \brief  Queue Policies */
typedef enum { 
    U_PQ_KEEP_HIGHEST,  /**< drop lower priority elements first */
    U_PQ_KEEP_LOWEST    /**< drop higher priority elements first */
} u_pq_policy_t;

/** \brief Queue item slot */
typedef struct u_pqitem_s u_pqitem_t;

/** \brief  Priority queue object */
typedef struct u_pq_s u_pq_t;

int u_pq_create (size_t nelems, u_pq_policy_t policy, u_pq_t **pq);
void u_pq_free (u_pq_t *q);
u_pqitem_t *u_pq_items (u_pq_t *q);
size_t u_pq_count (u_pq_t *q);
int u_pq_push (u_pq_t *q, double weigth, const void *ptr);

int u_pq_first (u_pq_t *q);
int u_pq_last (u_pq_t *q);
int u_pq_prev (u_pq_t *q, size_t t);
int u_pq_next (u_pq_t *q, size_t t);
double u_pq_prio (u_pq_t *q, size_t t);

/** \brief  Traverse the queue in reverse priority order 
 *  
 *  \param  q   ::u_pq_t object to be traversed
 *  \param  t   iterator variable (\c int type)
 */
#define u_pq_foreach_reverse(q, t)  \
    for (t = u_pq_last(q); t != -1; t = u_pq_prev(q, t))

/** \brief  Traverse the queue in reverse priority order 
 *  
 *  \param  q   ::u_pq_t object to be traversed
 *  \param  t   iterator variable (\c int type)
 */
#define u_pq_foreach(q, t)  \
    for (t = u_pq_first(q); t != -1; t = u_pq_next(q, t))

/**
 *  \}
 */ 

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* !_U_PQ_H_ */
