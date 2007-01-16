/* 
 * Copyright (c) 2005, 2006 by KoanLogic s.r.l. - All rights reserved.  
 */

#ifndef _U_HMAP_H_
#define _U_HMAP_H_

#include <sys/types.h>
#include <toolbox/str.h>
#include <toolbox/queue.h>

#ifdef __cplusplus
extern "C" {
#endif

/** \brief hmap error codes */
typedef enum {
    U_HMAP_ERR_NONE = 0,
    U_HMAP_ERR_EXISTS,
    U_HMAP_ERR_FAIL
} u_hmap_ret_t;

/** \brief hmap options */
typedef enum {
    U_HMAP_OPTS_OWNSDATA =       0x1,
    U_HMAP_OPTS_NO_OVERWRITE =   0x2,
    U_HMAP_OPTS_NO_ORDERING =    0x4,
	U_HMAP_OPTS_HASH_STRONG = 	 0x8
} u_hmap_options_t;

/** \brief Policies to discard hmap elements */
typedef enum {
    U_HMAP_PCY_NONE = 1,    /**< never discard old elements - 
                                 for bounded inserts only */
    U_HMAP_PCY_FIFO,    /**< discard entry inserted longest ago */
    U_HMAP_PCY_LRU,     /**< discard least recently used */
    U_HMAP_PCY_LFU      /**< discard least frequently used */     
} u_hmap_pcy_t;

typedef struct u_hmap_s u_hmap_t;     
typedef struct u_hmap_opts_s u_hmap_opts_t;
typedef struct u_hmap_q_s u_hmap_q_t;
typedef struct u_hmap_o_s u_hmap_o_t;     

/* internal pre-declarations */
struct u_hmap_o_s 
{
    void *key,
         *val;
    LIST_ENTRY(u_hmap_o_s) next;
    u_hmap_q_t *pqe; 
};

/** \brief Optional Map settings */
struct u_hmap_opts_s {

    size_t max_size,        /**< maximum size of hashhmap array */
           max_elems;       /**< maximum number of elements in hmap */

    u_hmap_pcy_t policy;    /**< caching policy */

    int options;          /**< see definitions for U_HMAP_OPTS_* */ 
                              

    /** hash function to be used in hashhmap */
    size_t (*f_hash)(void *key, size_t buckets);   
    /** function for key comparison */
    int (*f_comp)(void *k1, void *k2);   
    /** function for freeing an object */
    void (*f_free)(u_hmap_o_t *obj);   
    /** function to get a string representation of a (key, val) object */
    u_string_t *(*f_str)(u_hmap_o_t *obj);   
};


/* u_hmap_* */
const char *u_hmap_strerror(u_hmap_ret_t);

/* u_hmap_t */
int u_hmap_new (u_hmap_opts_t *opts, u_hmap_t **hmap);
int u_hmap_put (u_hmap_t *hmap, u_hmap_o_t *obj, u_hmap_o_t **old);
int u_hmap_get (u_hmap_t *hmap, void *key, u_hmap_o_t **obj);
int u_hmap_del (u_hmap_t *hmap, void *key, u_hmap_o_t **obj);
int u_hmap_free (u_hmap_t *hmap);
int u_hmap_foreach (u_hmap_t *hmap, int f(void *val));

/* u_hmap_o_t */
u_hmap_o_t *u_hmap_o_new (void *key, void *val);
void u_hmap_o_free (u_hmap_o_t *obj);

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
