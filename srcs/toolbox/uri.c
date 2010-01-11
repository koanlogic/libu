/* 
 * Copyright (c) 2005-2010 by KoanLogic s.r.l. - All rights reserved.  
 */

#include <stdlib.h>
#include <string.h>

#include <regex.h>

#include <toolbox/uri.h>
#include <toolbox/carpal.h>
#include <toolbox/misc.h>
#include <toolbox/memory.h>

/* See RFC 3986, Appendix B. */
const char *uri_pat = 
    "^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?" ;

struct u_uri_s
{
    char *scheme;
    char *user, *pwd, *host, *port; /* possible 'authority' pieces */
    char *authority;
    char *path;
    char *query;
    char *fragment;
};

static int u_uri_fill (u_uri_t *u, const char *uri, regmatch_t pmatch[10]);
static int split_authority (const char *authority, u_uri_t *u);

/**
    \defgroup uri URI
    \{
        The \ref uri module allows the parsing and validation of URI strings as
        defined in <a href="http://www.ietf.org/rfc/rfc3986.txt">RFC 3986</a>.

        It can be used for parsing a given uri string:
    \code
    u_uri_t *u = NULL;
    const char *uri = "ftp://ftp.is.co.za/rfc/rfc1808.txt";
    const char *authority, *scheme;

    // parse and tokenize the uri string
    dbg_err_if (u_uri_parse(uri, &u));

    // get tokens you are interested in, e.g:
    dbg_err_if ((scheme = u_uri_get_scheme(u)) == NULL);
    dbg_err_if ((authority = u_uri_get_authority(u)) == NULL);
    
    // should give "ftp '://' ftp.is.co.za"
    con("%s \'://\' %s", scheme, authority);
    ...
    // free the uri object once you are done with it
    u_uri_free(u);
    \endcode
        Or, the way round, to build a URI string starting from its components:
    \code
    u_uri_t *u = NULL;
    char s[U_URI_STRMAX];

    // make room for the new URI object
    dbg_err_if (u_uri_new(&u));

    // set the relevant attributes
    (void) u_uri_set_scheme(u, "http");
    (void) u_uri_set_authority(u, "www.ietf.org");
    (void) u_uri_set_path(u, "rfc/rfc3986.txt");

    // encode it to string 's'
    dbg_err_if (u_uri_unparse(u, s));

    // should give: http://www.ietf.org/rfc/rfc3986.txt
    con("%s", s);
    ...
    u_uri_free(u);
    \endcode
        \note ::u_uri_t objects are building blocks in the \ref net module.
 */

/** 
 *  \brief Parse an URI string and create the corresponding ::u_uri_t object 
 *
 *  Parse the NUL-terminated string \p uri and create an ::u_uri_t object at 
 *  \p *pu 
 *
 *  \param  uri the NUL-terminated string that must be parsed
 *  \param  pu  the newly created ::u_uri_t object containing the b
 *
 *  \retval  0  on success
 *  \retval ~0  on error
 */
int u_uri_parse (const char *uri, u_uri_t **pu)
{
    u_uri_t *u = NULL;
    int rc = 0;
    char es[1024];
    regex_t re;
    regmatch_t pmatch[10];

    dbg_return_if (uri == NULL, ~0);
    dbg_return_if (pu == NULL, ~0);

    dbg_err_if ((rc = regcomp(&re, uri_pat, REG_EXTENDED)));
    dbg_err_if ((rc = regexec(&re, uri, 10, pmatch, 0)));

    dbg_err_if (u_uri_new(&u));
    dbg_err_if (u_uri_fill(u, uri, pmatch));

    regfree(&re);

    *pu = u;

    return 0;
err:
    if (rc)
    {
        regerror(rc, &re, es, sizeof es);
        dbg("%s: %s", uri, es);
    }
    regfree(&re);

    if (u)
        u_uri_free(u);

    return ~0;
}

/**
 *  \brief  Assemble an URI string starting from its atoms
 *
 *  Assemble an URI string at \p s, starting from its pieces stored in the
 *  supplied ::u_uri_t object \p u
 *      
 *  \param  u   reference to an already filled in ::u_uri_t object
 *  \param  s   reference to an already alloc'd string of size ::U_URI_STRMAX
 *
 *  \retval  0  on success
 *  \retval ~0  on error
 */ 
int u_uri_unparse (u_uri_t *u, char s[U_URI_STRMAX])
{
    dbg_return_if (u == NULL, ~0);
    dbg_return_if (u->path == NULL, ~0);    /* path is mandatory */
    dbg_return_if (s == NULL, ~0);

    /* see RFC 3986, Section 5.3. Component Recomposition */

    *s = '\0';

    if (u->scheme)
    {
        dbg_err_if (u_strlcat(s, u->scheme, U_URI_STRMAX));
        dbg_err_if (u_strlcat(s, ":", U_URI_STRMAX));
    }

    if (u->authority)
    {
        dbg_err_if (u_strlcat(s, "//", U_URI_STRMAX));
        dbg_err_if (u_strlcat(s, u->authority, U_URI_STRMAX));
    }
    else /* try recompose authority through its atoms */
    {
        dbg("TODO");
    }

    dbg_err_if (u_strlcat(s, u->path, U_URI_STRMAX));

    if (u->query)
    {
        dbg_err_if (u_strlcat(s, "?", U_URI_STRMAX));
        dbg_err_if (u_strlcat(s, u->query, U_URI_STRMAX));
    }

    if (u->fragment)
    {
        dbg_err_if (u_strlcat(s, "#", U_URI_STRMAX));
        dbg_err_if (u_strlcat(s, u->fragment, U_URI_STRMAX));
    }
 
    return 0;
err:
    return ~0;
}

/**
 *  \brief  Make room for a new ::u_uri_t object
 *
 *  Make room for a new ::u_uri_t object at \p *pu.  The returned object is 
 *  completely empty: use the needed setter methods to fill it before passing
 *  it to the encoder.
 *
 *  \param  pu  Reference to an ::u_uri_t that, on success, will point to the 
 *              newly created object
 *
 *  \retval  0  on success
 *  \retval ~0  on error
 */ 
int u_uri_new (u_uri_t **pu)
{
    u_uri_t *u = NULL;

    dbg_return_if (pu == NULL, ~0);

    dbg_err_sif ((u = u_zalloc(sizeof(u_uri_t))) == NULL);

    u->scheme = NULL;
    u->user = NULL;
    u->pwd = NULL;
    u->host = NULL;
    u->port = NULL;
    u->authority = NULL;
    u->path = NULL;
    u->query = NULL;
    u->fragment = NULL;

    *pu = u;

    return 0;
err:
    if (u)
        u_uri_free(u);
    return ~0;
}

/**
 *  \brief dispose memory allocated to \p uri
 *
 *  Free the previously (via ::u_uri_parse or ::u_uri_new) allocated 
 *  ::u_uri_t object \p u.
 *
 *  \param  u   reference to the ::u_uri_t object that needs to be disposed
 *
 *  \return nothing
 */ 
void u_uri_free (u_uri_t *u)
{
    dbg_return_if (u == NULL, )

    U_FREE(u->scheme);
    U_FREE(u->user);
    U_FREE(u->pwd);
    U_FREE(u->host);
    U_FREE(u->port);
    U_FREE(u->authority);
    U_FREE(u->path);
    U_FREE(u->query);
    U_FREE(u->fragment);
    u_free(u);
}

#define U_URI_GETSET_F(field)                                       \
const char *u_uri_get_##field (u_uri_t *uri)                        \
{                                                                   \
    dbg_return_if (uri == NULL, NULL);                              \
    return uri->field;                                              \
}                                                                   \
                                                                    \
int u_uri_set_##field (u_uri_t *uri, const char *val)               \
{                                                                   \
    dbg_return_if (uri == NULL, ~0);                                \
    dbg_return_if (val == NULL, ~0);                                \
                                                                    \
    dbg_return_if ((uri->field = u_strdup(val)) == NULL, ~0);       \
                                                                    \
    return 0;                                                       \
}

U_URI_GETSET_F(scheme)
U_URI_GETSET_F(user)
U_URI_GETSET_F(pwd)
U_URI_GETSET_F(host)
U_URI_GETSET_F(port)
U_URI_GETSET_F(authority)
U_URI_GETSET_F(path)
U_URI_GETSET_F(query)
U_URI_GETSET_F(fragment)

/** \brief  Print an ::u_uri_t object to \c stderr (DEBUG) */
void u_uri_print (u_uri_t *u)
{
    dbg_return_if (u == NULL, );

    con("scheme: %s", u->scheme ? u->scheme : "__NOT_SET__");
    con("user: %s", u->user ? u->user : "__NOT_SET__");
    con("pwd: %s", u->pwd ? u->pwd : "__NOT_SET__");
    con("host: %s", u->host ? u->host : "__NOT_SET__");
    con("port: %s", u->port ? u->port : "__NOT_SET__");
    con("authority: %s", u->authority ? u->authority : "__NOT_SET__");
    con("path: %s", u->path ? u->path : "__NOT_SET__");
    con("query: %s", u->query ? u->query : "__NOT_SET__");
    con("fragment: %s", u->fragment ? u->fragment : "__NOT_SET__");

    return;
}

/**
 *  \}
 */

static int u_uri_fill (u_uri_t *u, const char *uri, regmatch_t pmatch[10])
{
    size_t i, ms_len;
    char ms[U_URI_STRMAX];

    for (i = 0; i < 10; ++i)
    {
        /* skip greedy matches */
        switch (i)
        {
            case 0: case 1: case 3: case 6: case 8:
                continue;
        }

        /* skip unmatched regex tokens */
        if (pmatch[i].rm_so == -1)
            continue;

        /* prepare a NUL-terminated string with the token that was matched */
        ms_len = U_MIN(pmatch[i].rm_eo - pmatch[i].rm_so + 1, sizeof ms);
        dbg_if (u_strlcpy(ms, uri + pmatch[i].rm_so, ms_len));

        switch (i)
        {
            case 2: /* scheme */
                dbg_err_sif ((u->scheme = u_strdup(ms)) == NULL);
                break;
            case 4: /* authority */
                dbg_err_sif ((u->authority = u_strdup(ms)) == NULL);
                dbg_err_if (split_authority(ms, u));
                break;
            case 5: /* path */
                dbg_err_sif ((u->path = u_strdup(ms)) == NULL);
                break;
            case 7: /* query */
                dbg_err_sif ((u->query = u_strdup(ms)) == NULL);
                break;
            case 9: /* fragment */
                dbg_err_sif ((u->fragment = u_strdup(ms)) == NULL);
                break;
        }
    }

    return 0;
err:
    return ~0;
}

/* TODO */
static int split_authority (const char *authority, u_uri_t *u)
{
    dbg_return_if (u == NULL, ~0);

    if (authority == NULL)
        return 0;

    dbg("TODO");

    return 0;
}
