/*
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.
 */

#ifndef _U_JSON_H_
#define _U_JSON_H_

#include <sys/types.h>
#include <u/libu_conf.h>
#include <u/toolbox/lexer.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward decls. */ 
struct u_json_s;

/**
 *  \addtogroup json
 *  \{
 */ 

/** \brief  Internal representation of any JSON object */
typedef struct u_json_s u_json_t;

/** \brief  Opaque iterator for traversing arrays and objects. */
typedef struct { u_json_t *cur; } u_json_it_t;

/** \brief  Walk strategy when traversing JSON objects */
enum { 
    U_JSON_WALK_PREORDER,   /**< pre-order tree walk */
    U_JSON_WALK_POSTORDER   /**< post-order tree walk */
};

/** \brief  JSON base types */
typedef enum {
    U_JSON_TYPE_UNKNOWN = 0,
    U_JSON_TYPE_STRING,         /**< string type */
    U_JSON_TYPE_NUMBER,         /**< number (i.e. int, frac or exp) type */
    U_JSON_TYPE_OBJECT,         /**< object container */
    U_JSON_TYPE_ARRAY,          /**< array container */
    U_JSON_TYPE_TRUE,           /**< a boolean true value */
    U_JSON_TYPE_FALSE,          /**< a boolean false value */
    U_JSON_TYPE_NULL            /**< an explicit null value */
} u_json_type_t;


/** \brief  Size of access keys to JSON values (can be changed at compile time 
 *          via \c -DU_JSON_FQN_SZ=nnn flag) */
#ifndef U_JSON_FQN_SZ
#define U_JSON_FQN_SZ   256
#endif  /* !U_JSON_FQN_SZ */

/** \brief  Max nesting of JSON fields (can be changed at compile time 
 *          via \c -DU_JSON_MAX_DEPTH=nnn flag) */
#ifndef U_JSON_MAX_DEPTH
#define U_JSON_MAX_DEPTH   16
#endif  /* !U_JSON_MAX_DEPTH */

/* Encode/Decode/Validate. */
int u_json_decode (const char *json, u_json_t **pjo);
int u_json_encode (u_json_t *jo, char **ps);
int u_json_validate (const char *json, char status[U_LEXER_ERR_SZ]);

/* Cache creation/destruction. */
int u_json_index (u_json_t *jo);
int u_json_unindex (u_json_t *jo);

/* Cache-aided get/set operations. */
u_json_t *u_json_cache_get (u_json_t *jo, const char *name);
const char *u_json_cache_get_val (u_json_t *jo, const char *name);
int u_json_cache_get_int (u_json_t *jo, const char *name, long *pval);
int u_json_cache_get_real (u_json_t *jo, const char *name, double *pval);
int u_json_cache_get_bool (u_json_t *jo, const char *name, char *pval);

int u_json_cache_set_tv (u_json_t *jo, const char *name, 
        u_json_type_t type, const char *val);

/* JSON base objects [cd]tor. */
int u_json_new (u_json_t **pjo);
void u_json_free (u_json_t *jo);

int u_json_new_object (const char *key, u_json_t **pjo);
int u_json_new_array (const char *key, u_json_t **pjo);
int u_json_new_string (const char *key, const char *val, u_json_t **pjo);
int u_json_new_number (const char *key, const char *val, u_json_t **pjo);
int u_json_new_real (const char *key, double val, u_json_t **pjo);
int u_json_new_int (const char *key, long val, u_json_t **pjo);
int u_json_new_null (const char *key, u_json_t **pjo);
int u_json_new_bool (const char *key, char val, u_json_t **pjo);

/* JSON base objects get/set/add/remove operations. */
int u_json_set_key (u_json_t *jo, const char *key);
int u_json_set_val (u_json_t *jo, const char *val);
int u_json_set_type (u_json_t *jo, u_json_type_t type); 
int u_json_add (u_json_t *head, u_json_t *jo);
int u_json_remove (u_json_t *jo);
const char *u_json_get_val (u_json_t *jo);
int u_json_set_val_ex (u_json_t *jo, const char *val, char validate);
int u_json_get_int (u_json_t *jo, long *pl);
int u_json_get_real (u_json_t *jo, double *pd);
int u_json_get_bool (u_json_t *jo, char *pb);

/* Access to children of a container object. */
u_json_t *u_json_child_first (u_json_t *jo);
u_json_t *u_json_child_last (u_json_t *jo);

/* Array specific ops. */
unsigned int u_json_array_count (u_json_t *jo);
u_json_t *u_json_array_get_nth (u_json_t *jo, unsigned int n);

/* Misc. */
void u_json_print (u_json_t *jo);
void u_json_walk (u_json_t *jo, int strategy, size_t l, 
        void (*cb)(u_json_t *, size_t, void *), void *cb_args);

/* Iterators. */
int u_json_it (u_json_t *jo, u_json_it_t *it);
u_json_t *u_json_it_next (u_json_it_t *it);
u_json_t *u_json_it_prev (u_json_it_t *it);

/**
 *  \}
 */ 

#ifdef __cplusplus
}
#endif

#endif  /* !_U_JSON_H_ */
