/*
 * Copyright (c) 2005-2008 by KoanLogic s.r.l. - All rights reserved.
 */

#ifndef _U_LIST_H_
#define _U_LIST_H_

#include <u/libu_conf.h>

#ifdef __cplusplus
extern "C" {
#endif

struct u_list_s;
typedef struct u_list_s u_list_t;

int u_list_create(u_list_t **plist);
void u_list_free(u_list_t *list);
int u_list_add(u_list_t *list, void *ptr);
int u_list_insert(u_list_t *list, void *ptr, size_t n);
int u_list_del(u_list_t *list, void *ptr);
int u_list_del_n(u_list_t *list, size_t n, void **pptr);
size_t u_list_count(u_list_t *list);
void* u_list_get_n(u_list_t *list, size_t n);
void* u_list_first(u_list_t *list, void**);
void* u_list_next(u_list_t *list, void**);

#ifdef __cplusplus
}
#endif

#endif /* !_U_LIST_H_ */
