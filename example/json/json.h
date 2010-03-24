#ifndef _JSON_LEX_H_
#define _JSON_LEX_H_

#include <sys/types.h>

#ifndef JSON_KEY_SZ
    #define JSON_KEY_SZ 128
#endif  /* !JSON_KEY_SZ */

#ifndef JSON_VAL_SZ
    #define JSON_VAL_SZ 128
#endif  /* !JSON_VAL_SZ */

enum { JSON_WALK_PREORDER, JSON_WALK_POSTORDER };

typedef struct json_lex_s json_lex_t;
typedef struct json_obj_s json_obj_t;

int json_lex_new (const char *s, json_lex_t **pjl);
void json_lex_free (json_lex_t *jl);

int json_lex (json_lex_t *jl, json_obj_t **pjo);
const char *json_lex_geterr (json_lex_t *jl);

int json_obj_new (json_obj_t **pjo);
int json_obj_set_key (json_obj_t *jo, const char *key);
int json_obj_set_val (json_obj_t *jo, const char *val);
int json_obj_set_type (json_obj_t *jo, int type);

void json_obj_free (json_obj_t *jo);
int json_obj_add (json_obj_t *head, json_obj_t *jo);

void json_obj_print (json_obj_t *jo);
void json_obj_walk (json_obj_t *jo, int strategy, size_t l, 
        void (*cb)(json_obj_t *, size_t));

int json_encode (json_obj_t *jo, const char **ps);

#endif  /* !_JSON_LEX_H_ */
