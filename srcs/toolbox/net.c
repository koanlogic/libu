/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l.
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>

#include <toolbox/carpal.h>
#include <toolbox/net.h>
#include <toolbox/misc.h>

#ifndef HAVE_GETADDRINFO
/* Duplicate addrinfo layout in case struct addrinfo and related
 * methods were not available on the platform. */
struct u_addrinfo_s
{
    int ai_flags;       /* not used */
    int ai_family;      /* AF_INET, AF_INET6, AF_UNIX */
    int ai_socktype;    /* SOCK_STREAM, SOCK_DGRAM, SOCK_SEQPACKET */
    int ai_protocol;    /* (ai_family == AF_UNIX) => 0, otherwise one of:
                           IPPROTO_TCP, IPPROTO_UDP, IPPROTO_SCTP */
    char *ai_canonname;
    u_socklen_t ai_addrlen;
    struct sockaddr *ai_addr;
    struct u_addrinfo_s *ai_next;
};
#endif  /* !HAVE_GETADDRINFO */

/* how u_net uri are represented */
struct u_net_addr_s
{
    int opts;   /* OR'd U_NET_OPT's */
    u_net_mode_t mode;  /* one of U_NET_SSOCK or U_NET_CSOCK */
    u_addrinfo_t *addr; /* list of available addresses */
    u_addrinfo_t *cur;  /* reference to the current working address */
};

/* private stuff */ 
static int na_new (u_net_mode_t mode, u_net_addr_t **pa);
static void ai_free (u_addrinfo_t *ai);
#ifndef NO_SCTP
static int sctp_enable_events (int s, int opts);
#endif  /* !NO_SCTP */
static int bind_reuse_addr (int s);

/* low level resolvers */
static int do_resolv (
        int rf (const char *, const char *, const char *, u_addrinfo_t *ai), 
        const char *host, const char *port, const char *path, int family, 
        int type, int proto, u_addrinfo_t **pai);
#ifndef NO_UNIXSOCK
/* this is needed here because getaddrinfo() don't understand AF_UNIX */
static int resolv_sun (const char *dummy1, const char *dummy2, const char *path,
        u_addrinfo_t *ai); 
#endif  /* !NO_UNIXSOCK */
static int ai_resolv (const char *host, const char *port, const char *path,
        int family, int type, int proto, int passive, u_addrinfo_t **pai);

#ifndef HAVE_GETADDRINFO
static int resolv_sin (const char *host, const char *port, const char *dummy, 
        u_addrinfo_t *ai); 
#ifndef NO_IPV6
static int resolv_sin6 (const char *host, const char *port, const char *dummy,
        u_addrinfo_t *ai);
#endif  /* !NO_IPV6 */
static int resolv_port (const char *s_port, uint16_t *pin_port);
#endif  /* !HAVE_GETADDRINFO */

/* socket creation horses */
static int do_sock (
        int f(struct sockaddr *, u_socklen_t, int, int, int, int, int, 
            struct timeval *),
        struct u_net_addr_s *a, int backlog, u_addrinfo_t **pai, 
        struct timeval *timeout);
static int do_csock (struct sockaddr *sad, u_socklen_t sad_len, int domain, 
        int type, int protocol, int opts, int dummy,
        struct timeval *timeout);
static int do_ssock (struct sockaddr *sad, u_socklen_t sad_len, int domain, 
        int type, int protocol, int opts, int backlog,
        struct timeval *dummy);

/* URI scheme mapper */
struct u_net_scheme_map_s
{
    int addr_family;    /* one of AF_... */
    int sock_type;      /* one of SOCK_... */
    int proto;          /* one of IPPROTO_... */
};
typedef struct u_net_scheme_map_s u_net_scheme_map_t;

static int scheme_mapper (const char *scheme, u_net_scheme_map_t *map);
static int uri2addr (u_uri_t *u, u_net_scheme_map_t *m, u_net_addr_t *a);
static inline int update_timeout (struct timeval *timeout, 
        struct timeval *tstart);

/**
    \defgroup net Networking
    \{
        The \ref net module defines the following private URI schemes 
        corresponding to the protocol combos:
   
        <table>
          <tr>
            <td></td>
            <td><b>TCP</b></td>
            <td><b>UDP</b></td>
            <td><b>SCTP</b></td>
          </tr>
          <tr>
            <td><b>IPv4</b></td>
            <td><tt>tcp://</tt> or <tt>tcp4://</tt></td>
            <td><tt>udp://</tt> or <tt>udp4://</tt></td>
            <td><tt>sctp://</tt> or <tt>sctp4://</tt></td>
          </tr>
          <tr>
            <td><b>IPv6</b></td>
            <td><tt>tcp6://</tt></td>
            <td><tt>udp6://</tt></td>
            <td><tt>sctp6://</tt></td>
          </tr>
          <tr>
            <td><b>UNIX</b></td>
            <td><tt>unix://</tt></td>
            <td>N.A.</td>
            <td>N.A.</td>
          </tr>
        </table>
      
        Except for \c unix:// , which accepts only a valid pathname in the 
        target file system, e.g.: 
        - <code> unix:///tmp/my.sock </code>
  
        every other scheme needs an <b>host</b> (by name or numeric address) 
        and a <b>port</b> (by service name, or numeric in range 
        <code>[1..65535]</code>) separated by a single \c ':' character, e.g.:
        - <code> tcp://www.kame.net:http </code>
        - <code> udp6://[fe80::200:f8ff:fe21:67cf]:65432 </code>
        - <code> sctp4://myhost:9999 </code>
  
        Note that IPv6 numeric addresses must be enclosed with brackets as per 
        RFC 3986.
  
        Also, the wildcard address is specified with a \c '*', and the same 
        representation is used to let the kernel choose an ephemeral port for 
        us. e.g.:
        - <code> tcp6://[*]:1025 </code>
        - <code> tcp4://192.168.0.1:* </code>

        There exist two ways to obtain a socket descriptor: the first creates
        and consumes an address object without the caller ever noticing it and
        is ideal for one-shot initialization and use, e.g. a passive socket 
        which is created once and accept'ed multiple times during the process 
        lifetime:
    \code
    int sd, asd;
    struct sockaddr_storage sa;
    socklen_t sa_len = sizeof sa;

    // create a passive TCP socket
    dbg_err_if ((sd = u_net_sd("tcp://my:http-alt", U_NET_SSOCK, 0)) == -1);

    for (;;)
    {
        // accept new incoming connections
        asd = u_accept(sd, (struct sockaddr *) &sa, &sa_len);

        dbg_ifb (asd == -1)
            continue;

        // handle it
        do_serve(asd);
    }
    \endcode

        The second allows to reuse the same address object multiple times, and 
        could easily fit a scenario where a transient connection must be set up
        on regular basis:
    \code
    int csd;
    u_net_addr_t *a = NULL;

    // create an address object for an active TCP socket
    dbg_err_if (u_net_uri2addr("tcp://my:http-alt", U_NET_CSOCK, &a));

    for (;;)
    {
        // sleep some time 
        (void) u_sleep(SOME_TIME);

        // connect to the server host, reusing the same address
        dbg_ifb ((csd = u_net_sd_by_addr(a)) == -1)
            continue;

        // do some messaging over the connected socket
        dbg_if (do_io(csd));
    }
    \endcode

    The net module primarily aims at simplifying the socket creation process. 
    When you receive back your brand new socket descriptor, its goal is almost 
    done.  You can use libu's ::u_read, ::u_write, ::u_net_readn, 
    ::u_net_writen or barebones \c sendto(2), \c select(2), \c recvmsg(2), as 
    much as you like.  One of my favourite is to use it in association with 
    <a href="http://www.monkey.org/~provos/libevent">libevent</a>.
*/

/** 
 *  \brief  Parse the supplied uri and transform it into an \a u_net_addr_t .
 *
 *  Parse the supplied \p uri string and return an ::u_net_addr_t object at
 *  \p pa as a result argument.  The address can then be used (and reused) to 
 *  create passive or connected sockets by means of the ::u_net_sd_by_addr 
 *  interface.
 *
 *  \param  uri     an URI string
 *  \param  mode    one of ::U_NET_SSOCK if the address needs to be used
 *                  for a passive socket, or ::U_NET_CSOCK if the address
 *                  will be used for a connected (i.e. non-passive) socket
 *  \param  pa      the translated ::u_net_addr_t as a result argument
 *
 *  \retval  0  on success
 *  \retval ~0  on error
 */
int u_net_uri2addr (const char *uri, u_net_mode_t mode, u_net_addr_t **pa)
{
    const char *s;
    u_uri_t *u = NULL;
    u_net_scheme_map_t smap;
    u_net_addr_t *a = NULL;

    dbg_return_if (uri == NULL, ~0);
    dbg_return_if (pa == NULL, ~0);

    /* decode address and map the uri scheme to a suitable set of socket 
     * creation parameters */
    dbg_err_if (u_uri_crumble(uri, U_URI_OPT_NONE, &u));
    dbg_err_if ((s = u_uri_get_scheme(u)) == NULL);
    dbg_err_ifm (scheme_mapper(s, &smap), "unsupported URI scheme: %s", s);

    /* create the address container */
    dbg_err_sif (na_new(mode, &a));

    /* create the internal representation */
    dbg_err_if (uri2addr(u, &smap, a));

    u_uri_free(u);

    *pa = a;

    return 0;
err:
    if (u)
        u_uri_free(u);
    if (a)
        u_net_addr_free(a);
    return ~0;
}

/**
 *  \brief  Create a socket (connected or passive) from a given ::u_net_addr_t
 *          object.
 * 
 *  Create a socket, being connected or passive depends on \p mode value, given
 *  the already filled-in ::u_net_addr_t object \p a.  The returned socket 
 *  descriptor can then be used for any I/O operation compatible with its
 *  underlying nature.
 *
 *  \param  a       a valid ::u_net_addr_t address, created by a previous
 *                  call to ::u_net_uri2addr 
 *  \param  timeout maximum time to wait for connection
 *
 *  \return the created socket descriptor, or \c -1 on error.
 */ 
int u_net_sd_by_addr_ex (u_net_addr_t *a, struct timeval *timeout)
{
    dbg_return_if (a == NULL, ~1);

    switch (a->mode)
    {
        case U_NET_SSOCK:
            return do_sock(do_ssock, a, U_NET_BACKLOG, &a->cur, timeout);
        case U_NET_CSOCK:
            return do_sock(do_csock, a, 0, &a->cur, timeout);
        default:
            dbg_return_ifm (1, -1, "unknown socket mode %d", a->mode);
    }
}

/**
 *  \brief  Wrapper to ::u_net_sd_by_addr_ex() with no timeout
 */ 
int u_net_sd_by_addr (u_net_addr_t *a)
{
    return u_net_sd_by_addr_ex(a, NULL);
}

/**
 *  \brief  Minimalistic, one-shot interface which wraps ::u_net_uri2addr, 
 *          ::u_net_addr_set_opts and ::u_net_sd_by_addr bits together.
 *
 *  \param  uri     an URI string
 *  \param  mode    one of ::U_NET_CSOCK (connected) or ::U_NET_SSOCK (passive)
 *  \param  opts    set of OR'd <code>U_NET_OPT_*</code> bits
 *  \param  timeout maximum time to wait for connection
 *
 *  \return the newly created socket descriptor, or \c -1 on error.
 */ 
int u_net_sd_ex (const char *uri, u_net_mode_t mode, int opts, 
        struct timeval *timeout)
{
    int s = -1;
    u_net_addr_t *a = NULL;

    /* 'mode' check is done by u_net_uri2addr() */
    dbg_return_if (uri == NULL, -1);

    /* decode address (u_net_addr_t validation is done later by 
     * u_net_sd_by_addr() */
    dbg_err_if (u_net_uri2addr(uri, mode, &a));

    /* add options if any */
    u_net_addr_set_opts(a, opts);

    /* do the real job */
    s = u_net_sd_by_addr_ex(a, timeout);

    /* dispose the (one-shot) address */
    u_net_addr_free(a);

    return s;
err:
    if (s != -1)
        (void) close(s);
    if (a)
        u_net_addr_free(a);

    return -1;
}

/**
 *  \brief  Wrapper to ::u_net_sd_ex() with no timeout
 */ 
int u_net_sd (const char *uri, u_net_mode_t mode, int opts)
{
    return u_net_sd_ex(uri, mode, opts, NULL);
}
 
/** 
 *  \brief  Tell if the supplied address is entitled to call accept(2) 
 *
 *  \param  a   A ::u_net_addr_t object that has been created via 
 *              ::u_net_uri2addr
 *
 *  \retval 1   if the address is suitable for accept'ing connections
 *  \retval 0   if the address is invalid, or not eligible for accept(2)
 */
int u_net_addr_can_accept (u_net_addr_t *a)
{
    dbg_return_if (a == NULL, 0);
    dbg_return_if (a->cur == NULL, 0);

    switch (a->cur->ai_socktype)
    {
        case SOCK_STREAM:
        case SOCK_SEQPACKET:
            return (a->mode == U_NET_SSOCK) ? 1 : 0;
        default:
            return 0;
    }
}

/**
 *  \brief  Set the supplied address' options to \p opts.
 *
 *  Install the supplied options set \p opts into the address pointed by \p 
 *  a.  Beware that this operation overrides any option that was previously 
 *  installed in the address.
 *
 *  \param  a       an already allocated ::u_net_addr_t object
 *  \param  opts    the set of options that will be installed
 *
 *  \return nothing
 */ 
void u_net_addr_set_opts (u_net_addr_t *a, int opts)
{
    dbg_return_if (a == NULL, );
    a->opts = opts;
    return;
}

/**
 *  \brief  Add the supplied \p opts to the given address.
 *
 *  Add the supplied options set \p opts to \p a options' set.
 *  This operation is not disruptive so that any option that was previously 
 *  installed in the address is retained.
 *
 *  \param  a       an already allocated ::u_net_addr_t object
 *  \param  opts    the set of options that will added to the set of options
 *                  already installed
 *
 *  \return nothing
 */ 
void u_net_addr_add_opts (u_net_addr_t *a, int opts)
{
    dbg_return_if (a == NULL, );
    a->opts |= opts;
    return;
}

/** 
 *  \brief  Free memory allocated to an ::u_net_addr_t object
 *
 *  Free resources allocated to the previously allocated ::u_net_addr_t 
 *  object \p a.  A new ::u_net_addr_t object is created at each 
 *  ::u_net_uri2addr invocation and must be explicitly released, otherwise
 *  it'll be leaked.
 *
 *  \return nothing
 */
void u_net_addr_free (u_net_addr_t *a)
{
    dbg_return_if (a == NULL, );

    /* release the inner blob */
    ai_free(a->addr);

    /* release the wrapping container */
    u_free(a);
    return;
}

/** \brief  accept(2) wrapper that handles \c EINTR */ 
int u_accept (int sd, struct sockaddr *addr, u_socklen_t *addrlen)
{
    int ad;

again:
    if ((ad = accept(sd, addr, addrlen)) == -1 && (errno == EINTR))
        goto again; /* interrupted */

#ifdef U_NET_TRACE
    u_dbg("accept(%d, %p, %d) = %d", sd, addr, addrlen, ad); 
#endif

    /* in case of error log errno, otherwise fall through */
    dbg_err_sif (ad == -1);
err:
    return ad;
}

/** \brief  socket(2) wrapper */
int u_socket (int domain, int type, int protocol)
{
    int s = socket(domain, type, protocol);

#ifdef U_NET_TRACE
    u_dbg("socket(%d, %d, %d) = %d", domain, type, protocol, s); 
#endif

    /* in case of error log errno, otherwise fall through */
    dbg_err_sif (s == -1);
err:
    return s;
}

/** \brief  Wrapper to ::u_connect_ex with no timeout   */
int u_connect (int sd, const struct sockaddr *addr, u_socklen_t addrlen)
{
    return u_connect_ex(sd, addr, addrlen, NULL);
}

/**
 *  \brief  timeouted connect(2) wrapper that handles \c EINTR
 *
 *  Timeouted connect(2) wrapper that handles \c EINTR.
 *  In case the underlying select(2) is interrupted by a trapped signal, 
 *  the object pointed to by the \p timeout argument may be modified, so it's 
 *  safer to explicitly reinitialize it for any subsequent use.
 *
 *  \param  sd      socket descriptor
 *  \param  addr    address of the peer
 *  \param  addrlen size of \p addr in bytes
 *  \param  timeout if non-NULL specifies a maximum interval to wait for the 
 *                  connection attempt to complete.  If a NULL value is supplied
 *                  the default platform timeout is in place.
 *
 *  \retval  0  success
 *  \retval -1  generic error
 *  \retval -2  timeout
 */ 
int u_connect_ex (int sd, const struct sockaddr *addr, u_socklen_t addrlen,
        struct timeval *timeout)
{
    int rc;
    u_socklen_t rc_len = sizeof rc;
    struct timeval tstart, tcopy, *ptcopy = timeout ? &tcopy : NULL;
    fd_set writefds;

    dbg_return_if (sd < 0, -1);
    dbg_return_if (addr == NULL, -1);
    dbg_return_if (addrlen == 0, -1);

    if (timeout)
    {
        dbg_err_if (u_net_set_nonblocking(sd));
        dbg_err_sif (gettimeofday(&tstart, NULL) == -1);
    }
   
    /* Open Group Base Specifications:
     *  "If connect() is interrupted by a signal that is caught while blocked 
     *   waiting to establish a connection, connect() shall fail and set 
     *   connect() to [EINTR], but the connection request shall not be aborted, 
     *   and the connection shall be established asynchronously." 
     * That's why we can't just simply 'goto again' here when errno==EINTR,
     * instead a rather convoluted select() based machinery must be put in
     * place.  Anyway, since the timeout'ed version shares the same code layout
     * we can kill two birds with one stone... */
    nop_return_if ((rc = connect(sd, addr, addrlen)) == 0, 0);
    dbg_err_sif (errno != EINTR && errno != EINPROGRESS);

    /* "When the connection has been established asynchronously, select() and 
     *  poll() shall indicate that the file descriptor for the socket is ready 
     *  for writing." */
    FD_ZERO(&writefds);
    FD_SET(sd, &writefds);

    for (;;)
    {
        /* On Linux, select() also modifies timeout if the call is 
         * interrupted by a signal handler (i.e., the EINTR error return).
         * We have to take care of this non-POSIX Linux feature by making a 
         * copy of the original timeout value, so that, on EINTR, it can be 
         * correctly re-computed by update_timeout(). */
        if (timeout)
            tcopy = *timeout;

        /* Try repeatedly to select the socket descriptor for a write 
         * condition to happen. */
        if ((rc = select(sd + 1, NULL, &writefds, NULL, ptcopy)) > 0)
            break;

        /* Timeout: jump to err. */
        if (rc == 0)
        {
            errno = ETIMEDOUT;
            dbg_err("connection timeouted on socket %d", sd);
        }

        /* Unless interrupted by a cought signal (in which case select() is
         * re-entered), bail out. */
        dbg_err_sif (rc == -1 && errno != EINTR);

        /* When interrupted, recalculate timeout value. */
        if (timeout)
        {
            dbg_err_if (update_timeout(timeout, &tstart));

            /* Handle corner case. */
            if (timeout->tv_sec < 0)
            {
                errno = ETIMEDOUT;
                dbg_err("negative timeout on socket %d.", sd);
            }
        }
    }

    /* Ok, if we reached here we're almost done, just peek at SO_ERROR to
     * check if there is any pending error on the socket. */
    dbg_err_sif (u_getsockopt(sd, SOL_SOCKET, SO_ERROR, &rc, &rc_len) == -1);
    if (rc)
    {
        errno = rc;
        warn_err("error detected on socket %d: %s", sd, strerror(errno));
    }

    if (timeout)
        (void) u_net_unset_nonblocking(sd);

    return 0;
err:
    if (timeout)
        (void) u_net_unset_nonblocking(sd);

    return (errno == ETIMEDOUT) ? -2 : -1;
}

/** \brief  bind(2) wrapper */
int u_bind (int sd, const struct sockaddr *addr, u_socklen_t addrlen)
{
    int rc = bind(sd, addr, addrlen);

#ifdef U_NET_TRACE
    u_dbg("bind(%d, %p, %d) = %d", sd, addr, addrlen, rc); 
#endif

    /* in case of error log errno, otherwise fall through */
    dbg_err_sif (rc == -1);
err:
    return rc;
}

/** \brief  listen(2) wrapper */
int u_listen (int sd, int backlog)
{
    int rc = listen(sd, backlog);

#ifdef U_NET_TRACE
    u_dbg("listen(%d, %d) = %d", sd, backlog, rc); 
#endif

    /* in case of error log errno, otherwise fall through */
    dbg_err_sif (rc == -1);
err:
    return rc;
}

/** \brief  setsockopt(2) wrapper */
int u_setsockopt (int sd, int lev, int name, const void *val, u_socklen_t len)
{
#ifdef HAVE_SETSOCKOPT
    int rc = setsockopt(sd, lev, name, val, len);

#ifdef U_NET_TRACE
    u_dbg("setsockopt(%d, %d, %d, %p, %d) = %d", sd, lev, name, val, len, rc);
#endif

    dbg_err_sif (rc == -1);
err:
    return rc;
#else
    u_unused_args(sd, lev, name, val, len);
    u_info("setsockopt not implemented on this platform");
    return -1;
#endif  /* HAVE_SETSOCKOPT */
}

/** \brief  getsockopt(2) wrapper */
int u_getsockopt (int sd, int lev, int name, void *val, u_socklen_t *len)
{
#ifdef HAVE_GETSOCKOPT
    int rc = getsockopt(sd, lev, name, val, len);

#ifdef U_NET_TRACE
    u_dbg("getsockopt(%d, %d, %d, %p, %p) = %d", sd, lev, name, val, len, rc);
#endif

    dbg_err_sif (rc == -1);
err:
    return rc;
#else
    u_unused_args(sd, lev, name, val, len);
    u_info("getsockopt not implemented on this platform");
    return -1;
#endif  /* HAVE_GETSOCKOPT */
}

/**
 *  \brief  Mark the supplied socket descriptor as non-blocking
 *
 *  \param  s   a TCP socket descriptor
 *
 *  \retval  0  if successful
 *  \retval ~0  on error or fcntl(2) not implemented
 */ 
int u_net_set_nonblocking (int s)
{
#ifdef HAVE_FCNTL
    int flags;

    dbg_err_sif ((flags = fcntl(s, F_GETFL, 0)) == -1);
    flags |= O_NONBLOCK;
    dbg_err_sif (fcntl(s, F_SETFL, flags) == -1);
#else   /* !HAVE_FCNTL */
    warn_err("missing fcntl(2): can't set socket %d as non blocking", s);
#endif  /* HAVE_FCNTL */

    return 0;
err:
    return ~0;
}

/**
 *  \brief  Unset the non-blocking bit on the supplied socket descriptor.
 *
 *  \param  s   a TCP socket descriptor
 *
 *  \retval  0  if successful
 *  \retval ~0  on error or fcntl(2) not implemented
 */ int u_net_unset_nonblocking (int s)
{
#ifdef HAVE_FCNTL
    int flags;

    dbg_err_sif ((flags = fcntl(s, F_GETFL, 0)) == -1);
    flags &= ~O_NONBLOCK;
    dbg_err_sif (fcntl(s, F_SETFL, flags) == -1);
#else   /* !HAVE_FCNTL */
    warn_err("missing fcntl(2): can't reset non-blocking bit on socket %d", s);
#endif  /* HAVE_FCNTL */

    return 0;
err:
    return ~0;
}

/**
 *  \brief  Disable Nagle algorithm on the supplied TCP socket
 *
 *  \param  s   a TCP socket descriptor
 *
 *  \retval  0  if successful or \c TCP_NODELAY not implemented
 *  \retval ~0  on error
 */ 
int u_net_nagle_off (int s)
{
#if defined(HAVE_TCP_NODELAY) && !defined(__minix)
    int y = 1;

    dbg_return_if (s < 0, ~0);

    dbg_err_if (u_setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &y, sizeof y) == -1);

    return 0;
err:
    return ~0;
#else   /* !HAVE_TCP_NODELAY || __minix */
    u_unused_args(s);
    u_dbg("TCP_NODELAY not supported on this platform");
    return 0;
#endif  /* HAVE_TCP_NODELAY && !__minix */
}

/** \brief  Wrapper to inet_ntop(3) */
const char *u_inet_ntop (int af, const void *src, char *dst, u_socklen_t len)
{
    const unsigned char *p = (const unsigned char *) src;

    if (af == AF_INET)
    {
        char s[U_INET_ADDRSTRLEN];

        dbg_if (u_snprintf(s, sizeof s, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]));

        if (u_strlcpy(dst, s, len))
        {
            errno = ENOSPC;
            return NULL; 
        }
        
        return dst;
    }
#ifndef NO_IPV6
    if (af == AF_INET6)
        return inet_ntop(af, src, dst, len);
#endif  /* !NO_IPV6 */

    errno = EAFNOSUPPORT;
    return NULL;
}

/** \brief  Pretty print the given sockaddr (path, or address and port) */
const char *u_sa_ntop (const struct sockaddr *sa, char *d, size_t dlen)
{
    char a[256];    /* should be big enough for UNIX sock pathnames */

    dbg_err_if (sa == NULL);
    dbg_err_if (d == NULL);
    dbg_err_if (dlen == 0);

    switch (sa->sa_family)
    {
        case AF_INET: 
        {
            const struct sockaddr_in *s4 = (const struct sockaddr_in *) sa;
            dbg_err_if (!u_inet_ntop(AF_INET, &s4->sin_addr, a, sizeof a));
            dbg_if (u_snprintf(d, dlen, "%s:%d", a, ntohs(s4->sin_port)));
            break;
        }
#ifndef NO_IPV6
        case AF_INET6: 
        {
            const struct sockaddr_in6 *s6 = (const struct sockaddr_in6 *) sa;
            dbg_err_if (!u_inet_ntop(AF_INET6, &s6->sin6_addr, a, sizeof a));
            dbg_if (u_snprintf(d, dlen, "[%s]:%d", a, ntohs(s6->sin6_port)));
            break;
        }
#endif  /* !NO_IPV6 */
#ifndef NO_UNIXSOCK
        case AF_UNIX:
        {
            const struct sockaddr_un *su = (const struct sockaddr_un *) sa;
            dbg_if (u_strlcpy(d, su->sun_path, dlen));
            break;
        }
#endif  /* !NO_UNIXSOCK */
        default:
            dbg_err("unhandled socket type");
    }

    return d;
err:
    return "(sockaddr conversion failed)";
}

/**
 *  \}
 */

/* - 'wf' is one of do_ssock() or do_csock() 
 * - if 'psa' is not NULL it will receive the connected/bound sockaddr back */
static int do_sock (
        int wf (struct sockaddr *, u_socklen_t, int, int, int, int, int,
            struct timeval *), 
        struct u_net_addr_s *a, int backlog, u_addrinfo_t **pai,
        struct timeval *timeout)
{
    int sock_type, sd = -1;
    u_addrinfo_t *ap;
    char h[1024];

    dbg_return_if ((ap = a->addr) == NULL, ~0);

    for ( ; ap != NULL; ap = ap->ai_next)
    {
#ifndef NO_SCTP
        /* override .ai_socktype for SCTP one-to-many sockets with 
         * SOCK_SEQPACKET.  the default is SOCK_STREAM, see scheme_mapper() 
         * in sctp[46] branches */ 
        sock_type = (ap->ai_protocol == IPPROTO_SCTP && 
                (a->opts & U_NET_OPT_SCTP_ONE_TO_MANY)) ? 
            SOCK_SEQPACKET : ap->ai_socktype;
#else
        sock_type = ap->ai_socktype;
#endif  /* !NO_SCTP */

        if ((sd = wf(ap->ai_addr, ap->ai_addrlen, ap->ai_family, sock_type, 
                    ap->ai_protocol, a->opts, backlog, timeout)) >= 0)
        {
            /* TODO call getsockname to catch the real connected/bound address 
             * to feed 'pai' */

            u_info("%s %s", (wf == do_ssock) ? "bind succeeded for" 
                    : "connect succeeded to", (ap->ai_family == AF_UNIX) 
                    ? ap->ai_canonname : u_sa_ntop(ap->ai_addr, h, sizeof h));

            /* if caller has supplied a valid 'pai', copy out the pointer 
             * to the addrinfo that has fullfilled the connection request */
            if (pai)
                *pai = ap;

            break;
        }
    }

    if (sd == -1)
    {
        u_addrinfo_t *wai = a->addr;

        u_info("could not %s %s", (wf == do_ssock) ? "bind" : "connect to", 
                (wai->ai_family == AF_UNIX) ? wai->ai_canonname 
                : u_sa_ntop(wai->ai_addr, h, sizeof h));
    }

    return sd;
}

static int do_csock (struct sockaddr *sad, u_socklen_t sad_len, int domain, 
        int type, int protocol, int opts, int dummy, struct timeval *timeout)
{
    int s = -1;
#ifdef HAVE_SO_BROADCAST
    int bc = 1;
#endif  /* HAVE_SO_BROADCAST */

    u_unused_args(dummy);

    dbg_return_if (sad == NULL, -1);

    dbg_err_sif ((s = u_socket(domain, type, protocol)) == -1);

    if ((type == SOCK_DGRAM) && (opts & U_NET_OPT_DGRAM_BROADCAST))
    {
#ifdef HAVE_SO_BROADCAST
        dbg_err_sif (u_setsockopt(s, SOL_SOCKET, SO_BROADCAST, 
                    &bc, sizeof bc) == -1);
#else
        u_warn("SO_BROADCAST is not defined on this platform");
#endif  /* HAVE_SO_BROADCAST */
    }

#ifndef NO_SCTP
    if (opts & U_NET_OPT_SCTP_ONE_TO_MANY)
        dbg_err_sif (sctp_enable_events(s, opts));
#endif  /* NO_SCTP */

    /* NOTE that by default UDP and SCTP sockets (not only TCP and UNIX) are 
     * connected.  For UDP this has a couple of important implications:
     * 1) the caller must use u{,_net}_write for I/O instead of sendto 
     * 2) async errors are returned to the process. */
    if (!(opts & U_NET_OPT_DONT_CONNECT))
        dbg_err_sif (u_connect_ex(s, sad, sad_len, timeout) < 0);

    return s;
err:
    U_CLOSE(s);
    return -1;
}

static int do_ssock (struct sockaddr *sad, u_socklen_t sad_len, int domain, 
        int type, int protocol, int opts, int backlog, struct timeval *dummy)
{
    int s = -1;

    dbg_return_if (sad == NULL, -1);
    u_unused_args(dummy);

    dbg_err_sif ((s = u_socket(domain, type, protocol)) == -1);
    
    /* 
     * by default address reuse is in place for all server sockets except 
     * AF_UNIX, unless the caller has set the U_NET_OPT_DONT_REUSE_ADDR option 
     */ 
    if (domain != AF_UNIX && !(opts & U_NET_OPT_DONT_REUSE_ADDR))
        dbg_err_if (bind_reuse_addr(s));

#ifndef NO_SCTP
    if (opts & U_NET_OPT_SCTP_ONE_TO_MANY)
        dbg_err_sif (sctp_enable_events(s, opts));
#endif

    dbg_err_sif (u_bind(s, (struct sockaddr *) sad, sad_len) == -1);

    /* only stream and seqpacket sockets enter the LISTEN state */
    if (type == SOCK_STREAM || type == SOCK_SEQPACKET)
        dbg_err_sif (u_listen(s, backlog) == -1);

    return s;
err:
    U_CLOSE(s);
    return -1;
}

/* change the rules used in validating addresses supplied in a bind(2) call 
 * so that reuse of local addresses is allowed */
static int bind_reuse_addr (int s)
{
    int y = 1;

    dbg_return_if (s < 0, -1);

    dbg_err_if (u_setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &y, sizeof y) == -1);

    return 0;
err:
    return ~0;
}

#ifndef NO_SCTP
/* change server/client notification settings for one-to-many SCTP sockets */
static int sctp_enable_events (int s, int o)
{
    struct sctp_event_subscribe e;

    e.sctp_data_io_event = 
        (o & U_NET_OPT_SCTP_DATA_IO_EVENT) ? 1 : 0;

    e.sctp_association_event = 
        (o & U_NET_OPT_SCTP_ASSOCIATION_EVENT) ? 1 : 0;

    e.sctp_address_event = 
        (o & U_NET_OPT_SCTP_ADDRESS_EVENT) ? 1 : 0;

    e.sctp_send_failure_event = 
        (o & U_NET_OPT_SCTP_SEND_FAILURE_EVENT) ? 1 : 0;

    e.sctp_peer_error_event = 
        (o & U_NET_OPT_SCTP_PEER_ERROR_EVENT) ? 1 : 0;
    
    e.sctp_shutdown_event = 
        (o & U_NET_OPT_SCTP_SHUTDOWN_EVENT) ? 1 : 0;
    
    e.sctp_partial_delivery_event = 
        (o & U_NET_OPT_SCTP_PARTIAL_DELIVERY_EVENT) ? 1 : 0;
    
    e.sctp_adaptation_layer_event = 
        (o & U_NET_OPT_SCTP_ADAPTATION_LAYER_EVENT) ? 1 : 0;
    
    e.sctp_authentication_event = 
        (o & U_NET_OPT_SCTP_AUTHENTICATION_EVENT) ? 1 : 0;

    dbg_err_if (u_setsockopt(s, IPPROTO_SCTP, SCTP_EVENTS, &e, sizeof e) == -1);

    return 0;
err:
    return ~0;
}
#endif  /* !NO_SCTP */

static int scheme_mapper (const char *scheme, u_net_scheme_map_t *map)
{
    dbg_return_if (scheme == NULL, ~0);
    dbg_return_if (map == NULL, ~0);

    if (!strcasecmp(scheme, "tcp4") || !strcasecmp(scheme, "tcp"))
    {
        map->addr_family = AF_INET;
        map->sock_type = SOCK_STREAM;
        map->proto  = IPPROTO_TCP;
    }
    else if (!strcasecmp(scheme, "udp4") || !strcasecmp(scheme, "udp"))
    {
        map->addr_family = AF_INET;
        map->sock_type = SOCK_DGRAM;
        map->proto  = IPPROTO_UDP;
    }
#ifndef NO_SCTP
    else if (!strcasecmp(scheme, "sctp4") || !strcasecmp(scheme, "sctp"))
    {
        map->addr_family = AF_INET;
        /* default to STREAM, will be set to SEQPACKET in case 
         * U_NET_OPT_SCTP_ONE_TO_MANY is set */
        map->sock_type = SOCK_STREAM;
        map->proto  = IPPROTO_SCTP;
    }
#endif  /* !NO_SCTP */
#ifndef NO_UNIXSOCK
    else if (!strcasecmp(scheme, "unix"))
    {
        map->addr_family = AF_UNIX;
        map->sock_type = SOCK_STREAM;   /* default to STREAM sockets */
        map->proto  = 0;
    }
#endif  /* !NO_UNIXSOCK */
#ifndef NO_IPV6
    else if (!strcasecmp(scheme, "tcp6"))
    {
        map->addr_family = AF_INET6;
        map->sock_type = SOCK_STREAM;
        map->proto  = IPPROTO_TCP;
    }
    else if (!strcasecmp(scheme, "udp6"))
    {
        map->addr_family = AF_INET6;
        map->sock_type = SOCK_DGRAM;
        map->proto  = IPPROTO_UDP;
    }
#ifndef NO_SCTP
    else if (!strcasecmp(scheme, "sctp6"))
    {
        map->addr_family = AF_INET6;
        /* default to STREAM, will be set to SEQPACKET in case 
         * U_NET_OPT_SCTP_ONE_TO_MANY is set */
        map->sock_type = SOCK_STREAM;
        map->proto  = IPPROTO_SCTP;
    }
#endif  /* !NO_SCTP */
#endif  /* !NO_IPV6 */
    else
        return ~0;

    return 0;
}

/* just malloc enough room for the address container and zero-out everything
 * but the 'mode' attribute */
static int na_new (u_net_mode_t mode, u_net_addr_t **pa)
{
    u_net_addr_t *a = NULL;

    dbg_return_if (pa == NULL, ~0);
    dbg_return_if (!U_NET_IS_MODE(mode), ~0);

    dbg_err_sif ((a = u_zalloc(sizeof(u_net_addr_t))) == NULL);

    a->mode = mode;
    a->opts = 0;
    a->addr = NULL;
    a->cur = NULL;

    *pa = a;

    return 0;
err:
    if (a) 
        u_net_addr_free(a);
    return ~0;
}

/* basically an ai_resolv() wrapper using u_net objects */
static int uri2addr (u_uri_t *u, u_net_scheme_map_t *m, u_net_addr_t *a)
{
    dbg_return_if (u == NULL, ~0);
    dbg_return_if (m == NULL, ~0);
    dbg_return_if (a == NULL, ~0);

    return ai_resolv(u_uri_get_host(u), u_uri_get_port(u), u_uri_get_path(u), 
            m->addr_family, m->sock_type, m->proto, 
            (a->mode == U_NET_SSOCK) ? 1 : 0, &a->addr);
}

/* 'rf' is one of: resolv_sin, resolv_sun or resolv_sin6 */ 
static int do_resolv (
        int rf (const char *, const char *, const char *, u_addrinfo_t *ai), 
        const char *host, const char *port, const char *path, int family, 
        int type, int proto, u_addrinfo_t **pai)
{
    u_addrinfo_t *ai = NULL;

    /* 'port' can be NULL when family == AF_UNIX */
    dbg_return_if (rf == NULL, ~0);
    dbg_return_if (pai == NULL, ~0);

    dbg_err_sif ((ai = u_zalloc(sizeof *ai)) == NULL);

    /* set header flags */
    ai->ai_flags = 0;           /* unused */
    ai->ai_family = family;
    ai->ai_socktype = type;
    ai->ai_protocol = proto;

    /* call the resolver (they can possibly change some attribute setting) */
    dbg_err_if (rf(host, port, path, ai));

    *pai = ai;

    return 0;
err:
    if (ai)
        ai_free(ai);
    return ~0;
}

#ifndef NO_UNIXSOCK
static int resolv_sun (const char *dummy1, const char *dummy2, 
        const char *path, u_addrinfo_t *ai)
{
    char *cname = NULL;
    /* This variable used to be called 'sun'.  Unfortunately, we can't call
     * it like this on OpenSolaris, because their C preprocessor sets the 
     * symbol 'sun' to the numeric constant '1' :) */
    struct sockaddr_un *sunix = NULL;

    u_unused_args(dummy1, dummy2);

    dbg_return_if (path == NULL, ~0);
    dbg_return_if (ai == NULL, ~0);

    dbg_err_sif ((sunix = u_zalloc(sizeof *sunix)) == NULL);

    sunix->sun_family = AF_UNIX;
    dbg_err_ifm (u_strlcpy(sunix->sun_path, path, sizeof sunix->sun_path), 
            "%s too long", path);

    ai->ai_protocol = 0;    /* override previous setting if any */
    ai->ai_addr = (struct sockaddr *) sunix;
    ai->ai_addrlen = sizeof *sunix;
    ai->ai_next = NULL;

    /* set cname to what was provided as 'path' by the caller */
    dbg_err_sif ((cname = u_strdup(path)) == NULL);
    ai->ai_canonname = cname;

    return 0;
err:
    U_FREE(sunix);
    U_FREE(cname);
    return ~0;
}
#endif  /* !NO_UNIXSOCK */

static void ai_free (u_addrinfo_t *ai)
{
    nop_return_if (ai == NULL, );

#ifdef HAVE_GETADDRINFO
    if (ai->ai_family == AF_UNIX)
    {
#endif
        /* UNIX addresses have been created "manually", i.e. not by means 
         * of getaddrinfo()  */
        U_FREE(ai->ai_addr);
        U_FREE(ai->ai_canonname);
        u_free(ai);
#ifdef HAVE_GETADDRINFO
    }
    else
        freeaddrinfo(ai); 
#endif

    return;
}

#ifdef HAVE_GETADDRINFO
static int ai_resolv (const char *host, const char *port, const char *path,
        int family, int type, int proto, int passive, u_addrinfo_t **pai)
{
    int e;
    const char *hostname, *servname;
    struct addrinfo hints, *ai = NULL;

    /* 'host', 'port' and 'path' check depend on 'family' */
    dbg_return_if (pai == NULL, ~0);

    /* early intercept UNIX sock creation requests and dispatch it straight 
     * to resolv_sun() */
    if (family == AF_UNIX)
    {
        return do_resolv(resolv_sun, NULL, NULL, path, family, type, 
                proto, pai);
    }

    dbg_return_if (host == NULL, ~0);
    dbg_return_if (port == NULL, ~0);

    /* 
     * Handle '*' for both host and port.
     */
    hostname = !strcmp(host, "*") ? NULL : host;
    servname = !strcmp(port, "*") ? NULL : port;

    memset(&hints, 0, sizeof hints);

    hints.ai_flags = passive ? AI_PASSIVE : 0;
    hints.ai_flags |= hostname ? AI_CANONNAME : 0;

    hints.ai_family = family;
    hints.ai_socktype = type;
    hints.ai_protocol = proto;

    switch ((e = getaddrinfo(hostname, servname, &hints, &ai)))
    {
        case 0:             /* ok */
            break;
        case EAI_SYSTEM:    /* system error returned in errno */
            dbg_err_sifm (1, "getaddrinfo failed");
        default:            /* gai specific error */
            dbg_err_ifm (1, "getaddrinfo failed: %s", gai_strerror(e));
    }

    *pai = ai;

    return 0;
err:
    if (ai)
        ai_free(ai);
    return ~0;
}

#else   /* !HAVE_GETADDRINFO */

static int ai_resolv (const char *host, const char *port, const char *path, 
        int family, int type, int proto, int passive, u_addrinfo_t **pai)
{
    u_unused_args(passive);

    /* dispatch is based on address family type */
    switch (family)
    {
        case AF_INET:
            return do_resolv(resolv_sin, host, port, NULL, family, type, 
                    proto, pai);
#ifndef NO_UNIXSOCK
        case AF_UNIX:
            return do_resolv(resolv_sun, NULL, NULL, path, family, type,
                    proto, pai);
#endif  /* !NO_UNIXSOCK */
#ifndef NO_IPV6
        case AF_INET6:
            return do_resolv(resolv_sin6, host, port, NULL, family, type, 
                    proto, pai);
#endif  /* !NO_IPV6 */
        default:
            dbg_return_ifm (1, ~0, "address family not supported");
    }
}

static int resolv_sin (const char *host, const char *port, 
        const char *dummy, u_addrinfo_t *ai)
{
    in_addr_t saddr;
    char *cname = NULL;
    struct hostent *hp = NULL;
    struct sockaddr_in *sin = NULL;

    u_unused_args(dummy);

    dbg_return_if (ai == NULL, ~0);
    dbg_return_if (host == NULL, ~0);
    dbg_return_if (port == NULL, ~0);

    dbg_err_sif ((sin = u_zalloc(sizeof *sin)) == NULL);

    sin->sin_family = AF_INET;
    dbg_err_if (resolv_port(port, &sin->sin_port));

    /* '*' is the wildcard address */
    if (!strcmp(host, "*"))
        sin->sin_addr.s_addr = htonl(INADDR_ANY);
    else
    {
        if (strspn(host, "0123456789.") != strlen(host)) /* !ipv4 dotted */
        {
            dbg_err_ifm ((hp = gethostbyname(host)) == NULL, 
                    "%s: %d", host, h_errno);
            dbg_err_if (hp->h_addrtype != AF_INET);
            memcpy(&sin->sin_addr.s_addr, hp->h_addr, sizeof(in_addr_t));
        }
        else if ((saddr = inet_addr(host)) != INADDR_NONE)
            sin->sin_addr.s_addr = saddr;
        else
            dbg_err("invalid host name: \'%s\'", host);
    }

    ai->ai_addr = (struct sockaddr *) sin;
    ai->ai_addrlen = sizeof *sin;
    ai->ai_next = NULL;

    /* set cname to what was provided as 'host' by the caller */
    dbg_err_sif ((cname = u_strdup(host)) == NULL);
    ai->ai_canonname = cname;

    return 0;
err:
    U_FREE(sin);
    U_FREE(cname);
    return ~0;
}

#ifndef NO_IPV6
static int resolv_sin6 (const char *host, const char *port, 
        const char *dummy, u_addrinfo_t *ai)
{
    char *cname = NULL;
    struct hostent *hp = NULL;
    struct sockaddr_in6 *sin6 = NULL;

    u_unused_args(dummy);

    dbg_return_if (ai == NULL, ~0);
    dbg_return_if (host == NULL, ~0);
    dbg_return_if (port == NULL, ~0);

    dbg_err_sif ((sin6 = u_zalloc(sizeof *sin6)) == NULL);

#ifdef SIN6_LEN
    /* the SIN6_LEN constant must be defined if the system supports the
     * length member for IPv6 socket address structure */
    sin6->sin6_len = sizeof *sin6;
#endif
    sin6->sin6_family = AF_INET6;
    dbg_err_if (resolv_port(port, &sin6->sin6_port));
    sin6->sin6_flowinfo = 0;

    /* '*' is the wildcard address */
    if (!strcmp(host, "*"))
        sin6->sin6_addr = in6addr_any;
    else
    {
        /* try DNS for an AAAA record of 'host' */
        if (!strchr(host, ':'))
        {
            /* XXX gethostbyname2(3) is deprecated by POSIX, anyway if we 
             * XXX reached here is because our platform is not that POSIX, 
             * XXX i.e. IPv6 is implemented but addrinfo is not available. */
            dbg_err_ifm ((hp = gethostbyname2(host, AF_INET6)) == NULL, 
                    "%s: %d", host, h_errno);
            dbg_err_if (hp->h_addrtype != AF_INET6);
            dbg_err_if (hp->h_length != sizeof(struct in6_addr));
            memcpy(&sin6->sin6_addr, hp->h_addr, hp->h_length);
        }
        else /* convert numeric address */
        {
            switch (inet_pton(AF_INET6, host, &sin6->sin6_addr))
            {
                case -1:
                    dbg_strerror(errno); 
                    /* log errno and fall through */
                case 0:
                    dbg_err("invalid IPv6 host name: \'%s\'", host);
                    /* log hostname and jump to err: */
                default:
                    break;
            }
        }
    }

    ai->ai_addr = (struct sockaddr *) sin6;
    ai->ai_addrlen = sizeof *sin6;
    ai->ai_next = NULL;

    /* set cname to what was provided as 'host' by the caller */
    dbg_err_sif ((cname = u_strdup(host)) == NULL);
    ai->ai_canonname = cname;

    return 0;
err:
    U_FREE(sin6);
    U_FREE(cname);
    return ~0;
}
#endif  /* !NO_IPV6 */

/* given a port string (both numeric and service), return a port in network 
 * format ready to be used in struct sockaddr_in{,6} */
static int resolv_port (const char *s_port, uint16_t *pin_port)
{
    int i_port;
    struct servent *se;

    dbg_return_if (s_port == NULL, ~0);
    dbg_return_if (pin_port == NULL, ~0);

    /* handle wildcard as ephemeral */
    if (!strcmp(s_port, "*"))
    {
        *pin_port = (uint16_t) 0; 
    }
    else if (strspn(s_port, "0123456789") == strlen(s_port)) /* numeric */
    {
        dbg_err_if (u_atoi(s_port, &i_port));
        dbg_err_ifm (i_port < 0 || i_port > 65535, "%s out of range", s_port);
        *pin_port = htons((uint16_t) i_port);
    }
    else    /* service name (don't mind about protocol) */
    {
        dbg_err_if ((se = getservbyname(s_port, NULL)) == NULL);
        *pin_port = (uint16_t) se->s_port;
    }

    return 0;
err:
    return ~0;
}
#endif  /* HAVE_GETADDRINFO */

static inline int update_timeout (struct timeval *timeout, 
        struct timeval *tstart)
{
    struct timeval now, telapsed;

    dbg_return_if (timeout == NULL, ~0);
    dbg_return_if (tstart == NULL, ~0);

    dbg_err_sif (gettimeofday(&now, NULL) == -1);

    /* Time elapsed since last interrupt or first call to select(). */
    u_timersub(&now, tstart, &telapsed);

    /* Update timeout value by subtracting the elapsed interval. */
    u_timersub(timeout, &telapsed, timeout);

    /* Assign new tstart value. */
    *tstart = now;

    return 0;
err:
    return ~0;
}
