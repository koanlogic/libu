/* 
 * Copyright (c) 2005, 2006 by KoanLogic s.r.l. - All rights reserved.  
 */

#ifndef _U_HMAP_H_
#define _U_HMAP_H_

#include <u/queue.h>
#include <u/str.h>

#ifdef __cplusplus
extern "C" {
#endif


/** \brief Policies to discard hmap elements */
typedef enum {
    U_HMAP_PCY_NONE,    /**< never discard old elements - 
                          for bounded inserts only */
    U_HMAP_PCY_FIFO,    /**< discard entry inserted longest ago */
    U_HMAP_PCY_LRU,     /**< discard least recently used */
    U_HMAP_PCY_LFU      /**< discard least frequently used */     
} u_hmap_pcy_t;

/** \brief Optional Map settings */
typedef struct u_hmap_opts_s {

    size_t max_size;        /**< maximum size of hashhmap array */
    size_t max_elems;       /**< maximum number of elements in hmap */
    u_hmap_pcy_t policy;    /**< caching policy */

    /** hash function to be used in hashhmap */
    size_t (*f_hash)(const char *key, size_t buckets);   
    /** function for key comparison */
    int (*f_comp)(const char *k1, const char *k2);   
    /** function for freeing an object */
    void (*f_free)(void *val);   
    /** function to get a string representation of an object */
    u_string_t *(*f_str)(void *val);   
} u_hmap_opts_t;

/* internal pre-declarations */
typedef struct u_hmap_s u_hmap_t;     


/* u_hmap_t */
int u_hmap_new (u_hmap_opts_t *opts, u_hmap_t **hmap);
int u_hmap_put (u_hmap_t *hmap, const char *key, void *val);
int u_hmap_get (u_hmap_t *hmap, const char *key, void **val);
int u_hmap_del (u_hmap_t *hmap, const char *key);
int u_hmap_free (u_hmap_t *hmap);
int u_hmap_foreach (u_hmap_t *hmap, int f(void *val));

/* u_hmap_opts_t */
int u_hmap_opts_new (u_hmap_opts_t **opts);

/* testing */
void u_hmap_dbg (u_hmap_t *hmap);
void u_hmap_opts_dbg (u_hmap_opts_t *opts);
void u_hmap_pcy_dbg (u_hmap_t *hmap);


#ifdef __cplusplus
}
#endif

#endif /* !_U_HMAP_H_ */
