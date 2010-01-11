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

/**
 *  \addtogroup uri
 *  \{
 */  

/** \brief  Maximum length allowed when encoding an URI string, see 
 *          ::u_uri_unparse */
#define U_URI_STRMAX    4096

/** \brief  Base type for all URI operations */
typedef struct u_uri_s u_uri_t;

/* uri ctor/dtor */
int u_uri_new (u_uri_t **pu);
void u_uri_free (u_uri_t *u);

/* uri encoder/decoder */
int u_uri_parse (const char *s, u_uri_t **pu);
int u_uri_unparse (u_uri_t *u, char s[U_URI_STRMAX]);

/* print u_uri_t internal state */
void u_uri_print (u_uri_t *u);

/* getter/setter methods for u_uri_t objects */

/** \brief  Get the scheme value from the supplied \p uri */
const char *u_uri_get_scheme (u_uri_t *uri);

/** \brief  Set the scheme value to \p val on the supplied \p uri */
int u_uri_set_scheme (u_uri_t *uri, const char *val);

/** \brief  Get the user value from the supplied \p uri */
const char *u_uri_get_user (u_uri_t *uri);

/** \brief  Set the user value to \p val on the supplied \p uri */
int u_uri_set_user (u_uri_t *uri, const char *val);

/** \brief  Get the pwd value from the supplied \p uri */
const char *u_uri_get_pwd (u_uri_t *uri);

/** \brief  Set the pwd value to \p val on the supplied \p uri */
int u_uri_set_pwd (u_uri_t *uri, const char *val);

/** \brief  Get the host value from the supplied \p uri */
const char *u_uri_get_host (u_uri_t *uri);

/** \brief  Set the host value to \p val on the supplied \p uri */
int u_uri_set_host (u_uri_t *uri, const char *val);

/** \brief  Get the port value from the supplied \p uri */
const char *u_uri_get_port (u_uri_t *uri);

/** \brief  Set the port value to \p val on the supplied \p uri */
int u_uri_set_port (u_uri_t *uri, const char *val);

/** \brief  Get the authority value from the supplied \p uri */
const char *u_uri_get_authority (u_uri_t *uri);

/** \brief  Set the authority value to \p val on the supplied \p uri */
int u_uri_set_authority (u_uri_t *uri, const char *val);

/** \brief  Get the path value from the supplied \p uri */
const char *u_uri_get_path (u_uri_t *uri);

/** \brief  Set the path value to \p val on the supplied \p uri */
int u_uri_set_path (u_uri_t *uri, const char *val);

/** \brief  Get the query value from the supplied \p uri */
const char *u_uri_get_query (u_uri_t *uri);

/** \brief  Set the query value to \p val on the supplied \p uri */
int u_uri_set_query (u_uri_t *uri, const char *val);

/** \brief  Get the fragment value from the supplied \p uri */
const char *u_uri_get_fragment (u_uri_t *uri);

/** \brief  Set the fragment value to \p val on the supplied \p uri */
int u_uri_set_fragment (u_uri_t *uri, const char *val);

/**
 *  \}
 */ 

#ifdef __cplusplus
}
#endif

#endif /* !_U_URI_H_ */ 
