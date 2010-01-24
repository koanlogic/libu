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
    char *userinfo, *user, *pwd, *host, *port; /* 'authority' pieces */
    char *authority;
    char *path;
    char *query;
    char *fragment;
    int opts;
};

static int u_uri_fill (u_uri_t *u, const char *uri, regmatch_t pmatch[10]);
static int knead_authority (u_uri_t *u, char s[U_URI_STRMAX]);
static int parse_authority (char *authority, u_uri_t *u);
static int parse_userinfo (char *userinfo, u_uri_t *u);

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
    // note that no u_uri_opts_t have been supplied
    dbg_err_if (u_uri_crumble(uri, 0, &u));

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
    dbg_err_if (u_uri_new(0, &u));

    // set the relevant attributes
    (void) u_uri_set_scheme(u, "http");
    (void) u_uri_set_authority(u, "www.ietf.org");
    (void) u_uri_set_path(u, "rfc/rfc3986.txt");

    // encode it to string 's'
    dbg_err_if (u_uri_knead(u, s));

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
 *  \param  uri     the NUL-terminated string that must be parsed
 *  \param  opts    bitmask of or'ed ::u_uri_opts_t values
 *  \param  pu      the newly created ::u_uri_t object containing the b
 *
 *  \retval  0  on success
 *  \retval ~0  on error
 */
int u_uri_crumble (const char *uri, u_uri_opts_t opts, u_uri_t **pu)
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

    dbg_err_if (u_uri_new(opts, &u));
    dbg_err_if (u_uri_fill(u, uri, pmatch));

    regfree(&re);

    *pu = u;

    return 0;
err:
    if (rc)
    {
        regerror(rc, &re, es, sizeof es);
        u_dbg("%s: %s", uri, es);
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
int u_uri_knead (u_uri_t *u, char s[U_URI_STRMAX])
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
        dbg_err_if (knead_authority(u, s));

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
 *  \param  opts    bitmask of or'ed ::u_uri_opts_t values
 *  \param  pu      Reference to an ::u_uri_t that, on success, will point to 
 *                  the newly created object
 *
 *  \retval  0  on success
 *  \retval ~0  on error
 */ 
int u_uri_new (u_uri_opts_t opts, u_uri_t **pu)
{
    u_uri_t *u = NULL;

    dbg_return_if (pu == NULL, ~0);

    dbg_err_sif ((u = u_zalloc(sizeof(u_uri_t))) == NULL);

    u->scheme = NULL;
    u->userinfo = NULL;
    u->user = NULL;
    u->pwd = NULL;
    u->host = NULL;
    u->port = NULL;
    u->authority = NULL;
    u->path = NULL;
    u->query = NULL;
    u->fragment = NULL;
    u->opts = opts;

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
 *  Free the previously (via ::u_uri_crumble or ::u_uri_new) allocated 
 *  ::u_uri_t object \p u.
 *
 *  \param  u   reference to the ::u_uri_t object that needs to be disposed
 *
 *  \return nothing
 */ 
void u_uri_free (u_uri_t *u)
{
    dbg_return_if (u == NULL, );

    U_FREE(u->scheme);
    U_FREE(u->userinfo);
    U_FREE(u->user);
    U_FREE(u->pwd);
    U_FREE(u->host);
    U_FREE(u->port);
    U_FREE(u->authority);
    U_FREE(u->path);
    U_FREE(u->query);
    U_FREE(u->fragment);
    u_free(u);

    return;
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
    U_FREE(uri->field);                                             \
    dbg_return_sif ((uri->field = u_strdup(val)) == NULL, ~0);      \
                                                                    \
    return 0;                                                       \
}

U_URI_GETSET_F(scheme)
U_URI_GETSET_F(userinfo)
U_URI_GETSET_F(user)
U_URI_GETSET_F(pwd)
U_URI_GETSET_F(host)
U_URI_GETSET_F(port)
U_URI_GETSET_F(authority)
U_URI_GETSET_F(path)
U_URI_GETSET_F(query)
U_URI_GETSET_F(fragment)

/** \brief  Print an ::u_uri_t object to \c stderr (DEBUG/TEST) */
void u_uri_print (u_uri_t *u, int extended)
{
    dbg_return_if (u == NULL, );

    u_con("   scheme: %s", u->scheme ? u->scheme : "");
    u_con("authority: %s", u->authority ? u->authority : "");
    u_con("     path: %s", u->path ? u->path : "");
    u_con("    query: %s", u->query ? u->query : "");
    u_con(" fragment: %s", u->fragment ? u->fragment : "");

    if (extended)
    {
        u_con(" userinfo: %s", u->userinfo ? u->userinfo : "");
        u_con("     user: %s", u->user ? u->user : "");
        u_con("      pwd: %s", u->pwd ? u->pwd : "");
        u_con("     host: %s", u->host ? u->host : "");
        u_con("     port: %s", u->port ? u->port : "");
    }

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
        ms_len = 
            U_MIN((size_t) pmatch[i].rm_eo - pmatch[i].rm_so + 1, sizeof ms);
        (void) u_strlcpy(ms, uri + pmatch[i].rm_so, ms_len);

        switch (i)
        {
            case 2: /* scheme */
                dbg_err_sif ((u->scheme = u_strdup(ms)) == NULL);
                break;
            case 4: /* authority */
                dbg_err_sif ((u->authority = u_strdup(ms)) == NULL);
                dbg_err_if (parse_authority(ms, u));
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

/*  authority = [ userinfo "@" ] host [ ":" port ] */
static int parse_authority (char *authority, u_uri_t *u)
{
    char *s, *cur, *end;
    char userinfo[U_URI_STRMAX];
    size_t len;

    dbg_return_if (u == NULL, ~0);

    if (u->opts & U_URI_OPT_DONT_PARSE_AUTHORITY)
        return 0;

    s = cur = authority;
    dbg_err_if ((end = strchr(authority, '\0')) == s);

    /* check for optional userinfo */
    if ((cur = strchr(s, '@')))
    {
        len = U_MIN((size_t) (cur - s + 1), sizeof userinfo);
        (void) u_strlcpy(userinfo, s, len);
        dbg_err_if (parse_userinfo(userinfo, u));
        dbg_err_ifm ((s = cur + 1) == end, "empty host");
    }

    /* host = IP-literal / IPv4address / reg-name */
    if (*s == '[')  
    {
        /* IP-literal = "[" IPv6address / IPvFuture case "]" */
        dbg_err_ifm ((cur = strchr(s, ']')) == NULL, 
                "miss closing ] in IP-literal");
        dbg_err_ifm (cur == s + 1, "empty IPv6address / IPvFuture");

        dbg_err_sif ((u->host = u_strndup(s + 1, cur - s - 1)) == NULL);
        ++cur;
    }
    else
    {
        /* IPv4address / reg-name, as-is */

        if ((cur = strchr(s, ':')) == NULL)
        {
            dbg_err_sif ((u->host = u_strdup(s)) == NULL);
            goto end;
        }
        else
        {
            dbg_err_ifm (cur == s, "empty IPv4address / reg-name");
            dbg_err_sif ((u->host = u_strndup(s, cur - s)) == NULL);
        }
    }

    /* check if we've reached the end of the authority string, otherwise
     * pretend that next character is the port marker */
    nop_goto_if ((s = cur) == end, end);
    dbg_err_ifm (*s != ':', "bad syntax in authority string");
    dbg_err_ifm (++s == end, "empty port");
    dbg_err_sif ((u->port = u_strdup(s)) == NULL);

end:
    return 0;
err:
    return ~0;
}

static int parse_userinfo (char *userinfo, u_uri_t *u)
{
    char *p;

    dbg_return_if (u == NULL, ~0);

    if (userinfo == NULL)
        return 0;

    if (u->opts & U_URI_OPT_DONT_PARSE_USERINFO)
    {
        /* dup it as-is to .userinfo */
        dbg_err_sif ((u->userinfo = u_strdup(userinfo)) == NULL);
        return 0;
    }

    /* 3.2.1.  User Information: Use of the format "user:password" in the 
     * userinfo field is deprecated.  
     * Anyway :) */
    if ((p = strchr(userinfo, ':')) != NULL)
    {
        dbg_err_ifm (p == userinfo, "no user in userinfo");
        dbg_err_sif ((u->user = u_strndup(userinfo, p - userinfo)) == NULL);
        ++p;
        dbg_err_sif ((u->pwd = u_strdup(p)) == NULL);
    }
    else
        dbg_err_sif ((u->user = u_strdup(userinfo)) == NULL);

    return 0;
err:
    return ~0;
}

static int knead_authority (u_uri_t *u, char s[U_URI_STRMAX])
{
    dbg_return_if (u == NULL, ~0);
    dbg_return_if (u->host == NULL, ~0);
    dbg_return_if (s == NULL, ~0);

    if (u->userinfo)
    {
        dbg_err_if (u_strlcat(s, u->userinfo, U_URI_STRMAX));
        dbg_err_if (u_strlcat(s, "@", U_URI_STRMAX));
    }
    else if (u->user)
    {
        dbg_err_if (u_strlcat(s, u->user, U_URI_STRMAX)); 

        if (u->pwd)
        {
            dbg_err_if (u_strlcat(s, ":", U_URI_STRMAX)); 
            dbg_err_if (u_strlcat(s, u->pwd, U_URI_STRMAX)); 
        }

        dbg_err_if (u_strlcat(s, "@", U_URI_STRMAX));
    }

    dbg_err_if (u_strlcat(s, u->host, U_URI_STRMAX));

    if (u->port)
    {
        dbg_err_if (u_strlcat(s, ":", U_URI_STRMAX));
        dbg_err_if (u_strlcat(s, u->port, U_URI_STRMAX));
    }

    return 0;
err:
    return ~0;
}
