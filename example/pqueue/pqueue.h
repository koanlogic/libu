/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l.
 */

#ifndef _PQUEUE_H_
#define _PQUEUE_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 *  \addtogroup grouptag
 *  \{
 */

struct pq_item_s;
struct pq_s;

typedef struct pq_item_s pq_item_t;
typedef struct pq_s pq_t;

int pq_create (size_t maxitems, pq_t **ppq);
int pq_push (pq_t *pq, double key, const void *val);
void *pq_delmax (pq_t *pq, double *pkey);
void *pq_peekmax (pq_t *pq, double *pkey);
int pq_empty (pq_t *pq);
int pq_full (pq_t *pq);
void pq_free (pq_t *pq);

/**
 *  \}
 */ 

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* !_PQUEUE_H_ */
