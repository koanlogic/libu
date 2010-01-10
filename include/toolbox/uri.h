/*
 * Copyright (c) 2005-2010 by KoanLogic s.r.l. - All rights reserved.
 */

#ifndef _U_URI_H_
#define _U_URI_H_

#include <u/libu_conf.h>

#ifdef __cplusplus
extern "C" {
#endif

struct u_uri_s;     /* fwd decl */
typedef struct u_uri_s u_uri_t;

/* uri decoder */
int u_uri_parse (const char *s, u_uri_t **pu);

/* getter methods */
const char *u_uri_scheme (u_uri_t *uri);
const char *u_uri_user (u_uri_t *uri);
const char *u_uri_pwd (u_uri_t *uri);
const char *u_uri_host (u_uri_t *uri);
const char *u_uri_path (u_uri_t *uri);
const char *u_uri_port (u_uri_t *uri);
const char *u_uri_fragment (u_uri_t *uri);
const char *u_uri_query (u_uri_t *uri);

void u_uri_free (u_uri_t *uri);

#ifdef __cplusplus
}
#endif

#endif /* !_U_URI_H_ */ 
