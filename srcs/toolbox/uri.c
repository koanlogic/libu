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
    char *user;
    char *pwd;
    char *host;
    char *port;
    char *path;
    char *query;
    char *fragment;
};

static int u_uri_fill (u_uri_t *u, const char *uri, regmatch_t pmatch[10]);

/**
    \defgroup uri URI
    \{
        The \ref uri module allows the parsing and validation of URI strings as
        defined in <a href="http://www.ietf.org/rfc/rfc3986.txt">RFC 3986</a>.

        Usage is really simple and goes like this:
    \code
    u_uri_t *u = NULL;
    const char *uri = "http://me@[2001:db8::7]:8080/path/to/me/public_html";
    const char *user, *scheme;

    // parse and tokenize the uri string
    dbg_err_if (u_uri_parse(uri, &u));

    // get tokens you are interested in, e.g:
    dbg_err_if ((scheme = u_uri_scheme(u)) == NULL);
    dbg_err_if ((user = u_uri_user(u)) == NULL);
    ...
    // free the uri object once you are done with it
    u_uri_free(u);
    ...
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
 *  \retval 0   on success
 *  \retval ~0  on error
 */
int u_uri_parse (const char *uri, u_uri_t **pu)
{
    u_uri_t *u = NULL;
    int rc = 0;
    char es[1024];
    regex_t re;
    regmatch_t pmatch[10];

    dbg_err_if ((rc = regcomp(&re, uri_pat, REG_EXTENDED)));
    dbg_err_if ((rc = regexec(&re, uri, 10, pmatch, 0)));

    dbg_err_sif ((u = u_zalloc(sizeof(u_uri_t))) == NULL);
    dbg_err_if (u_uri_fill(u, uri, pmatch));

    regfree(&re);

    *pu = u;

    return 0;
err:
    if (rc)
    {
        regerror(rc, &re, es, sizeof es);
        dbg("%s", es);
    }
    regfree(&re);

    if (u)
        u_uri_free(u);

    return ~0;
}

static int u_uri_fill (u_uri_t *u, const char *uri, regmatch_t pmatch[10])
{
    size_t i, ms_len;
    char ms[4096];

    for (i = 0; i < 10; ++i)
    {
        if (pmatch[i].rm_so == -1)
        {
            dbg("[%zu] __NO_MATCH__", i);
            continue;
        }

        ms_len = pmatch[i].rm_eo - pmatch[i].rm_so + 1;
        dbg_if (u_strlcpy(ms, uri + pmatch[i].rm_so, ms_len));

        switch (i)
        {
            case 0: case 1: case 3: case 8:
                break;
            case 2: /* scheme */
                dbg_err_sif ((u->scheme = u_strdup(ms)) == NULL);
                break;
            case 4: /* authority */
                /* TODO break down authority string */
                dbg("TODO break authority: %s", ms); 
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

/** \brief dispose memory allocated to URI \a uri */
void u_uri_free (u_uri_t *uri)
{
    if (uri == NULL)
        return;

    u_free(uri->scheme);
    u_free(uri->user);
    u_free(uri->pwd);
    u_free(uri->host);
    u_free(uri->path);
    u_free(uri->port);
    u_free(uri->fragment);
    u_free(uri->query);
    u_free(uri);
}

/** \brief  return the \c scheme string of the parsed \p uri */
const char *u_uri_scheme (u_uri_t *uri)
{
    dbg_return_if (uri == NULL, NULL);
    return uri->scheme;
}

/** \brief  return the \c user string of the parsed \p uri */
const char *u_uri_user (u_uri_t *uri)
{
    dbg_return_if (uri == NULL, NULL);
    return uri->user;
}

/** \brief  return the \c password string of the parsed \p uri */
const char *u_uri_pwd (u_uri_t *uri)
{
    dbg_return_if (uri == NULL, NULL);
    return uri->pwd;
}

/** \brief  return the \c host string of the parsed \p uri */
const char *u_uri_host (u_uri_t *uri)
{
    dbg_return_if (uri == NULL, NULL);
    return uri->host;
}

/** \brief  return the \c path string of the parsed \p uri */
const char *u_uri_path (u_uri_t *uri)
{
    dbg_return_if (uri == NULL, NULL);
    return uri->path;
}

/** \brief  return the \c port string of the parsed \p uri */
const char *u_uri_port (u_uri_t *uri)
{
    dbg_return_if (uri == NULL, NULL);
    return uri->port;
}

/** \brief  return the \c query string of the parsed \p uri */
const char *u_uri_query (u_uri_t *uri)
{
    dbg_return_if (uri == NULL, NULL);
    return uri->query;
}

/** \brief  return the \c fragment string of the parsed \p uri */
const char *u_uri_fragment (u_uri_t *uri)
{
    dbg_return_if (uri == NULL, NULL);
    return uri->fragment;
}

/**
 *  \}
 */

