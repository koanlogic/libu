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

int u_array_create (size_t a_sz, u_array_t **pa);
int u_array_set_n (u_array_t *a, size_t idx, void *elem, void **oelem);
int u_array_get_n (u_array_t *a, size_t idx, void **pelem);
int u_array_set_cb_free (u_array_t *a, void (*cb_free)(void *));
int u_array_grow (u_array_t *a, size_t more);
void u_array_free (u_array_t *a);
size_t u_array_count (u_array_t *a);
size_t u_array_avail (u_array_t *a);
size_t u_array_nslot (u_array_t *a);
size_t u_array_top (u_array_t *a);
void u_array_print (u_array_t *a);
int u_array_push (u_array_t *a, void *elem);
size_t u_array_max_nslot (void);

#ifdef __cplusplus
}
#endif

#endif /* !_U_ARRAY_H_ */
