/*
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.
 */

#ifndef _U_LIST_H_
#define _U_LIST_H_

#include <u/libu_conf.h>

#ifdef __cplusplus
extern "C" {
#endif

/* forward decl */
struct u_list_s;

/**
 *  \addtogroup list
 *  \{
 */

/** \brief  Basic list object type */
typedef struct u_list_s u_list_t;

/** \brief  List iterator 
 *
 *  \param  list    an ::u_list_t object
 *  \param  n       a variable that will get the current value from \p list
 *  \param  it      an opaque iterator with type \c void**
 */
#define u_list_foreach(list, n, it) \
    for(n = u_list_first(list, &it); n; n = u_list_next(list, &it))

/** \brief  List iterator with iteration counter
 *
 *  \param  list    an ::u_list_t object
 *  \param  n       a variable that will get the current value from \p list
 *  \param  it      an opaque iterator with type \c void**
 *  \param  i       variable of type \c integer that will be get the iteration
 *                  counter (0..i)
 */ 
#define u_list_iforeach(list, n, it, i) \
    for(i = 0, n = u_list_first(list, &it); n; n = u_list_next(list, &it), ++i)

int u_list_create (u_list_t **plist);
void u_list_free (u_list_t *list);
int u_list_clear (u_list_t *list);
int u_list_add (u_list_t *list, void *ptr);
int u_list_insert (u_list_t *list, void *ptr, size_t n);
int u_list_del (u_list_t *list, void *ptr);
int u_list_del_n (u_list_t *list, size_t n, void **pptr);
void *u_list_get_n (u_list_t *list, size_t n);
size_t u_list_count (u_list_t *list);
void *u_list_first (u_list_t *list, void **it);
void *u_list_next (u_list_t *list, void **it);

/**
 *  \}
 */ 

#ifdef __cplusplus
}
#endif

#endif /* !_U_LIST_H_ */
