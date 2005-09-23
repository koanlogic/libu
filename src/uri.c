/* 
 * Copyright (c) 2005, KoanLogic s.r.l. - All rights reserved.  
 */

static const char rcsid[] =
    "$Id: uri.c,v 1.1.1.1 2005/09/23 13:04:38 tho Exp $";

#include <stdlib.h>
#include <string.h>

#include <u/uri.h>
#include <u/debug.h>
#include <u/misc.h>
#include <u/alloc.h>

/**
 *  \defgroup uri URI (rfc2396) Manipulation
 *  \{
 */

/* split a string separated by 'c' in two substrings */
static int split(const char *s, size_t len, char c, char **left, char **right)
{
    char *buf = 0;
    const char *p;
    char *l = 0, *r = 0;
    
    buf = u_strndup(s, len);
    if(!buf)
        goto err;

    if((p = strchr(buf, c)) != NULL)
    {
        l = u_strndup(s, p - buf);
        r = u_strndup(1 + p, len - (p - buf) - 1);
        if(!l || !r)
            goto err;
    } else {
        r = NULL;
        if((l = u_strndup(buf, len)) == NULL)
            goto err;
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
    char *port = 0;

    if(split(s, len, ':', &uri->host, &port))
        return ~0;

    if(port)
    {
        uri->port = atoi(port);
        U_FREE(port);
    }
    return 0;
}

static int parse_middle(const char *s, size_t len, u_uri_t *uri)
{
    const char *p;

    if( (p = strchr(s, '@')) == NULL)
        return parse_hostinfo(s, len, uri);
    else
        return parse_userinfo(s, p-s,uri) + parse_hostinfo(1+p, s+len-p-1, uri);
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
    U_FREE(uri);
}

/** \brief parse the URI string \a s and create an \c u_uri_t at \a *pu */
int u_uri_parse (const char *s, u_uri_t **pu)
{
    const char *p, *p0;
    int i;
    u_uri_t *uri;

    dbg_return_if ((uri = (u_uri_t*) u_zalloc(sizeof(u_uri_t))) == NULL, ~0);

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
    dbg_err_if (p - p0 && parse_middle(p0, p - p0, uri));

    /* save path */
    dbg_err_if (*p && (uri->path = u_strdup(p)) == NULL);

    *pu = uri;

    return 0;
err:
    u_uri_free(uri);
    return ~0;
}

/**
 *      \}
 */
