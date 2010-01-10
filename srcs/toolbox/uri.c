/* 
 * Copyright (c) 2005-2010 by KoanLogic s.r.l. - All rights reserved.  
 */

#include <stdlib.h>
#include <string.h>

#include <toolbox/uri.h>
#include <toolbox/carpal.h>
#include <toolbox/misc.h>
#include <toolbox/memory.h>

struct u_uri_s
{
    char *scheme;
    char *user;
    char *pwd;
    char *host;
    char *port;
    char *path;
};

static int split(const char *s, size_t len, char c, char **left, char **right);
static int parse_middle(const char *s, size_t len, u_uri_t *uri);
static int parse_hostinfo(const char *s, size_t len, u_uri_t *uri);
static int parse_userinfo(const char *s, size_t len, u_uri_t *uri);

/**
    \defgroup uri URI
    \{
        The \ref uri module allows the parsing and validation of a subset 
        of URI strings defined in 
        <a href="http://www.ietf.org/rfc/rfc3986.txt">RFC 3986</a>, and 
        specifically (at present) an URI is tokenized up to the so-called
        \c hier-part, leaving the \c query and \c fragment unparsed, i.e.
        indistinguishable from the \c path.

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

/** \brief parse the URI string \a s and create an \c u_uri_t at \a *pu */
int u_uri_parse (const char *s, u_uri_t **pu)
{
    int i;
    const char *p, *p0;
    u_uri_t *uri = NULL;

    dbg_err_sif ((uri = u_zalloc(sizeof(u_uri_t))) == NULL);

    dbg_err_if ((p = strchr(s, ':')) == NULL); /* err if malformed */

    /* save schema string */
    dbg_err_if ((uri->scheme = u_strndup(s, p - s)) == NULL);

    p++; /* skip ':' */

    /* skip "//" */
    for (i = 0; i < 2; ++i, ++p)
        dbg_err_if (!p || *p == 0 || *p != '/'); /* err if malformed */

    /* save p */
    p0 = p;

    /* find the first path char ('/') or the end of the string */
    while (*p && *p != '/')
        ++p;

    /* parse userinfo and hostinfo */
    dbg_err_if ((p - p0) && parse_middle(p0, p - p0, uri));

    /* save path */
    dbg_err_if (*p && (uri->path = u_strdup(p)) == NULL);

    *pu = uri;

    return 0;
err:
    u_uri_free(uri);
    return ~0;
}

/** \brief dispose memory allocated to URI \a uri */
void u_uri_free (u_uri_t *uri)
{
    if (uri == NULL)
        return;

    U_FREE(uri->scheme);
    U_FREE(uri->user);
    U_FREE(uri->pwd);
    U_FREE(uri->host);
    U_FREE(uri->path);
    U_FREE(uri->port);
    U_FREE(uri);
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

/**
 *  \}
 */

/* split a string separated by 'c' in two substrings */
static int split(const char *s, size_t len, char c, char **left, char **right)
{
    char *buf = NULL;
    const char *p;
    char *l = NULL, *r = NULL;
    
    dbg_err_sif ((buf = u_strndup(s, len)) == NULL);

    if((p = strchr(buf, c)) != NULL)
    {
        dbg_err_sif ((l = u_strndup(s, p - buf)) == NULL);
        dbg_err_sif ((r = u_strndup(1 + p, len - (p - buf) - 1)) == NULL);
    } else {
        r = NULL;
        dbg_err_sif ((l = u_strndup(buf, len)) == NULL);
    }

    /* return result strings */
    *left = l;
    *right = r;

    U_FREE(buf);

    return 0;
err:
    U_FREE(buf);
    U_FREE(l);
    U_FREE(r);
    return ~0;
}

static int parse_userinfo(const char *s, size_t len, u_uri_t *uri)
{
    return split(s, len, ':', &uri->user, &uri->pwd);
}

static int parse_hostinfo(const char *s, size_t len, u_uri_t *uri)
{
    char *a, *b;

    /* check for numeric IPv6: "A host identified by an IPv6 literal address 
     * is represented inside the square brackets without a preceding version 
     * flag" */
    if ((a = strchr(s, '[')) && (b = strchr(s, ']')))
    {
        dbg_err_ifm (b <= a + 1, "malformed IPv6 URL: %s", s);
        dbg_err_sif ((uri->host = u_strndup(a + 1, b - a - 1)) == NULL);

        /* check for optional port */
        if (strlen(b) > 3)
        {
            if (b[1] == ':')
            {
                uri->port = u_strndup(b + 2, len - (b - a) - 2);
                dbg_err_if (uri->port == NULL);
            }
            else
                dbg_err("bad syntax for IPv6 address %s", s);     
        }
    }
    else    /* numeric IPv4 or host name */
        dbg_err_if (split(s, len, ':', &uri->host, &uri->port));

    return 0;
err:
    return ~0;
}

static int parse_middle(const char *s, size_t len, u_uri_t *uri)
{
    const char *p;

    if( (p = strchr(s, '@')) == NULL)
        return parse_hostinfo(s, len, uri);
    else
        return parse_userinfo(s, p - s, uri) +
            parse_hostinfo(1 + p, s + len - p - 1, uri);
}
