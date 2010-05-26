#ifndef _U_JSON_H_
#define _U_JSON_H_

#include <sys/types.h>

#ifndef U_JSON_KEY_SZ
    #define U_JSON_KEY_SZ 128
#endif  /* !U_JSON_KEY_SZ */

#ifndef U_JSON_VAL_SZ
    #define U_JSON_VAL_SZ 128
#endif  /* !U_JSON_VAL_SZ */

enum { U_JSON_WALK_PREORDER, U_JSON_WALK_POSTORDER };

typedef struct u_json_s u_json_t;
typedef struct u_json_obj_s u_json_obj_t;

int u_json_new (const char *s, u_json_t **pjl);
void u_json_free (u_json_t *jl);

int u_json_parse (const char *json, u_json_obj_t **pjo);
const char *u_json_geterr (u_json_t *jl);

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

#endif  /* !_U_JSON_H_ */
