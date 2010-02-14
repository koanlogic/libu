/* 
 * Copyright (c) 2005-2010 by KoanLogic s.r.l. - All rights reserved.  
 */

#ifndef _U_HMAP_H_
#define _U_HMAP_H_

#include <sys/types.h>
#include <u/libu.h>

#ifdef __cplusplus
extern "C" {
#endif

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

/** \brief hmap options */
typedef enum {
    U_HMAP_OPTS_OWNSDATA =      0x1,    /**< hmap owns memory */
    U_HMAP_OPTS_NO_OVERWRITE =  0x2,    /**< don't overwrite equal keys */
    U_HMAP_OPTS_HASH_STRONG =   0x4     /**< custom hash function is strong */
} u_hmap_options_t;

/** \brief hmap data type for keys and values 
 * (only for U_HMAP_OPTS_OWNSDATA) */
typedef enum {
    U_HMAP_OPTS_DATATYPE_POINTER,
    U_HMAP_OPTS_DATATYPE_STRING,
    U_HMAP_OPTS_DATATYPE_OPAQUE,
    U_HMAP_OPTS_DATATYPE_LAST = U_HMAP_OPTS_DATATYPE_OPAQUE
} u_hmap_options_datatype_t;

/** \brief Policies to discard hmap elements */
typedef enum {
    U_HMAP_PCY_NONE = 0,    /**< never discard old elements - 
                                 grow if size is exceeded */
    U_HMAP_PCY_FIFO,    /**< discard entry inserted longest ago */
    U_HMAP_PCY_LRU,     /**< discard least recently used */
    U_HMAP_PCY_LFU,     /**< discard least frequently used */     
    U_HMAP_PCY_LAST = U_HMAP_PCY_LFU 
} u_hmap_pcy_type_t;

typedef struct u_hmap_s u_hmap_t;     
typedef struct u_hmap_pcy_s u_hmap_pcy_t;     

/* policy queue object */
struct u_hmap_q_s 
{
    void *key;
    void *o;

    TAILQ_ENTRY(u_hmap_q_s) next;
};
typedef struct u_hmap_q_s u_hmap_q_t;

/* hmap element object */
struct u_hmap_o_s 
{
    void *key;
    void *val;

    LIST_ENTRY(u_hmap_o_s) next;

    u_hmap_q_t *pqe; 

    u_hmap_t *hmap;         
};
typedef struct u_hmap_o_s u_hmap_o_t;     

/** \brief Map options */
struct u_hmap_opts_s {

    size_t size,            /**< approximate size of hashhmap array */
           max;             /**< maximum number of elements in hmap -
                             only applies to hmaps with discard policy */

    u_hmap_type_t type;         /**< type of hashmap */

    u_hmap_pcy_type_t policy;   /**< discard policy (disabled by default) */

    int options;                /**< see definitions for U_HMAP_OPTS_* */ 

    u_hmap_options_datatype_t key_type;         /**< type of key */     
    u_hmap_options_datatype_t val_type;         /**< type of value */     

    size_t key_sz;                      /* size of key (if OPAQUE) */
    size_t val_sz;                      /* size of value (if OPAQUE) */
                              
    /** hash function to be used in hashhmap */
    size_t (*f_hash)(const void *key, size_t buckets);   
    /** function for key comparison */
    int (*f_comp)(const void *k1, const void *k2);   
    /** function for freeing an object */
    void (*f_free)(u_hmap_o_t *obj);   
    /** function for freeing a key */
    void (*f_key_free)(const void *key);   
    /** function for freeing a value */
    void (*f_val_free)(void *val);   
    /** function to get a string representation of a (key, val) object */
    u_string_t *(*f_str)(u_hmap_o_t *obj);   

    int easy;               /**< whether simplified interface is active 
                                (internal) */ 
};
typedef struct u_hmap_opts_s u_hmap_opts_t;

/* [u_hmap_easy_*] - simplified interface */
int u_hmap_easy_new (u_hmap_opts_t *opts, u_hmap_t **phmap);
int u_hmap_easy_put (u_hmap_t *hmap, const char *key, const void *val); 
void *u_hmap_easy_get (u_hmap_t *hmap, const char *key); 
int u_hmap_easy_del (u_hmap_t *hmap, const char *key); 
void u_hmap_easy_free (u_hmap_t *hmap);

/* [u_hmap_*] */
int u_hmap_new (u_hmap_opts_t *opts, u_hmap_t **phmap);
int u_hmap_put (u_hmap_t *hmap, u_hmap_o_t *obj, u_hmap_o_t **old);
int u_hmap_get (u_hmap_t *hmap, const void *key, u_hmap_o_t **obj);
int u_hmap_del (u_hmap_t *hmap, const void *key, u_hmap_o_t **obj);
int u_hmap_copy (u_hmap_t *to, u_hmap_t *from);
void u_hmap_free (u_hmap_t *hmap);
int u_hmap_foreach (u_hmap_t *hmap, int f(const void *val));
int u_hmap_foreach_keyval (u_hmap_t *hmap, int f(const void *key, 
            const void *val));
int u_hmap_foreach_arg (u_hmap_t *hmap, int f(const void *val, 
            const void *arg), void *arg);
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
int u_hmap_opts_set_type (u_hmap_opts_t *opts, int type);
int u_hmap_opts_set_policy (u_hmap_opts_t *opts, int policy);
int u_hmap_opts_set_val_freefunc (u_hmap_opts_t *opts, 
        void (*f_free)(void *val));
int u_hmap_opts_set_val_type (u_hmap_opts_t *opts, 
        u_hmap_options_datatype_t type);
int u_hmap_opts_set_val_sz (u_hmap_opts_t *opts, size_t sz);
int u_hmap_opts_copy (u_hmap_opts_t *to, u_hmap_opts_t *from);
void u_hmap_opts_free (u_hmap_opts_t *opts);
/* advanced options (invalid for simplified interface) */
int u_hmap_opts_set_options (u_hmap_opts_t *opts, int options);
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
void u_hmap_pcy_dbg (u_hmap_t *hmap);

#ifdef __cplusplus
}
#endif

#endif /* !_U_HMAP_H_ */
