/*
 * Copyright (c) 2005-2008 by KoanLogic s.r.l. - All rights reserved.
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

/* password hashing function:
 * supply a string and its lenght, return the hashed string */
typedef int (*u_pwd_hash_cb_t) (const char *, size_t, char []);

/* load function:
 * fgets-like prototype with generic stream type */
typedef char *(*u_pwd_load_cb_t) (char *, int, void *);

/* master password db open:
 * supply an uri, return the resource handler */
typedef int (*u_pwd_open_cb_t) (const char *, void **);

/* master password db close:
 * supply res handler */
typedef void (*u_pwd_close_cb_t) (void *);

/* update notification:
 * return true if supplied timestamp is older than last modification time */
typedef int (*u_pwd_notify_cb_t) (const char *, time_t, time_t *);

/* interface */
int u_pwd_init (const char *res_uri, u_pwd_open_cb_t cb_open, 
        u_pwd_load_cb_t cb_load, u_pwd_close_cb_t cb_close, 
        u_pwd_notify_cb_t cb_notify, u_pwd_hash_cb_t cb_hash, 
        size_t hash_len, int in_memory, u_pwd_t **ppwd);
int u_pwd_init_file (const char *res_uri, u_pwd_hash_cb_t cb_hash, 
        size_t hash_len, int in_memory, u_pwd_t **ppwd);
void u_pwd_term (u_pwd_t *pwd);
int u_pwd_in_memory (u_pwd_t *pwd);
int u_pwd_retr (u_pwd_t *pwd, const char *user, u_pwd_rec_t **prec);
int u_pwd_auth_user (u_pwd_t *pwd, const char *user, const char *password);
void u_pwd_rec_free (u_pwd_rec_t *rec);
const char *u_pwd_rec_get_user (u_pwd_rec_t *rec);
const char *u_pwd_rec_get_password (u_pwd_rec_t *rec);
const char *u_pwd_rec_get_opaque (u_pwd_rec_t *rec);

#ifdef __cplusplus
}
#endif

#endif /* !_U_PWD_H_ */
