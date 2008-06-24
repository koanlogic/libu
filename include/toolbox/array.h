/*
 * Copyright (c) 2005-2008 by KoanLogic s.r.l. - All rights reserved.
 */

#ifndef _U_ARRAY_H_
#define _U_ARRAY_H_

#include <u/libu_conf.h>

#ifdef __cplusplus
extern "C" {
#endif

struct u_array_s;
typedef struct u_array_s u_array_t;

#define U_ARRAY_GROW_AUTO   0

int u_array_create (size_t sz, u_array_t **pa);
void u_array_free (u_array_t *a);
int u_array_grow (u_array_t *a, size_t more);
int u_array_add (u_array_t *a, void *elem);
void *u_array_get_n (u_array_t *a, size_t idx);
int u_array_set_n (u_array_t *a, void *elem, size_t idx);
size_t u_array_count (u_array_t *a);
size_t u_array_avail (u_array_t *a);
size_t u_array_size (u_array_t *a);

#ifdef __cplusplus
}
#endif

#endif /* !_U_ARRAY_H_ */
