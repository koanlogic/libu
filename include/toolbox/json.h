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
enum { U_JSON_WALK_PREORDER, U_JSON_WALK_POSTORDER };

/** \brief  JSON base types */
typedef enum {
    U_JSON_TYPE_UNKNOWN = 0,
    U_JSON_TYPE_STRING,
    U_JSON_TYPE_NUMBER,
    U_JSON_TYPE_OBJECT,
    U_JSON_TYPE_ARRAY,
    U_JSON_TYPE_TRUE,
    U_JSON_TYPE_FALSE,
    U_JSON_TYPE_NULL
} u_json_type_t;

int u_json_parse (const char *json, u_json_obj_t **pjo);

int u_json_obj_new (u_json_obj_t **pjo);
int u_json_obj_set_key (u_json_obj_t *jo, const char *key);
int u_json_obj_set_val (u_json_obj_t *jo, const char *val);
int u_json_obj_set_type (u_json_obj_t *jo, int type); 
void u_json_obj_free (u_json_obj_t *jo);
int u_json_obj_add (u_json_obj_t *head, u_json_obj_t *jo);

void u_json_obj_print (u_json_obj_t *jo);
void u_json_obj_walk (u_json_obj_t *jo, int strategy, size_t l, 
        void (*cb)(u_json_obj_t *, size_t));

int u_json_encode (u_json_obj_t *jo, const char **ps);

/**
 *  \}
 */ 

#ifdef __cplusplus
}
#endif

#endif  /* !_U_JSON_H_ */
