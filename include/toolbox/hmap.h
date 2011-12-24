/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.  
 */

#ifndef _U_HMAP_H_
#define _U_HMAP_H_

#include <sys/types.h>
#include <u/libu_conf.h>
#include <u/toolbox/str.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 
 *  \addtogroup hmap
 *  \{
 */

/** \brief hmap error codes */
typedef enum {
    U_HMAP_ERR_NONE = 0,
    U_HMAP_ERR_EXISTS,
    U_HMAP_ERR_FAIL
} u_hmap_ret_t;

/** \brief type of hashmap */
typedef enum {
    U_HMAP_TYPE_CHAIN = 0,  /**< separate chaining (default) */
    U_HMAP_TYPE_LINEAR,     /**< linear probing */
    U_HMAP_TYPE_LAST = U_HMAP_TYPE_LINEAR           
} u_hmap_type_t;

#define U_HMAP_IS_TYPE(t)   (t <= U_HMAP_TYPE_LAST)

/** \brief hmap options */
typedef enum {
    U_HMAP_OPTS_OWNSDATA =      0x1,    /**< hmap owns memory */
    U_HMAP_OPTS_NO_OVERWRITE =  0x2,    /**< don't overwrite equal keys */
    U_HMAP_OPTS_HASH_STRONG =   0x4     /**< custom hash function is strong */
} u_hmap_options_t;

/** \brief hmap data type for keys and values 
 * (only for U_HMAP_OPTS_OWNSDATA) */
typedef enum {
    U_HMAP_OPTS_DATATYPE_POINTER = 0,   /**< pointer to custom data */
    U_HMAP_OPTS_DATATYPE_STRING,        /**< null-terminated string */
    U_HMAP_OPTS_DATATYPE_OPAQUE,        /**< user data of given size */
    U_HMAP_OPTS_DATATYPE_LAST = U_HMAP_OPTS_DATATYPE_OPAQUE
} u_hmap_options_datatype_t;

#define U_HMAP_IS_DATATYPE(t)   (t <= U_HMAP_OPTS_DATATYPE_LAST)

/** \brief Policies to discard hmap elements */
typedef enum {
/*
 * Legend:
 *
 *      ()      pointer
 *      {}      empty queue
 *      []      queue object
 *      ->,<-   push/pop direction
 *      n       element number
 *      t       time
 *      a       accesses
 *      p       priority
 */
    U_HMAP_PCY_NONE = 0,    /**< never discard old elements
                                 (grow if threshold is exceeded) */
                            /* queue: -> (front) {} (back) */

    U_HMAP_PCY_FIFO,    /**< discard entry inserted longest ago */
                        /* queue: -> (front) [n][n-1][n-2].. (back) ->  */

    U_HMAP_PCY_LRU,     /**< discard least recently used (accessed) */
                        /* queue: -> (front) [tn][tn-1][tn-2].. (back) -> */

    U_HMAP_PCY_LFU,     /**< discard least frequently used (accessed) */
                        /* queue: <-> (front) ..[a-2][a-1][a] (back) */

    U_HMAP_PCY_CUSTOM,  /**< user policy via u_hmap_opts_set_policy_cmp() */
                        /* queue: -> (front) [p][p-1][p-2].. (back) -> */

    U_HMAP_PCY_LAST = U_HMAP_PCY_CUSTOM
} u_hmap_pcy_type_t;

#define U_HMAP_IS_PCY(p)    (p <= U_HMAP_PCY_LAST)

typedef struct u_hmap_s u_hmap_t;     
typedef struct u_hmap_pcy_s u_hmap_pcy_t;     
typedef struct u_hmap_o_s u_hmap_o_t;     

/** \brief Map options */
struct u_hmap_opts_s;
typedef struct u_hmap_opts_s u_hmap_opts_t;

/* [u_hmap_easy_*] - simplified interface */
int u_hmap_easy_new (u_hmap_opts_t *opts, u_hmap_t **phmap);
int u_hmap_easy_put (u_hmap_t *hmap, const char *key, const void *val); 
void *u_hmap_easy_get (u_hmap_t *hmap, const char *key); 
int u_hmap_easy_del (u_hmap_t *hmap, const char *key); 
void u_hmap_easy_clear (u_hmap_t *hmap);
void u_hmap_easy_free (u_hmap_t *hmap);

/* [u_hmap_*] */
int u_hmap_new (u_hmap_opts_t *opts, u_hmap_t **phmap);
int u_hmap_put (u_hmap_t *hmap, u_hmap_o_t *obj, u_hmap_o_t **old);
int u_hmap_get (u_hmap_t *hmap, const void *key, u_hmap_o_t **obj);
int u_hmap_del (u_hmap_t *hmap, const void *key, u_hmap_o_t **obj);
int u_hmap_copy (u_hmap_t *to, u_hmap_t *from);
void u_hmap_clear (u_hmap_t *hmap);
void u_hmap_free (u_hmap_t *hmap);
int u_hmap_foreach (u_hmap_t *hmap, int f(const void *val));
int u_hmap_foreach_keyval (u_hmap_t *hmap, int f(const void *key, 
            const void *val));
int u_hmap_foreach_arg (u_hmap_t *hmap, int f(const void *val, 
            const void *arg), void *arg);
ssize_t u_hmap_count (u_hmap_t *hmap);
const char *u_hmap_strerror (u_hmap_ret_t);

/* [u_hmap_o_*] */
u_hmap_o_t *u_hmap_o_new (u_hmap_t *hmap, const void *key, const void *val);
void *u_hmap_o_get_key (u_hmap_o_t *obj);
void *u_hmap_o_get_val (u_hmap_o_t *obj);
void u_hmap_o_free (u_hmap_o_t *obj);

/* [u_hmap_opts_*] */
int u_hmap_opts_new (u_hmap_opts_t **opts);
void u_hmap_opts_init (u_hmap_opts_t *opts);
/* for both simplified and normal interface */
int u_hmap_opts_set_size (u_hmap_opts_t *opts, int sz);
int u_hmap_opts_set_max (u_hmap_opts_t *opts, int max);
int u_hmap_opts_set_type (u_hmap_opts_t *opts, u_hmap_type_t type);
int u_hmap_opts_set_policy (u_hmap_opts_t *opts, u_hmap_pcy_type_t policy);
int u_hmap_opts_set_policy_cmp (u_hmap_opts_t *opts,
        int (*f_pcy_cmp)(void *o1, void *o2));
int u_hmap_opts_set_val_freefunc (u_hmap_opts_t *opts, 
        void (*f_free)(void *val));
int u_hmap_opts_set_val_type (u_hmap_opts_t *opts, 
        u_hmap_options_datatype_t type);
int u_hmap_opts_set_val_sz (u_hmap_opts_t *opts, size_t sz);
int u_hmap_opts_copy (u_hmap_opts_t *to, u_hmap_opts_t *from);
void u_hmap_opts_free (u_hmap_opts_t *opts);
/* advanced options (invalid for simplified interface) */
int u_hmap_opts_set_option (u_hmap_opts_t *opts, int option);
int u_hmap_opts_unset_option (u_hmap_opts_t *opts, int option);
int u_hmap_opts_set_hashfunc (u_hmap_opts_t *opts, 
        size_t (*f_hash)(const void *key, size_t buckets));
int u_hmap_opts_set_compfunc (u_hmap_opts_t *opts, 
        int (*f_comp)(const void *k1, const void *k2));
int u_hmap_opts_set_freefunc (u_hmap_opts_t *opts, 
        void (*f_free)(u_hmap_o_t *obj));
int u_hmap_opts_set_strfunc (u_hmap_opts_t *opts, 
        u_string_t *(*f_str)(u_hmap_o_t *obj));
int u_hmap_opts_set_key_type (u_hmap_opts_t *opts, 
        u_hmap_options_datatype_t type);
int u_hmap_opts_set_key_sz (u_hmap_opts_t *opts, size_t sz);
int u_hmap_opts_set_key_freefunc (u_hmap_opts_t *opts, 
        void (*f_free)(const void *key));

/* testing */
void u_hmap_dbg (u_hmap_t *hmap);
void u_hmap_opts_dbg (u_hmap_opts_t *opts);

/**
 *  \}
 */

void u_hmap_pcy_dbg (u_hmap_t *hmap);

#ifdef __cplusplus
}
#endif

#endif /* !_U_HMAP_H_ */
