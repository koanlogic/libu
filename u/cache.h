/* 
 * Copyright (c) 2005, 2006 by KoanLogic s.r.l. - All rights reserved.  
 */

#ifndef _U_CACHE_H_
#define _U_CACHE_H_

#include <u/queue.h>
#include <u/str.h>

#ifdef __cplusplus
extern "C" {
#endif

/** \brief Policies to discard cache elements */
typedef enum {
    U_CACHE_PCY_NONE,    /**< never discard old elements - for bounded inserts only */
    U_CACHE_PCY_FIFO,    /**< discard entry inserted longest ago */
    U_CACHE_PCY_LRU,     /**< discard least recently used */
    U_CACHE_PCY_LFU      /**< discard least frequently used */     
} u_cache_pcy_t;

/** \brief Optional Cache settings */
typedef struct u_cache_opts_s {

    size_t              max_size;   /**< maximum size of hashmap array */
    size_t              max_elems;  /**< maximum number of elements in cache */
    u_cache_pcy_t       policy;     /**< caching policy */

    /** hash function to be used in hashmap */
    size_t        (*f_hash)(const char *key, size_t buckets);   
    /** function for key comparison */
    int           (*f_comp)(const char *k1, const char *k2);   
    /** function for freeing an object */
    void          (*f_free)(void *val);   
    /** function to get a string representation of an object */
    u_string_t   *(*f_str)(void *val);   
} u_cache_opts_t;

/* internal pre-declarations */
typedef struct u_cache_s u_cache_t;     

/* u_cache_t */
int u_cache_new (u_cache_opts_t *opts, u_cache_t **cache);
int u_cache_put (u_cache_t *cache, const char *key, void *val);
int u_cache_get (u_cache_t *cache, const char *key, void **val);
int u_cache_del (u_cache_t *cache, const char *key);
int u_cache_free (u_cache_t *cache);
int u_cache_foreach (u_cache_t *cache, int f(void *val));

/* u_cache_opts_t */
int u_cache_opts_new (u_cache_opts_t **opts);

/* testing */
void u_cache_dbg (u_cache_t *cache);
void u_cache_opts_dbg (u_cache_opts_t *opts);
void u_cache_pcy_dbg (u_cache_t *cache);

#ifdef __cplusplus
}
#endif

#endif /* !_U_CACHE_H_ */
