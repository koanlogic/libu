/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l.
 */

#ifndef _U_PQUEUE_H_
#define _U_PQUEUE_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/* forward decl */
struct u_pq_s;

/**
 *  \addtogroup pq
 *  \{
 */

/** \brief  The priority queue handler. */
typedef struct u_pq_s u_pq_t;

int u_pq_create (size_t maxitems, u_pq_t **ppq);
int u_pq_push (u_pq_t *pq, double key, const void *val);
void *u_pq_delmax (u_pq_t *pq, double *pkey);
void *u_pq_peekmax (u_pq_t *pq, double *pkey);
int u_pq_empty (u_pq_t *pq);
int u_pq_full (u_pq_t *pq);
void u_pq_free (u_pq_t *pq);

/**
 *  \}
 */ 

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* !_U_PQUEUE_H_ */
