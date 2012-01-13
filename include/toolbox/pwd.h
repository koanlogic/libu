/*
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.
 */

#ifndef _U_PWD_H_
#define _U_PWD_H_

#include <u/libu_conf.h>

#ifdef __cplusplus
extern "C" {
#endif


/* forward decls */
struct u_pwd_s;
struct u_pwd_rec_s;

/**
 *  \addtogroup pwd
 *  \{
 */ 

/** \brief  default length of a password file line (can be changed at
 *          compile time via \c -DU_PWD_LINE_MAX=nnn flag)  */
#ifndef U_PWD_LINE_MAX
#define U_PWD_LINE_MAX  256
#endif  /* !U_PWD_LINE_MAX */

/** \brief  Base pwd object, mediates all operations over the password DB */
typedef struct u_pwd_s u_pwd_t;

/** \brief  Carry information about a single password DB record */
typedef struct u_pwd_rec_s u_pwd_rec_t;

/** \brief  Password hashing callback prototype: accept a string and its 
 *          lenght, return the hashed string */
typedef int (*u_pwd_hash_cb_t) (const char *, size_t, char []);

/** \brief  Record load callback prototype: has fgets(3)-like prototype with 
 *          generic storage resource handler */
typedef char *(*u_pwd_load_cb_t) (char *, int, void *);

/** \brief  Master password DB open callback prototype: accepts an uri, return 
 *          the (opaque) resource handler - the same that will be supplied to 
 *          the ::u_pwd_load_cb_t */
typedef int (*u_pwd_open_cb_t) (const char *, void **);

/* \brief   Master password DB close callback prototype: takes the resource 
 *          handler, return nothing */
typedef void (*u_pwd_close_cb_t) (void *);

/** \brief  Update notification callback prototype: return true if supplied 
 *          timestamp is older than last modification time (this will force
 *          a reload for in-memory password DBs) */
typedef int (*u_pwd_notify_cb_t) (const char *, time_t, time_t *);

/**
 *  \}
 */ 

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
void u_pwd_rec_free (u_pwd_t *pwd, u_pwd_rec_t *rec);
const char *u_pwd_rec_get_user (u_pwd_rec_t *rec);
const char *u_pwd_rec_get_password (u_pwd_rec_t *rec);
const char *u_pwd_rec_get_opaque (u_pwd_rec_t *rec);

#ifdef __cplusplus
}
#endif

#endif /* !_U_PWD_H_ */
