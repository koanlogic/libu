/*
 * Copyright (c) 2005-2007 by KoanLogic s.r.l. - All rights reserved.
 */

#ifndef _U_PWD_H_
#define _U_PWD_H_

#include <u/libu_conf.h>

#ifdef __cplusplus
extern "C" {
#endif

#define U_PWD_LINE_MAX  256

struct u_pwd_s;
typedef struct u_pwd_s u_pwd_t;

struct u_pwd_rec_s;
typedef struct u_pwd_rec_s u_pwd_rec_t;

/* hash prototype */
typedef ssize_t (*hash_fn_t) (const char *, size_t, char []);

/* fgets-like prototype with generic stream type */
typedef char *(*fgets_fn_t) (char *, int, void *);

int u_pwd_init (void *res, fgets_fn_t gf, hash_fn_t hf, size_t hash_len, 
        u_pwd_t **ppwd);
int u_pwd_load (u_pwd_t *pwd);
int u_pwd_term (u_pwd_t *pwd);
int u_pwd_retr (u_pwd_t *pwd, const char *user, u_pwd_rec_t **prec);
int u_pwd_auth_user (u_pwd_t *pwd, const char *user, const char *pass);

#ifdef __cplusplus
}
#endif

#endif /* !_U_PWD_H_ */
