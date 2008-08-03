/* 
 * Copyright (c) 2005-2008 by KoanLogic s.r.l. - All rights reserved.  
 */

#ifndef _U_CONFIG_H_
#define _U_CONFIG_H_

#include <stdio.h>
#include <u/libu_conf.h>

#ifdef __cplusplus
extern "C" {
#endif

struct u_config_s;
typedef struct u_config_s u_config_t;

struct u_config_driver_s
{
    int (*open)(const char *uri, void **parg);
    int (*close)(void *arg);
    char* (*gets)(void *arg, char *buf, size_t size);
    int (*resolv)(const char *name, char *uri, size_t uri_bufsz);
};
typedef struct u_config_driver_s u_config_driver_t;

int u_config_create(u_config_t **pc);
int u_config_free(u_config_t *c);
int u_config_load(u_config_t *c, int fd, int overwrite);

int u_config_load_from_file (const char *file, u_config_t **pc);

int u_config_load_from_drv(const char *uri, u_config_driver_t *drv, 
        int overwrite, u_config_t **pc);

typedef char* (*u_config_gets_t)(void *arg, char *buf, size_t size);
int u_config_load_from(u_config_t *c, u_config_gets_t cb, 
    void *arg, int overwrite);

const char* u_config_get_key(u_config_t *c);
const char* u_config_get_value(u_config_t *c);

int u_config_get_subkey(u_config_t *c, const char *subkey, u_config_t **pc);
int u_config_get_subkey_nth(u_config_t *c,const char *subkey, int n, 
    u_config_t **pc);

const char* u_config_get_subkey_value(u_config_t *c, const char *subkey);

int u_config_get_subkey_value_b(u_config_t *c, const char *subkey, int def, 
    int *out);
int u_config_get_subkey_value_i(u_config_t *c, const char *subkey, int def, 
    int *out);

int u_config_add_key(u_config_t *c, const char *key, const char *val);
int u_config_set_key(u_config_t *c, const char *key, const char *val);

int u_config_add_child(u_config_t *c, const char *key, u_config_t **pc);
int u_config_set_value(u_config_t *c, const char *val);
u_config_t* u_config_get_child_n(u_config_t *c, const char *key, int n);
u_config_t* u_config_get_child(u_config_t *c, const char *key);
int u_config_del_child(u_config_t *c, u_config_t *child);

int u_config_has_children(u_config_t *c);
int u_config_save_to_buf(u_config_t *c, char *buf, size_t size);
int u_config_load_from_buf(char *buf, size_t len, u_config_t **pc);

void u_config_print_to_fp(u_config_t *c, FILE *fp, int lev);
void u_config_print(u_config_t *c, int lev);

#ifdef __cplusplus
}
#endif

#endif /* !_U_CONFIG_H_ */
