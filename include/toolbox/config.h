/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.  
 */

#ifndef _U_CONFIG_H_
#define _U_CONFIG_H_

#include <stdio.h>
#include <u/libu_conf.h>

#ifdef __cplusplus
extern "C" {
#endif

/* forward decl */
struct u_config_s;

/**
 *  \addtogroup config
 *  \{
 */

/** \brief  Configuration base type */
typedef struct u_config_s u_config_t;

/** \brief  Configuration loading driver callbacks */
struct u_config_driver_s
{
    int (*open) (const char *uri, void **parg);
    /**<    
     *  Open the the resource pointed by \p uri and return its opaque handler
     *  through the \p parg result argument.  The \p *parg (e.g. a file stream
     *  pointer in case of a file system ::u_config_driver_t) will be possibly 
     *  used by subsequent u_config_driver_s::close and u_config_driver_s::gets
     *  callbacks.
     *
     *  \retval  0  on success
     *  \retval ~0  on failure
     */

    int (*close) (void *arg);
    /**< 
     *  Close the (previously u_config_driver_s::open'ed) resource pointed by
     *  \p arg. 
     *
     *  \retval  0  on success
     *  \retval ~0  on failure
     */

    char *(*gets) (void *arg, char *buf, size_t size);
    /**<  
     *  fgets(3)-like function using a generic resource pointer \p arg.
     *  The function reads at most one less than the number of characters 
     *  specified by \p size from the resource handler pointed by \p arg, 
     *  and stores them in the string \p buf.  Reading stops when a newline 
     *  character is found, at end-of-file or error.  The newline, if any, 
     *  is retained.  If any characters are read and there is no error, a 
     *  \c \\0 character is appended to end the string.
     *
     *  \return Upon successful completion, a pointer to the string is 
     *          returned.  If end-of-file occurs before any characters are 
     *          read, return \c NULL and the buffer contents remain 
     *          unchanged.  If an error occurs, return \c NULL and the 
     *          buffer contents are indeterminate.
     */

    int (*resolv) (const char *name, char *uri, size_t uri_bufsz);
    /**< 
     *  Resolver function for mapping relative to absolute resource names, 
     *  e.g. when handling an \c include directive from within another resource
     *  object).  In case of success, the resolved \p uri can be used as first 
     *  argument to a subsequent u_config_driver_s::open call.
     *  The \p name argument is the relative resource name, \p uri is a
     *  pre-alloc'd string of \p uri_bufsz bytes which will store the absolute
     *  resource name in case the resolution is successful.
     *
     *  \retval  0  on success
     *  \retval ~0  on failure
     */
};

/** \brief  Configuration loading driver type */
typedef struct u_config_driver_s u_config_driver_t;

/** \brief  Configuration tree walking strategies (See ::u_config_walk). */
typedef enum {
    U_CONFIG_WALK_PREORDER, /**< Visit the node, then its children. */ 
    U_CONFIG_WALK_POSTORDER /**< Visit node's children, then the node. */ 
} u_config_walk_t;

/**
 *  \}
 */ 

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

int u_config_sort_children(u_config_t *c, int(*)(u_config_t**, u_config_t**));
void u_config_walk (u_config_t *c, u_config_walk_t s, void (*cb)(u_config_t *));

#ifdef __cplusplus
}
#endif

#endif /* !_U_CONFIG_H_ */
