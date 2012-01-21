/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.  
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <toolbox/uri.h>
#include <toolbox/carpal.h>
#include <toolbox/misc.h>
#include <toolbox/memory.h>
#include <toolbox/lexer.h>

/* Internal representation of an URI value. */
struct u_uri_s
{
    unsigned int opts, flags;
    char scheme[U_TOKEN_SZ];
    char userinfo[U_TOKEN_SZ];
    char user[U_TOKEN_SZ], pwd[U_TOKEN_SZ];
    char authority[U_TOKEN_SZ];
    char host[U_TOKEN_SZ];
    char port[U_TOKEN_SZ];
    char path[U_TOKEN_SZ];
    char query[U_TOKEN_SZ];
    char fragment[U_TOKEN_SZ];
};

static int u_uri_parser (u_lexer_t *l, u_uri_opts_t opts, u_uri_t **pu);
static int u_uri_parse_scheme (u_lexer_t *l, u_uri_t *u);
static int u_uri_parse_hier_part (u_lexer_t *l, u_uri_t *u);
static int u_uri_parse_authority (u_lexer_t *l, u_uri_t *u);
static int u_uri_parse_query (u_lexer_t *l, u_uri_t *u);
static int u_uri_parse_fragment (u_lexer_t *l, u_uri_t *u);
static int u_uri_parse_abempty (u_lexer_t *l, u_uri_t *u);
static int u_uri_parse_userinfo (u_lexer_t *l, u_uri_t *u);
static int u_uri_parse_host (u_lexer_t *l, u_uri_t *u);
static int u_uri_parse_ipliteral (u_lexer_t *l);
static int u_uri_parse_ipv4address (u_lexer_t *l);
static int u_uri_parse_regname (u_lexer_t *l);
static int u_uri_expect_colon (u_lexer_t *l);
static int u_uri_expect_pct_encoded (u_lexer_t *l);
static int u_uri_expect_segment_nz (u_lexer_t *l);
static int u_uri_expect_path_abempty (u_lexer_t *l);
static int u_uri_query_first (u_lexer_t *l);
static int u_uri_fragment_first (u_lexer_t *l);
static int u_uri_path_first (u_lexer_t *l);
static int u_uri_ipliteral_first (u_lexer_t *l);
static int u_uri_ipv4address_first (u_lexer_t *l);
static int u_uri_regname_first (u_lexer_t *l);
static int u_uri_port_first (u_lexer_t *l);
static int u_uri_parse_port (u_lexer_t *l, u_uri_t *u);
static int u_uri_match_pchar (u_lexer_t *l);
static int u_uri_match_pchar_minus_at_sign (u_lexer_t *l);
static int u_uri_match_ups (u_lexer_t *l);
static int u_uri_adjust_greedy_match (u_lexer_t *l, char match[U_TOKEN_SZ]);
static int u_uri_knead_authority (u_uri_t *u, char s[U_URI_STRMAX]);
static int u_uri_crumble_user_password (u_uri_t *u);

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
    u_lexer_t *l = NULL;

    warn_err_if (u_lexer_new(uri, &l));
    warn_err_if (u_uri_parser(l, opts, pu));

    u_lexer_free(l), l = NULL;

    return 0;
err:
    u_lexer_free(l);
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
    dbg_return_if (s == NULL, ~0);

    /* see RFC 3986, Section 5.3. Component Recomposition */

    *s = '\0';

    if (strlen(u->scheme))
    {
        dbg_err_if (u_strlcat(s, u->scheme, U_URI_STRMAX));
        dbg_err_if (u_strlcat(s, ":", U_URI_STRMAX));
    }

    if (strlen(u->authority))
    {
        dbg_err_if (u_strlcat(s, "//", U_URI_STRMAX));
        dbg_err_if (u_strlcat(s, u->authority, U_URI_STRMAX));
    }
    else /* try recompose authority through its atoms */
        dbg_err_if (u_uri_knead_authority(u, s));

    dbg_err_if (u_strlcat(s, u->path, U_URI_STRMAX));

    if (strlen(u->query))
    {
        dbg_err_if (u_strlcat(s, "?", U_URI_STRMAX));
        dbg_err_if (u_strlcat(s, u->query, U_URI_STRMAX));
    }

    if (strlen(u->fragment))
    {
        dbg_err_if (u_strlcat(s, "#", U_URI_STRMAX));
        dbg_err_if (u_strlcat(s, u->fragment, U_URI_STRMAX));
    }
 
    return 0;
err:
    return ~0;
}

/** \brief  Print an ::u_uri_t object to \c stderr (DEBUG/TEST) */
void u_uri_print (u_uri_t *u, int extended)
{
    u_unused_args(extended);

    if (strlen(u->scheme))
        u_con("scheme: \"%s\"", u->scheme);

    if (strlen(u->userinfo))
    {
        u_con("userinfo: \"%s\"", u->userinfo);

        if (extended)
        {
            u_con("{");
            if (strlen(u->user))
                u_con("  user: \"%s\"", u->user);

            if (strlen(u->pwd))
                u_con("  pwd: \"%s\"", u->pwd);
            u_con("}");
        }
    }

    if (strlen(u->host))
        u_con("host: \"%s\"", u->host);

    if (strlen(u->port))
        u_con("port: \"%s\"", u->port);

    if (strlen(u->path))
        u_con("path: \"%s\"", u->path);

    if (strlen(u->query))
        u_con("query: \"%s\"", u->query);

    if (strlen(u->fragment))
        u_con("fragment: \"%s\"", u->fragment);

    return;
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
    u_uri_t *u = u_zalloc(sizeof *u);
    warn_err_sif (u == NULL);

    u->opts = opts;
    u->flags = U_URI_FLAGS_NONE;
    dbg_err_if (u_uri_set_path(u, "/"));    /* path is mandatory, so we set
                                               default value here */
    *pu = u;
    return 0;
err:
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
    if (u)
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
    dbg_err_if (u_strlcpy(uri->field, val, sizeof uri->field));     \
                                                                    \
    return 0;                                                       \
err:                                                                \
    return ~0;                                                      \
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

u_uri_flags_t u_uri_get_flags (u_uri_t *uri) { return uri->flags; }

/**
 *  \}
 */

static int u_uri_parser (u_lexer_t *l, u_uri_opts_t opts, u_uri_t **pu)
{
    u_uri_t *u = NULL;

    warn_err_sif (u_uri_new(opts, &u));

    /* Get URI scheme. */
    warn_err_if (u_uri_parse_scheme(l, u));

    /* Scheme and hier-part are separated by a ':'. */
    warn_err_if (u_uri_expect_colon(l));

    /* Hierarchical part (authority and/or path). */
    warn_err_if (u_uri_parse_hier_part(l, u));

    /* Optional query is introduced by a '?'. */
    if (u_uri_query_first(l))
        warn_err_if (u_uri_parse_query(l, u));

    /* Optional fragment is introduced by a '#'. */
    if (u_uri_fragment_first(l))
        warn_err_if (u_uri_parse_fragment(l, u));

    *pu = u;

    return 0;
err:
    u_uri_free(u);
    return ~0;
}

static int u_uri_expect_colon (u_lexer_t *l)
{
    return u_lexer_expect_char(l, ':');
}

static int u_uri_query_first (u_lexer_t *l)
{
    return (u_lexer_peek(l) == '?');
}

static int u_uri_fragment_first (u_lexer_t *l)
{
    return (u_lexer_peek(l) == '#');
}

static int u_uri_port_first (u_lexer_t *l)
{
    return (u_lexer_peek(l) == ':');
}

/* Check for both absolute and rootless paths. */
static int u_uri_path_first (u_lexer_t *l)
{
    return (u_lexer_peek(l) == '/' || u_uri_match_pchar(l));
}

/* scheme = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." ) */
static int u_uri_parse_scheme (u_lexer_t *l, u_uri_t *u)
{
    char c = u_lexer_peek(l);

    u_lexer_record_lmatch(l);

    if (!isalpha((int) c))
        U_LEXER_ERR(l, "Expect an alpha char, got \'%c\' instead.", c);

    do {
        U_LEXER_NEXT(l, &c); 
    } while (isalnum((int) c) || c == '+' || c == '-' || c == '.');

    u_lexer_record_rmatch(l);

    /* The match includes the first non-scheme char. */
    (void) u_lexer_get_match(l, u->scheme);
    u->scheme[strlen(u->scheme) - 1] = '\0';

    return 0;
err:
    return ~0;
}

/* "//" authority path-abempty */
static int u_uri_parse_authority (u_lexer_t *l, u_uri_t *u)
{
    char c, i = 0;

    /* Consume "//". */
    do {
        if ((c = u_lexer_peek(l)) != '/')
            U_LEXER_ERR(l, "Expect \'/\', got \'%c\' instead.", c);
        U_LEXER_NEXT(l, NULL);
    } while (++i < 2);

    /* [userinfo "@"] */
    if (strchr(u_lexer_lookahead(l), '@'))
        warn_err_if (u_uri_parse_userinfo(l, u));
    warn_err_if (u_uri_parse_host(l, u));

    /* Optional port is introduced by a ':' char. */
    if (u_uri_port_first(l))
        warn_err_if (u_uri_parse_port(l, u));

    /* path-abempty */
    warn_err_if (u_uri_parse_abempty(l, u));

    return 0;
err:
    return ~0;
}

/* Much more relaxed than requested: we want to let service names, not just
 * port numbers.  See /etc/services. */
static int u_uri_parse_port (u_lexer_t *l, u_uri_t *u)
{
    char c;

    if ((c = u_lexer_peek(l)) != ':')
        U_LEXER_ERR(l, "Expect \':\', got \'%c\'", c);

    /* Consume useless ':'. */
    U_LEXER_NEXT(l, &c);

    u_lexer_record_lmatch(l);

    while (isalnum((int) c) || c == '_' || c == '-' || c == '.')
        U_LEXER_NEXT(l, &c);

    u_lexer_record_rmatch(l);
    (void) u_uri_adjust_greedy_match(l, u->port);

    return 0;
err:
    return ~0;
}

static int u_uri_parse_host (u_lexer_t *l, u_uri_t *u)
{
    /* u_uri_parse_ipliteral() will override this. */
    u_lexer_record_lmatch(l);

    if (u_uri_ipv4address_first(l))
    {
        warn_err_if (u_uri_parse_ipv4address(l));
        u->flags |= U_URI_FLAGS_HOST_IS_IPADDRESS;
    }
    else if (u_uri_regname_first(l))
        warn_err_if (u_uri_parse_regname(l));
    else if (u_uri_ipliteral_first(l))
    {
        /* The left pointer is handled inside the ipliteral parser as it must 
         * take care (i.e. skip) of starting '['. */
        warn_err_if (u_uri_parse_ipliteral(l));

        u->flags |= U_URI_FLAGS_HOST_IS_IPADDRESS | 
                    U_URI_FLAGS_HOST_IS_IPLITERAL;
    }

    u_lexer_record_rmatch(l);

    (void) u_uri_adjust_greedy_match(l, u->host);

    return 0;
err:
    return ~0;
}

static int u_uri_adjust_greedy_match (u_lexer_t *l, char match[U_TOKEN_SZ])
{
    size_t mlen;

    dbg_return_if (l == NULL, ~0);
    dbg_return_if (match == NULL, ~0);

    (void) u_lexer_get_match(l, match);
    mlen = strlen(match);

    if (!u_lexer_eot(l))
        --mlen;

    /* Take care of the one-char-extra greedy match in case of IP-literal's.
     * Please note that "This [Internet Protocol literal address] is the only 
     * place where square bracket characters are allowed in the URI syntax",
     * i.e. the following is safe as it doesn't impact no URI component other
     * than the IP-literal. */
    if (mlen && match[mlen - 1] == ']')
        --mlen;

    match[mlen] = '\0';

    return 0;
}

static int u_uri_ipliteral_first (u_lexer_t *l)
{
    return (u_lexer_peek(l) == '[');
}

static int u_uri_parse_ipliteral (u_lexer_t *l)
{
    char c;

    if ((c = u_lexer_peek(l)) != '[')
        U_LEXER_ERR(l, "Expect \'[\', got \'%c\' instead.", c);

    /* Consume useless '[' and reset left match. */
    U_LEXER_NEXT(l, &c);
    u_lexer_record_lmatch(l);

    while (u_uri_match_pchar(l) && c != ']')
        U_LEXER_NEXT(l, &c);

    /* We need to reach here with lexer cursor over the ']' char. */
    if ((c = u_lexer_peek(l)) != ']')
        U_LEXER_ERR(l, "Expect \']\', got \'%c\' instead.", c);

    /* Consume ending ']' and go out. */
    U_LEXER_NEXT(l, NULL);

    return 0;
err:
    return ~0;
}

static int u_uri_ipv4address_first (u_lexer_t *l)
{
    return (isdigit(u_lexer_peek(l)));
}

static int u_uri_parse_ipv4address (u_lexer_t *l)
{
    char c;

    do {
        U_LEXER_NEXT(l, &c); 
    } while (isdigit(c) || c == '.');

    return 0;
err:
    return ~0;
}

static int u_uri_regname_first (u_lexer_t *l)
{
    return (u_uri_match_ups(l));
}

static int u_uri_parse_regname (u_lexer_t *l)
{
    do {
        U_LEXER_NEXT(l, NULL); 
    } while (u_uri_match_ups(l));

    return 0;
err:
    return ~0;
}

static int u_uri_parse_userinfo (u_lexer_t *l, u_uri_t *u)
{
    char c;

    u_lexer_record_lmatch(l);

    do {
        U_LEXER_NEXT(l, &c);
    } while (u_uri_match_pchar_minus_at_sign(l));

    if (u_lexer_peek(l) != '@')
        U_LEXER_ERR(l, "Expect \'@\', got \'%c\' instead.", c);

    u_lexer_record_rmatch(l);
    (void) u_uri_adjust_greedy_match(l, u->userinfo);

    /* 3.2.1.  User Information: Use of the format "user:password" in the 
     * userinfo field is deprecated.  Anyway... */
    if (!(u->opts & U_URI_OPT_DONT_PARSE_USERINFO))
        dbg_if (u_uri_crumble_user_password(u)); 

    /* Consume '@' and go out. */
    U_LEXER_NEXT(l, NULL);

    return 0;
err:
    return ~0;
}

/* TODO: refine ! */
static int u_uri_crumble_user_password (u_uri_t *u)
{
    char c;
    u_lexer_t *l = NULL;

    dbg_return_if (!strlen(u->userinfo), ~0);

    /* Create a disposable lexer. */
    dbg_err_if (u_lexer_new(u->userinfo, &l));

    /* User name. */
    u_lexer_record_lmatch(l);

    /* Assume there is at least one char available. */
    do u_lexer_next(l, &c); while (c != ':' && !u_lexer_eot(l));

    u_lexer_record_rmatch(l);
    (void) u_uri_adjust_greedy_match(l, u->user);

    /* Check if we've exhausted the userinfo string. */
    nop_goto_if (u_lexer_eot(l), end);

    /* Skip ':'. */
    U_LEXER_NEXT(l, NULL);

    /* Password. */
    u_lexer_record_lmatch(l);

    do u_lexer_next(l, &c); while (!u_lexer_eot(l));

    u_lexer_record_rmatch(l);
    (void) u_uri_adjust_greedy_match(l, u->pwd);

end:
    u_lexer_free(l);
    return 0;
err:
    u_lexer_free(l);
    return ~0;
}

static int u_uri_parse_abempty (u_lexer_t *l, u_uri_t *u)
{
    u_lexer_record_lmatch(l);
    warn_err_if (u_uri_expect_path_abempty(l));
    u_lexer_record_rmatch(l);
    (void) u_uri_adjust_greedy_match(l, u->path);

    return 0;
err:
    return ~0;
}

/* ['/'] segment-nz path-abempty 
 * Absorbs both rootless and absolute paths. */
static int u_uri_parse_path (u_lexer_t *l, u_uri_t *u)
{
    u_lexer_record_lmatch(l);

    if (u_lexer_peek(l) == '/')
        U_LEXER_NEXT(l, NULL);
    
    warn_err_if (u_uri_expect_segment_nz(l));
    warn_err_if (u_uri_expect_path_abempty(l));

    u_lexer_record_rmatch(l);
    (void) u_uri_adjust_greedy_match(l, u->path);

    return 0;
err:
    return ~0;
}

/* See RFC3986 Appendix A. */
static int u_uri_parse_hier_part (u_lexer_t *l, u_uri_t *u)
{
    /* We need to look ahead two chars to see if we have the '//' string that 
     * introduces the authority token. */
    if (!strncmp(u_lexer_lookahead(l), "//", strlen("//")))
        warn_err_if (u_uri_parse_authority(l, u));
    else if (u_uri_path_first(l))
        warn_err_if (u_uri_parse_path(l, u));
    else /* Path empty. */
        u->path[0] = '\0';

    return 0;
err:
    return ~0;
}

static int u_uri_parse_query (u_lexer_t *l, u_uri_t *u)
{
    char c;

    if ((c = u_lexer_peek(l)) != '?')
        U_LEXER_ERR(l, "Expect \'?\', got \'%c\' instead.", c);

    /* Consume the starting '?' char. */
    U_LEXER_NEXT(l, &c);
    u_lexer_record_lmatch(l);

    while (c == '/' || c == '?' || u_uri_match_pchar(l))
        U_LEXER_NEXT(l, &c);

    u_lexer_record_rmatch(l);
    (void) u_uri_adjust_greedy_match(l, u->query);

    return 0;
err:
    return ~0;
}

/* Same as query. */
static int u_uri_parse_fragment (u_lexer_t *l, u_uri_t *u)
{
    char c;

    if ((c = u_lexer_peek(l)) != '#')
        U_LEXER_ERR(l, "Expect \'#\', got \'%c\' instead.", c);

    /* Consume the starting '#' char. */
    U_LEXER_NEXT(l, &c);
    u_lexer_record_lmatch(l);

    while (c == '/' || c == '?' || u_uri_match_pchar(l))
        U_LEXER_NEXT(l, &c);

    u_lexer_record_rmatch(l);
    (void) u_lexer_get_match(l, u->fragment);

    return 0;
err:
    return ~0;
}

/* unreserved / pct-encoded / sub-delims */
static int u_uri_match_ups (u_lexer_t *l)
{
    char c;

    switch ((c = u_lexer_peek(l)))
    {
        /* pct-encoded */
        case '%':
            warn_err_if (u_uri_expect_pct_encoded(l));

        /* unreserved */
        case '-': case '.': case '_': case '~': 

        /* sub-delims */
        case '!': case '$': case '&': case '\'': case '(':
        case ')': case '*': case '+': case ',': case ';': 
        case '=':
            return 1;

        /* ALPHA / DIGIT */
        default:
            return isalnum((int) c);
    }

    /* fallthrough */
err:
    return 0;
}

static int u_uri_match_pchar (u_lexer_t *l)
{
    switch (u_lexer_peek(l))
    {
        case '@':
            return 1;
        default:
            return u_uri_match_pchar_minus_at_sign(l);
    }
}

static int u_uri_match_pchar_minus_at_sign (u_lexer_t *l)
{
    switch (u_lexer_peek(l))
    {
        case ':':
            return 1;
        default:
            return u_uri_match_ups(l);
    }
}

static int u_uri_expect_pct_encoded (u_lexer_t *l)
{
    char i, c;

    if (u_lexer_peek(l) != '%')
        U_LEXER_ERR(l, "Expect \'%%\', got \'%c\' instead.", c);

    for (i = 0; i < 2; i++)
    {
        U_LEXER_NEXT(l, &c);

        if (!isxdigit((int) c))
            U_LEXER_ERR(l, "Non hex digit \'%c\' in percent encoding.", c);
    }

    return 0;
err:
    return ~0;
}

/* 1*pchar */
static int u_uri_expect_segment_nz (u_lexer_t *l)
{
    /* Expect at least one pchar. */
    if (!u_uri_match_pchar(l))
        U_LEXER_ERR(l, "Expect a pchar, got \'%c\' instead.", u_lexer_peek(l));

    do { U_LEXER_NEXT(l, NULL); } while (u_uri_match_pchar(l));

    return 0;
err:
    return ~0;
}

/* *pchar */
static int u_uri_expect_segment (u_lexer_t *l)
{
    do { U_LEXER_NEXT(l, NULL); } while (u_uri_match_pchar(l));

    return 0;
err:
    return ~0;
}

/* *("/" segment) */
static int u_uri_expect_path_abempty (u_lexer_t *l)
{
    /* Could be empty. */
    if (u_lexer_peek(l) != '/')
        return 0;

    /* Consume '/'-separated segments. */
    do { u_uri_expect_segment(l); } while (u_lexer_peek(l) == '/'); 

    return 0;
}

static int u_uri_knead_authority (u_uri_t *u, char s[U_URI_STRMAX])
{
    dbg_return_if (u == NULL, ~0);
    dbg_return_if (strlen(u->host) == 0, ~0);
    dbg_return_if (s == NULL, ~0);

    /* If host is IPv6 literal, automatically add the IPLITERAL flag.
     * XXX Lousy test, we'd better parse it through u_uri_parse_ipliteral() */
    if (strchr(u->host, ':'))
        u->flags |= U_URI_FLAGS_HOST_IS_IPLITERAL;

    dbg_err_if (u_strlcat(s, "//", U_URI_STRMAX));

    if (strlen(u->userinfo))
    {
        dbg_err_if (u_strlcat(s, u->userinfo, U_URI_STRMAX));
        dbg_err_if (u_strlcat(s, "@", U_URI_STRMAX));
    }
    else if (strlen(u->user))
    {
        dbg_err_if (u_strlcat(s, u->user, U_URI_STRMAX)); 

        if (strlen(u->pwd))
        {
            dbg_err_if (u_strlcat(s, ":", U_URI_STRMAX)); 
            dbg_err_if (u_strlcat(s, u->pwd, U_URI_STRMAX)); 
        }

        dbg_err_if (u_strlcat(s, "@", U_URI_STRMAX));
    }

    if (u->flags & U_URI_FLAGS_HOST_IS_IPLITERAL)
        dbg_err_if (u_strlcat(s, "[", U_URI_STRMAX));

    dbg_err_if (u_strlcat(s, u->host, U_URI_STRMAX));

    if (u->flags & U_URI_FLAGS_HOST_IS_IPLITERAL)
        dbg_err_if (u_strlcat(s, "]", U_URI_STRMAX));

    if (strlen(u->port))
    {
        dbg_err_if (u_strlcat(s, ":", U_URI_STRMAX));
        dbg_err_if (u_strlcat(s, u->port, U_URI_STRMAX));
    }

    return 0;
err:
    return ~0;
}


