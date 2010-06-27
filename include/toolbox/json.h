/*
 * Copyright (c) 2005-2010 by KoanLogic s.r.l. - All rights reserved.
 */

#ifndef _U_JSON_H_
#define _U_JSON_H_

#include <sys/types.h>
#include <u/libu_conf.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward decls. */ 
struct u_json_obj_s;

/**
 *  \addtogroup json
 *  \{
 */ 

/** \brief  Internal representation of any JSON object */
typedef struct u_json_obj_s u_json_obj_t;

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

/* Encode/Decode/Validate. */
int u_json_decode (const char *json, u_json_obj_t **pjo);
int u_json_encode (u_json_obj_t *jo, char **ps);

/* Setters. */
int u_json_obj_new (u_json_obj_t **pjo);
int u_json_obj_set_key (u_json_obj_t *jo, const char *key);
int u_json_obj_set_val (u_json_obj_t *jo, const char *val);
int u_json_obj_set_type (u_json_obj_t *jo, u_json_type_t type); 
void u_json_obj_free (u_json_obj_t *jo);
int u_json_obj_add (u_json_obj_t *head, u_json_obj_t *jo);
int u_json_obj_new_array (const char *key, u_json_obj_t **pjo);
int u_json_obj_new_object (const char *key, u_json_obj_t **pjo);
int u_json_obj_new_leaf (u_json_type_t type, const char *key, const char *val,
        u_json_obj_t **pjo);

/* Getters. */
u_json_obj_t *u_json_get (u_json_obj_t *jo, const char *key);
const char *u_json_get_val (u_json_obj_t *jo, const char *key);
unsigned int u_json_array_count (u_json_obj_t *jo);
u_json_obj_t *u_json_array_get_nth (u_json_obj_t *jo, unsigned int n);

const char *u_json_obj_get_val (u_json_obj_t *jo);

/* Removal. */
int u_json_obj_remove (u_json_obj_t *jo);

/* Indexing (i.e. setup the fast retrieval interface). */
int u_json_freeze (u_json_obj_t *jo);
int u_json_defrost (u_json_obj_t *jo);

/* Misc. */
void u_json_obj_print (u_json_obj_t *jo);
void u_json_obj_walk (u_json_obj_t *jo, int strategy, size_t l, 
        void (*cb)(u_json_obj_t *, size_t, void *), void *cb_args);

/**
 *  \}
 */ 

#ifdef __cplusplus
}
#endif

#endif  /* !_U_JSON_H_ */
