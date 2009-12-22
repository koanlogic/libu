/* $Id: net.c,v 1.11 2009/12/22 16:46:46 tho Exp $ */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include <toolbox/net.h>
#include <toolbox/uri.h>
#include <toolbox/carpal.h>
#include <toolbox/misc.h>
#include <toolbox/memory.h>
#include <u/missing.h>

/* generic socket creation interface */
typedef int (*u_net_disp_f) (struct sockaddr *, int, int);

/* workers */
static int do_csock (struct sockaddr *sad, int sad_len, int type);
static int do_ssock (struct sockaddr *sad, int sad_len, int type, int reuse, 
        int backlog);

/* per family dispatchers */
static int tcp4_ssock (struct sockaddr *sad, int reuse, int backlog);
static int tcp4_csock (struct sockaddr *sad, int dummy1, int dummy2);
static int tcp6_ssock (struct sockaddr *sad, int reuse, int backlog);
static int tcp6_csock (struct sockaddr *sad, int dummy1, int dummy2);
static int udp4_ssock (struct sockaddr *sad, int reuse, int backlog);
static int udp4_csock (struct sockaddr *sad, int dummy1, int dummy2);
static int udp6_ssock (struct sockaddr *sad, int reuse, int backlog);
static int udp6_csock (struct sockaddr *sad, int dummy1, int dummy2);
static int unix_ssock (struct sockaddr *sad, int dummy1, int backlog);
static int unix_csock (struct sockaddr *sad, int dummy1, int dummy2);

/* function table to dispatch by socket mode and address family */
u_net_disp_f mode_disp[][2] = 
{
    /* U_NET_SSOCK, U_NET_CSOCK */
    { NULL,         NULL },           /* U_NET_TYPE_MIN */
    { tcp4_ssock,   tcp4_csock },     /* U_NET_TCP4 */
    { tcp6_ssock,   tcp6_csock },     /* U_NET_TCP6 */
    { udp4_ssock,   udp4_csock },     /* U_NET_UDP4 */
    { udp6_ssock,   udp6_csock },     /* U_NET_UDP6 */
    { unix_ssock,   unix_csock },     /* U_NET_UNIX */
    { NULL,         NULL }            /* U_NET_TYPE_MAX */
};

/**
 *  \defgroup net Networking
 *  \{
 */

/** \brief Top level socket creation routine
 *
 * This routine creates a socket and returns its file descriptor. 
 * A \a client socket (\c U_NET_CSOCK) is identified by its connection 
 * endpoint, a \a server socket (\c U_NET_SSOCK) by its local address.
 *
 * The identification is done via a family of private URIs:
 * - {tc,ud}p[46]://&lt;host&gt;:&lt;port&gt; for TCP/UDP over IPv[46] 
 *   addresses,
 * - unix://&lt;abs_path&gt; for UNIX IPC pathnames.
 *
 * After resolving the supplied URI, the control is passed to the appropriate
 * handler which carries out the real job (i.e. \c connect(2) or \c bind(2)).  
 * Per-protocol handlers can be used in combination with the URI resolver and
 * translation functions for greater flexibility.
 *
 * \sa u_net_tcp4_ssock, u_net_tcp6_ssock, u_net_tcp4_csock, u_net_tcp6_csock,
 *     u_net_unix_ssock, u_net_unix_csock, u_net_uri2addr, u_net_uri2sin, 
 *     u_net_uri2sin6, u_net_uri2sun. 
 *
 * \param uri   the URI at which the socket shall be connected/bounded
 * \param mode  \c U_NET_SSOCK for server sockets, \c U_NET_CSOCK for clients.
 * 
 * \return
 *  - the socket descriptor on success
 *  - \c -1 on failure
 */
int u_net_sock (const char *uri, int mode)
{
    int s;
    u_net_disp_f dfun;
    u_net_addr_t *addr = NULL;

    dbg_return_if (uri == NULL, -1);
    dbg_return_if (!U_NET_IS_MODE(mode), -1);

    /* decode address */
    dbg_err_if (u_net_uri2addr(uri, &addr));
    dbg_err_if (!U_NET_IS_VALID_ADDR_TYPE(addr->type));

    /* try to dispatch request based on the supplied address and mode */
    if ((dfun = mode_disp[addr->type][mode]) != NULL)
    {
        s = dfun(&addr->sa.s, 1, U_NET_BACKLOG);
        u_net_addr_free(addr);
    }
    else
        dbg_err("address scheme not supported");

    return s;
err:
    if (addr)
        u_net_addr_free(addr);
    return -1;
}

/** \brief  Return a TCP socket descriptor bound to the supplied IPv4 address */
int u_net_tcp4_ssock (struct sockaddr_in *sad, int reuse, int backlog)
{ 
    return do_ssock((struct sockaddr *) sad, sizeof *sad, 
            AF_INET, reuse, backlog);
}

/** \brief  Return a TCP socket descriptor connected to the supplied IPv4
 *          address */
int u_net_tcp4_csock (struct sockaddr_in *sad)
{
    return do_csock((struct sockaddr *) sad, sizeof *sad, AF_INET);
}

#ifndef NO_IPV6
/** \brief  Return a TCP socket descriptor bound to the supplied IPv6 address */
int u_net_tcp6_ssock (struct sockaddr_in6 *sad, int reuse, int backlog)
{
    return do_ssock((struct sockaddr *) sad, sizeof *sad, 
            AF_INET6, reuse, backlog);
}

/** \brief  Return a TCP socket descriptor connected to the supplied IPv6
 *          address */
int u_net_tcp6_csock (struct sockaddr_in6 *sad)
{
    return do_csock((struct sockaddr *) sad, sizeof *sad, AF_INET6);
}
#endif /* !NO_IPV6 */

#ifndef NO_UNIXSOCK
/** \brief  Return a TCP socket descriptor bound to the supplied UNIX path */
int u_net_unix_ssock (struct sockaddr_un *sad, int backlog)
{
    dbg_return_if (sad == NULL, -1);

    /* try to remove a dangling unix sock path (if any) */
    dbg_return_sif (unlink(sad->sun_path) == -1 && errno != ENOENT, -1);

    return do_ssock((struct sockaddr *) sad, sizeof *sad, 
            AF_UNIX, 0 /* ignored for AF_UNIX */, backlog);
}

/** \brief  Return a TCP socket descriptor connected to the supplied UNIX 
 *          path */
int u_net_unix_csock (struct sockaddr_un *sad)
{
    return do_csock((struct sockaddr *) sad, sizeof *sad, AF_UNIX);
}
#endif /* !NO_UNIXSOCK */

int u_net_sock_tcp (u_net_addr_t *addr, int mode)
{
    u_unused_args(addr, mode);
    info_return_ifm (1, -1, "%s: REMOVED: use u_net_sock()", __FUNCTION__);
}

int u_net_sock_unix (u_net_addr_t *addr, int mode)
{
    u_unused_args(addr, mode);
    info_return_ifm (1, -1, "%s: REMOVED: use u_net_sock()", __FUNCTION__);
}

int u_net_sock_udp (u_net_addr_t *addr, int mode)
{
    u_unused_args(addr, mode);
    info_return_ifm (1, -1, "%s: REMOVED: use u_net_sock()", __FUNCTION__);
}

/** \brief  Create new \c u_net_addr_t object of the requested \p type */
int u_net_addr_new (int type, u_net_addr_t **pa)
{
    u_net_addr_t *a = NULL;

    dbg_return_if (pa == NULL, ~0);
    dbg_return_if (!U_NET_IS_VALID_ADDR_TYPE(type), ~0);

    dbg_err_sif ((a = u_zalloc(sizeof(u_net_addr_t))) == NULL);
    a->type = type;

    *pa = a;

    return 0;
err:
    if (a) 
        u_net_addr_free(a);
    return ~0;
}

/** \brief  Free the supplied \c u_net_addr_t object */
void u_net_addr_free (u_net_addr_t *a)
{
    U_FREE(a);
    return;
}

/** \brief  Decode the supplied \p uri into an \c u_net_addr_t object */
int u_net_uri2addr (const char *uri, u_net_addr_t **pa)
{
    u_uri_t *u = NULL;
    u_net_addr_t *a = NULL;
    int a_type = U_NET_TYPE_MIN;
    
    dbg_return_if (uri == NULL, ~0);
    dbg_return_if (pa == NULL, ~0);

    /* decode address */
    dbg_err_if (u_uri_parse(uri, &u));

    if (!strcasecmp(u->scheme, "tcp4"))
        a_type = U_NET_TCP4;
    else if (!strcasecmp(u->scheme, "udp4"))
        a_type = U_NET_UDP4;
    else if (!strcasecmp(u->scheme, "unix"))
        a_type = U_NET_UNIX;
    else if (!strcasecmp(u->scheme, "tcp6"))
        a_type = U_NET_TCP6;
    else if (!strcasecmp(u->scheme, "udp6"))
        a_type = U_NET_UDP6;
    else
        dbg_err("unsupported URI scheme: %s", u->scheme); 

    /* create address for the decoded type */
    dbg_err_if (u_net_addr_new(a_type, &a));

    /* fill it */
    switch (a_type)
    {
        case U_NET_TCP4:
        case U_NET_UDP4:
            dbg_err_if (u_net_uri2sin(u, &a->sa.sin));
            break;
#ifndef NO_UNIXSOCK
        case U_NET_UNIX:
            dbg_err_if (u_net_uri2sun(uri, &a->sa.sun));
            break;
#endif /* !NO_UNIXSOCK */
#ifndef NO_IPV6
        case U_NET_TCP6:
        case U_NET_UDP6:
            dbg_err_if (u_net_uri2sin6(uri, &a->sa.sin6));
            break;
#endif /* !NO_IPV6 */
        default:
            dbg_err("unsupported address type");
    }

    u_uri_free(u);
    *pa = a;

    return 0;
err:
    if (a)
        u_net_addr_free(a);
    if (u)
        u_uri_free(u);
    return ~0;
}

#ifndef NO_UNIXSOCK
/** \brief  Fill the supplied \c sockaddr_un object at \p *sad with addressing
 *          information coming from the UNIX \p uri string */
int u_net_uri2sun (const char *uri, struct sockaddr_un *sad)
{
    dbg_return_if (uri == NULL, ~0);
    dbg_return_if (sad == NULL, ~0);

    sad->sun_family = AF_UNIX;
    dbg_err_ifm (u_strlcpy(sad->sun_path, uri + strlen("unix://"), 
                sizeof sad->sun_path), "%s too long", uri);

    return 0;
err:
    return ~0;
}
#endif /* !NO_UNIXSOCK */

#ifndef NO_IPV6
/** \brief  Fill the supplied \c sockaddr_in6 object at \p *sad with addressing
 *          information coming from the IPv6 \p uri string */
int u_net_uri2sin6 (const char *uri, struct sockaddr_in6 *sad)
{
    dbg_return_if (uri == NULL, ~0);
    dbg_return_if (sad == NULL, ~0);

    info("TODO");
    return ~0;
}
#endif /* !NO_IPv6 */

/* translate u_uri_t into struct sockaddr_in */
int u_net_uri2sin (u_uri_t *uri, struct sockaddr_in *sad)
{
    in_addr_t saddr;
    struct hostent *hp = NULL;

    dbg_return_if (uri == NULL, ~0);
    dbg_return_if (sad == NULL, ~0);

    memset((char *) sad, 0, sizeof(struct sockaddr_in));
    sad->sin_port = htons(uri->port);
    sad->sin_family = AF_INET;

    hp = gethostbyname(uri->host);

    if (hp && hp->h_addrtype == AF_INET)
        memcpy(&sad->sin_addr.s_addr, hp->h_addr, sizeof(in_addr_t));
    else if ((saddr = inet_addr(uri->host)) != INADDR_NONE)
        sad->sin_addr.s_addr = saddr;
    else
        dbg_err("invalid host name: \'%s\'", uri->host);

    return 0;
err:
    return ~0;
}


/* 
 * all internal methods have a common layout to suit the dispatch table 
 * interface (see u_net_disp_f typedef)
 */

static int tcp4_ssock (struct sockaddr *sad, int reuse, int backlog)
{
    return u_net_tcp4_ssock((struct sockaddr_in *) sad, reuse, backlog);
}

static int tcp4_csock (struct sockaddr *sad, int dummy1, int dummy2)
{ 
    u_unused_args(dummy1, dummy2);

    return u_net_tcp4_csock((struct sockaddr_in *) sad);
} 

static int tcp6_ssock (struct sockaddr *sad, int reuse, int backlog)
{ 
#ifndef NO_IPV6
    return u_net_tcp6_ssock((struct sockaddr_in6 *) sad, reuse, backlog);
#else
    u_unused_args(sad, reuse, backlog);
    info("tcp6 socket not supported");
    return -1;
#endif  /* !NO_IPV6 */
}

static int tcp6_csock (struct sockaddr *sad, int dummy1, int dummy2)
{ 
    u_unused_args(dummy1, dummy2);
#ifndef NO_IPV6
    return u_net_tcp6_csock((struct sockaddr_in6 *) sad);
#else
    u_unused_args(sad);
    info("tcp6 socket not supported");
    return -1;
#endif  /* !NO_IPV6 */
}

static int unix_ssock (struct sockaddr *sad, int dummy1, int backlog)
{
    u_unused_args(dummy1);
#ifndef NO_UNIXSOCK
    return u_net_unix_ssock((struct sockaddr_un *) sad, backlog);
#else
    u_unused_args(sad, backlog);
    info("unix socket not supported");
    return -1;
#endif  /* !NO_UNIXSOCK */
}

static int unix_csock (struct sockaddr *sad, int dummy1, int dummy2)
{
    u_unused_args(dummy1, dummy2);
#ifndef NO_UNIXSOCK
    return u_net_unix_csock((struct sockaddr_un *) sad);
#else
    u_unused_args(sad);
    info("unix socket not supported");
    return -1;
#endif  /* !NO_UNIXSOCK */
}

static int udp4_ssock (struct sockaddr *sad, int reuse, int backlog)
{ 
    u_unused_args(sad, reuse, backlog);
    info_return_ifm (1, -1, "%s: TODO", __FUNCTION__); 
}

static int udp4_csock (struct sockaddr *sad, int dummy1, int dummy2)
{ 
    u_unused_args(sad, dummy1, dummy2);
    info_return_ifm (1, -1, "%s: TODO", __FUNCTION__); 
}

static int udp6_ssock (struct sockaddr *sad, int reuse, int backlog)
{ 
    u_unused_args(sad, reuse, backlog);
    info_return_ifm (1, -1, "%s: TODO", __FUNCTION__); 
}

static int udp6_csock (struct sockaddr *sad, int dummy1, int dummy2)
{ 
    u_unused_args(sad, dummy1, dummy2);
    info_return_ifm (1, -1, "%s: TODO", __FUNCTION__); 
}

static int do_csock (struct sockaddr *sad, int sad_len, int type)
{
    int s = -1;

    dbg_return_if (sad == NULL, -1);

    dbg_err_sif ((s = socket(type, SOCK_STREAM, 0)) == -1);
    dbg_err_sif (connect(s, sad, sad_len) == -1);

    return s;
err:
    U_CLOSE(s);
    return -1;
}

static int do_ssock (struct sockaddr *sad, int sad_len, int type, int reuse, 
        int backlog)
{
    int s = -1;
#ifdef HAVE_SETSOCKOPT
    int on = reuse ? 1 : 0;
#else
    u_unused_args(reuse);
#endif  /* HAVE_SETSOCKOPT */

    dbg_return_if (sad == NULL, -1);

    dbg_err_sif ((s = socket(type, SOCK_STREAM, 0)) == -1);
#ifdef HAVE_SETSOCKOPT
    if (type != AF_UNIX)
    {
        dbg_err_sif (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, 
                    &on, sizeof on) == -1);
    }
#else
    info("could not honour the reuse flag");
#endif
    dbg_err_sif (bind(s, (struct sockaddr *) sad, sad_len) == -1);
    dbg_err_sif (listen(s, backlog) == -1);

    return s;
err:
    U_CLOSE(s);
    return -1;
}
