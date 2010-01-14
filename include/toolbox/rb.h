/*
 * Copyright (c) 2005-2010 by KoanLogic s.r.l. - All rights reserved.
 */

#ifndef _U_RB_H_
#define _U_RB_H_

#include <u/libu.h>

/* forward decl */
struct u_rb_s;

/**
 *  \addtogroup rb
 *  \{
 */ 

/** \brief  ... */
typedef struct u_rb_s u_rb_t;

int u_rb_create (size_t hint_sz, u_rb_t **prb);
int u_rb_clear (u_rb_t *rb);
void u_rb_free (u_rb_t *rb);
size_t u_rb_size (u_rb_t *rb);

ssize_t u_rb_read (u_rb_t *rb, char *b, size_t b_sz);
ssize_t u_rb_write (u_rb_t *rb, const char *b, size_t b_sz);
size_t u_rb_ready (u_rb_t *rb);
size_t u_rb_avail (u_rb_t *rb);

/**
 *  \}
 */ 

#endif  /* !_U_RB_H_ */
