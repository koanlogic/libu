/* 
 * Copyright (c) 2005-2010 by KoanLogic s.r.l.
 */

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
static int do_csock (struct sockaddr *sad, int sad_len, int domain, int type);
static int do_ssock (struct sockaddr *sad, int sad_len, int domain, int type, 
        int reuse, int backlog);

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
u_net_disp_f disp_tbl[][2] = 
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

/* internal uri-to-sockaddr translators */
static int uri2sockaddr (int f(u_uri_t *, struct sockaddr *), 
        const char *uri, void *sad);
static int ipv4_uri_to_sin (u_uri_t *uri, struct sockaddr *sad);
#ifndef NO_IPV6
static int ipv6_uri_to_sin6 (u_uri_t *uri, struct sockaddr *sad);
#endif  /* !NO_IPV6 */
#ifndef NO_UNIXSOCK
static int unix_uri_to_sun (u_uri_t *uri, struct sockaddr *sad);
#endif  /* !NO_UNIXSOCK */

/* internal uri representation */
struct u_net_addr_s
{
    int type;   /* one of U_NET_TYPE's */
    union
    {
        struct sockaddr     s;
        struct sockaddr_in  sin;
#ifndef NO_IPV6
        struct sockaddr_in6 sin6;
#endif
#ifndef NO_UNIXSOCK
        struct sockaddr_un  sun;
#endif /* OS_UNIX */
    } sa;
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
 * \sa u_net_sock_by_addr
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
    u_net_addr_t *addr = NULL;

    /* 'mode' check is done by u_net_sock_by_addr() */
    dbg_return_if (uri == NULL, -1);

    /* decode address ('addr' validation is done by u_net_sock_by_addr() */
    dbg_err_if (u_net_uri2addr(uri, &addr));

    /* ask our worker to do the real job */
    s = u_net_sock_by_addr(addr, mode);
    u_net_addr_free(addr);

    return s;
err:
    if (addr)
        u_net_addr_free(addr);
    return -1;
}

/** \brief  Like \c u_net_sock but need an already filled-in \p addr.  You
 *          can use it whenever you need to reuse a given address: call 
 *          \c u_net_uri2addr once and \c u_net_sock_by_addr repeatedly.  */
int u_net_sock_by_addr (u_net_addr_t *addr, int mode)
{
    u_net_disp_f dfun;

    dbg_return_if (addr == NULL, -1);
    dbg_return_if (!U_NET_IS_VALID_ADDR_TYPE(addr->type), -1);
    dbg_return_if (!U_NET_IS_MODE(mode), -1);

    /* try to dispatch request based on the supplied address and mode */
    if ((dfun = disp_tbl[addr->type][mode]) != NULL)
        return dfun(&addr->sa.s, 1 /* reuse */, U_NET_BACKLOG);

    info("address scheme not supported");
    return -1;
}

/** \brief  Specialisation of \c u_net_sock_by_addr for TCP sockets */
int u_net_sock_tcp (u_net_addr_t *addr, int mode)
{
    dbg_return_if (addr == NULL, -1);
    dbg_return_if (addr->type != U_NET_TCP4 && addr->type != U_NET_TCP6, -1);

    return u_net_sock_by_addr(addr, mode);
}

/** \brief  Specialisation of \c u_net_sock_by_addr for UNIX sockets */
int u_net_sock_unix (u_net_addr_t *addr, int mode)
{
    dbg_return_if (addr == NULL, -1);
    dbg_return_if (addr->type != U_NET_UNIX, -1);

    return u_net_sock_by_addr(addr, mode);
}

/** \brief  Specialisation of \c u_net_sock_by_addr for UDP sockets */
int u_net_sock_udp (u_net_addr_t *addr, int mode)
{
    dbg_return_if (addr == NULL, -1);
    dbg_return_if (addr->type != U_NET_UDP4 && addr->type != U_NET_UDP6, -1);

    return u_net_sock_by_addr(addr, mode);
}

/** \brief  Return a TCP socket descriptor bound to the supplied IPv4 address */
int u_net_tcp4_ssock (struct sockaddr_in *sad, int reuse, int backlog)
{ 
    return do_ssock((struct sockaddr *) sad, sizeof *sad, 
            AF_INET, SOCK_STREAM, reuse, backlog);
}

/** \brief  Return a TCP socket descriptor connected to the supplied IPv4
 *          address */
int u_net_tcp4_csock (struct sockaddr_in *sad)
{
    return do_csock((struct sockaddr *) sad, sizeof *sad, AF_INET, SOCK_STREAM);
}

/** \brief  Return a UDP socket descriptor bound to the supplied IPv4 address */
int u_net_udp4_ssock (struct sockaddr_in *sad, int reuse)
{ 
    return do_ssock((struct sockaddr *) sad, sizeof *sad, 
            AF_INET, SOCK_DGRAM, reuse, 0 /* backlog is ignored by UDP */);
}

/** \brief  Return a UDP socket descriptor connected (the UDP way) to the 
 *          supplied IPv4 address */
int u_net_udp4_csock (struct sockaddr_in *sad)
{
    return do_csock((struct sockaddr *) sad, sizeof *sad, AF_INET, SOCK_DGRAM);
}

#ifndef NO_IPV6

/** \brief  Return a TCP socket descriptor bound to the supplied IPv6 address */
int u_net_tcp6_ssock (struct sockaddr_in6 *sad, int reuse, int backlog)
{
    return do_ssock((struct sockaddr *) sad, sizeof *sad, 
            AF_INET6, SOCK_STREAM, reuse, backlog);
}

/** \brief  Return a TCP socket descriptor connected to the supplied IPv6
 *          address */
int u_net_tcp6_csock (struct sockaddr_in6 *sad)
{
    return do_csock((struct sockaddr *) sad, sizeof *sad, 
            AF_INET6, SOCK_STREAM);
}

/** \brief  Return a UDP socket descriptor bound to the supplied IPv6 address */
int u_net_udp6_ssock (struct sockaddr_in6 *sad, int reuse)
{ 
    return do_ssock((struct sockaddr *) sad, sizeof *sad, 
            AF_INET6, SOCK_DGRAM, reuse, 0 /* backlog is ignored by UDP */);
}

/** \brief  Return a UDP socket descriptor connected (the UDP way) to the 
 *          supplied IPv6 address */
int u_net_udp6_csock (struct sockaddr_in6 *sad)
{
    return do_csock((struct sockaddr *) sad, sizeof *sad, AF_INET6, SOCK_DGRAM);
}

/** \brief  Fill the supplied \c sockaddr_in6 object at \p *sad with addressing
 *          information coming from the IPv6 \p uri string */
int u_net_uri2sin6 (const char *uri, struct sockaddr_in6 *sad)
{
    return uri2sockaddr(ipv6_uri_to_sin6, uri, sad);
}

static int ipv6_uri_to_sin6 (u_uri_t *uri, struct sockaddr *sad)
{
    char *hnum;
    struct hostent *hp = NULL;
    struct sockaddr_in6 *_sad;

    dbg_return_if (uri == NULL, ~0);
    dbg_return_if (strcasecmp(uri->scheme, "tcp6") && 
            strcasecmp(uri->scheme, "udp6"), ~0);
    dbg_return_if (uri->port <= 0 || uri->port > 65535, ~0);
    dbg_return_if ((_sad = (struct sockaddr_in6 *) sad) == NULL, ~0);

    memset((char *) _sad, 0, sizeof(struct sockaddr_in6));

    _sad->sin6_len = sizeof(struct sockaddr_in6);
    _sad->sin6_port = htons(uri->port);
    _sad->sin6_family = AF_INET6;
    _sad->sin6_flowinfo = 0;

    /* '*' is the wildcard address */
    if (!strcmp(uri->host, "*"))
        _sad->sin6_addr = in6addr_any;
    else
    {
        /* try with the resolver first, if no result, fall back to numeric */
        hp = gethostbyname(uri->host);

        hnum = (hp && hp->h_addrtype == AF_INET6) ? hp->h_addr : uri->host;

        /* convert address from printable to network format */
        switch (inet_pton(AF_INET6, hnum, &_sad->sin6_addr))
        {
            case -1:
                dbg_strerror(errno); 
                /* log errno and fall through */
            case 0:
                dbg_err("invalid IPv6 host name: \'%s\'", uri->host);
                /* log hostname and jump to err: */
            default:
                break;
        }
    }

    return 0;
err:
    return ~0;
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
            AF_UNIX, SOCK_STREAM, 0 /* ignored for AF_UNIX */, backlog);
}

/** \brief  Return a TCP socket descriptor connected to the supplied UNIX 
 *          path */
int u_net_unix_csock (struct sockaddr_un *sad)
{
    return do_csock((struct sockaddr *) sad, sizeof *sad, AF_UNIX, SOCK_STREAM);
}

/** \brief  Fill the supplied \c sockaddr_un object at \p *sad with addressing
 *          information coming from the UNIX \p uri string */
int u_net_uri2sun (const char *uri, struct sockaddr_un *sad)
{
    return uri2sockaddr(unix_uri_to_sun, uri, sad);
}

static int unix_uri_to_sun (u_uri_t *uri, struct sockaddr *sad)
{
    struct sockaddr_un *_sad;

    dbg_return_if (uri == NULL, ~0);
    dbg_return_if (strcasecmp(uri->scheme, "unix"), ~0);
    dbg_return_if ((_sad = (struct sockaddr_un *) sad) == NULL, ~0);

    _sad->sun_family = AF_UNIX;
    dbg_err_ifm (u_strlcpy(_sad->sun_path, uri->path, sizeof _sad->sun_path), 
            "%s too long", uri);

    return 0;
err:
    return ~0;
}

#endif /* !NO_UNIXSOCK */

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
            dbg_err_if (ipv4_uri_to_sin(u, &a->sa.s));
            break;
#ifndef NO_UNIXSOCK
        case U_NET_UNIX:
            dbg_err_if (unix_uri_to_sun(u, &a->sa.s));
            break;
#endif /* !NO_UNIXSOCK */
#ifndef NO_IPV6
        case U_NET_TCP6:
        case U_NET_UDP6:
            dbg_err_if (ipv6_uri_to_sin6(u, &a->sa.s));
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

static int uri2sockaddr (int f(u_uri_t *, struct sockaddr *), 
        const char *uri, void *sad)
{
    int rc;
    u_uri_t *u = NULL;

    dbg_return_if (uri == NULL, ~0);
    dbg_return_if (sad == NULL, ~0);
    
    dbg_err_if (u_uri_parse(uri, &u));
    rc = f(u, sad);
    u_uri_free(u);

    return rc;
err:
    if (u)
        u_uri_free(u);
    return ~0;
}

/** \brief  Fill the supplied \c sockaddr_in object at \p *sad with addressing
 *          information coming from the IPv4 \p uri string */
int u_net_uri2sin (const char *uri, struct sockaddr_in *sad)
{
    return uri2sockaddr(ipv4_uri_to_sin, uri, sad);
}

static int ipv4_uri_to_sin (u_uri_t *uri, struct sockaddr *sad)
{
    in_addr_t saddr;
    struct hostent *hp = NULL;
    struct sockaddr_in *_sad;

    dbg_return_if (uri == NULL, ~0);
    dbg_return_if (strcasecmp(uri->scheme, "tcp4") && 
            strcasecmp(uri->scheme, "udp4"), ~0);
    dbg_return_if (uri->port <= 0 || uri->port > 65535, ~0);
    dbg_return_if ((_sad = (struct sockaddr_in *) sad) == NULL, ~0);

    memset((char *) _sad, 0, sizeof(struct sockaddr_in));
    _sad->sin_port = htons(uri->port);
    _sad->sin_family = AF_INET;

    /* '*' is the wildcard address */
    if (!strcmp(uri->host, "*"))
        _sad->sin_addr.s_addr = htonl(INADDR_ANY);
    else
    {
        /* try with the resolver first, if no result, fall back to numeric */
        hp = gethostbyname(uri->host);

        if (hp && hp->h_addrtype == AF_INET)
            memcpy(&_sad->sin_addr.s_addr, hp->h_addr, sizeof(in_addr_t));
        else if ((saddr = inet_addr(uri->host)) != INADDR_NONE)
            _sad->sin_addr.s_addr = saddr;
        else
            dbg_err("invalid host name: \'%s\'", uri->host);
    }

    return 0;
err:
    return ~0;
}

/**
 *  \brief  accept(2) wrapper that handles EINTR
 *
 *  \param  ld          file descriptor
 *  \param  addr        see accept(2)   
 *  \param  addrlen     size of addr struct
 *
 *  \return on success returns the socket descriptor; on failure returns -1
 */ 
#ifdef HAVE_SOCKLEN_T
int u_accept(int ld, struct sockaddr *addr, socklen_t *addrlen)
#else
int u_accept(int ld, struct sockaddr *addr, int *addrlen)
#endif  /* HAVE_SOCKLEN_T */
{
    int ad = -1;

again:
    if ((ad = accept(ld, addr, addrlen)) == -1 && (errno == EINTR))
        goto again; /* interrupted */

    return ad;
}

/**
 *  \brief  Disable Nagle algorithm on the supplied TCP socket
 *
 *  \param  sd  a TCP socket descriptor
 *
 *  \return \c 0 if successful, \c ~0 on error
 */ 
int u_net_nagle_off (int sd)
{
#if defined(HAVE_TCP_NODELAY) && defined(HAVE_SETSOCKOPT)
    int rc, on = 1;

    dbg_return_if (sd < 0, ~0);

    rc = setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof on);
    dbg_err_sifm (rc == -1, "cannot disable Nagle algorithm on sd %d", sd);

    return 0;
err:
    return ~0;
#else /* !HAVE_TCP_NODELAY || !HAVE_SETSOCKOPT */
    u_unused_args(sd);
    dbg("u_net_nagle_off not supported on this platform");
    return 0;
#endif  /* !HAVE_TCP_NODELAY */
}

/**
 *  \}
 */

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
    u_unused_args(backlog);

    return u_net_udp4_ssock((struct sockaddr_in *) sad, reuse);
}

static int udp4_csock (struct sockaddr *sad, int dummy1, int dummy2)
{ 
    u_unused_args(dummy1, dummy2);

    return u_net_udp4_csock((struct sockaddr_in *) sad);
}

static int udp6_ssock (struct sockaddr *sad, int reuse, int backlog)
{ 
#ifndef NO_IPV6
    return do_ssock((struct sockaddr *) sad, sizeof(struct sockaddr_in6),
            AF_INET6, SOCK_DGRAM, reuse, backlog /* ignored by UDP */);
#else
    u_unused_args(sad, reuse, backlog);
    info("udp6 socket not supported");
    return -1;
#endif  /* !NO_IPV6 */
}

static int udp6_csock (struct sockaddr *sad, int dummy1, int dummy2)
{ 
    u_unused_args(dummy1, dummy2);
#ifndef NO_IPV6
    return do_csock((struct sockaddr *) sad, sizeof *sad, AF_INET6, SOCK_DGRAM);
#else
    u_unused_args(sad);
    info("udp6 socket not supported");
    return -1;
#endif  /* !NO_IPV6 */
}

static int do_csock (struct sockaddr *sad, int sad_len, int domain, int type)
{
    int s = -1;

    dbg_return_if (sad == NULL, -1);

    dbg_err_sif ((s = socket(domain, type, 0)) == -1);

    /* NOTE that also UDP sockets (not only TCP and UNIX) are connected.
     * This has a couple of important implications:
     * 1) the caller must use u_net_read/u_net_write for I/O instead of 
     *    recvfrom/sendto;
     * 2) async errors are returned to the process. */
    dbg_err_sif (connect(s, sad, sad_len) == -1);

    return s;
err:
    U_CLOSE(s);
    return -1;
}

static int do_ssock (struct sockaddr *sad, int sad_len, int domain, int type, 
        int reuse, int backlog)
{
    int s = -1;
#ifdef HAVE_SETSOCKOPT
    int on = reuse ? 1 : 0;
#else
    u_unused_args(reuse);
#endif  /* HAVE_SETSOCKOPT */

    dbg_return_if (sad == NULL, -1);

    dbg_err_sif ((s = socket(domain, type, 0)) == -1);
#ifdef HAVE_SETSOCKOPT
    if (domain != AF_UNIX)
    {
        dbg_err_sif (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, 
                    &on, sizeof on) == -1);
    }
#else
    info("could not honour the reuse flag");
#endif
    dbg_err_sif (bind(s, (struct sockaddr *) sad, sad_len) == -1);

    /* only stream sockets enter the LISTEN state */
    if (type == SOCK_STREAM)
        dbg_err_sif (listen(s, backlog) == -1);

    return s;
err:
    U_CLOSE(s);
    return -1;
}
