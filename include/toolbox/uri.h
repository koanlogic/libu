/*
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.
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
 *          ::u_uri_knead */
#define U_URI_STRMAX    4096

/** \brief  Option that can or'ed together and supplied to ::u_uri_crumble or 
 *          ::u_uri_new */
typedef enum
{
    U_URI_OPT_NONE = 0x00,
    /**< use default parsing algorithm */

    U_URI_OPT_DONT_PARSE_USERINFO = 0x01,
    /**< do not try to split user":"password in userinfo */
} u_uri_opts_t;

/** \brief  Flags set by the parsing machinery. */
typedef enum
{
    U_URI_FLAGS_NONE = 0x00,
    /**< use default parsing algorithm */

    U_URI_FLAGS_HOST_IS_IPADDRESS = 0x01,
    /**< host is in IP-literal or IPv4address format (otherwise assume 
     *   reg-name.) */

    U_URI_FLAGS_HOST_IS_IPLITERAL = 0x02,
    /**< host is in IP-literal.  User must set this flag if she wants to
     *   correctly knead an IP-literal address, i.e. one surrounded by
     *   square brackets. */
} u_uri_flags_t;

/** \brief  Base type for all URI operations */
typedef struct u_uri_s u_uri_t;

/* uri ctor/dtor */
int u_uri_new (u_uri_opts_t opts, u_uri_t **pu);
void u_uri_free (u_uri_t *u);

/* uri encoder/decoder */
int u_uri_crumble (const char *s, u_uri_opts_t opts, u_uri_t **pu);
int u_uri_knead (u_uri_t *u, char s[U_URI_STRMAX]);

/* print u_uri_t internal state */
void u_uri_print (u_uri_t *u, int extended);

/* getter/setter methods for u_uri_t objects */

/** \brief  Get the scheme value from the supplied \p uri */
const char *u_uri_get_scheme (u_uri_t *uri);

/** \brief  Set the scheme value to \p val on the supplied \p uri */
int u_uri_set_scheme (u_uri_t *uri, const char *val);

/** \brief  Get the userinfo value from the supplied \p uri */
const char *u_uri_get_userinfo (u_uri_t *uri);

/** \brief  Set the userinfo value to \p val on the supplied \p uri */
int u_uri_set_userinfo (u_uri_t *uri, const char *val);

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

/** \brief  Get flags set by the parser (if any.) */
u_uri_flags_t u_uri_get_flags (u_uri_t *uri);

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
