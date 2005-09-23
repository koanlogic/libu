/* 
 * Copyright (c) 2005, KoanLogic s.r.l. - All rights reserved.  
 */

static const char rcsid[] =
    "$Id: net.c,v 1.1 2005/09/23 13:04:38 tho Exp $";

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>

#include <u/net.h>
#include <u/uri.h>
#include <u/debug.h>
#include <u/misc.h>
#include <u/alloc.h>

/**
 *  \defgroup net Inter Process/Machine Communication
 *  \{
 */

int u_net_sock (const char *uri, int mode)
{
    int sd = -1;
    u_net_addr_t *addr = NULL;

    dbg_return_if (uri == NULL, -1);
    dbg_return_if (mode != U_NET_SSOCK && mode != U_NET_CSOCK, -1);
    
    dbg_err_if (u_net_uri2addr(uri, &addr));

    switch (addr->type)
    {
        case U_NET_TCP4:
        case U_NET_TCP6:
            sd = u_net_sock_tcp(addr, mode);
            break;
#ifdef OS_UNIX
        case U_NET_UNIX:
            sd = u_net_sock_unix(addr, mode);
            break;
#endif /* OS_UNIX */
        case U_NET_UDP4:
        case U_NET_UDP6:
        default:
           warn_err("access method not implemented");
    }

    dbg_if (sd == -1);
    u_net_addr_free(addr);
    return sd;

err:
    if (addr)
        u_net_addr_free(addr);
    return -1;
}

int u_net_sock_tcp (u_net_addr_t *a,  int mode)
{
    dbg_return_if (a == NULL, -1);
    dbg_return_if (a->type != U_NET_TCP4 || a->type != U_NET_TCP6, -1);

    switch (mode)
    {
        case U_NET_CSOCK:
            return a->type == U_NET_TCP4 ?
                u_net_tcp4_csock(&a->sa.sin) :
                u_net_tcp6_csock(&a->sa.sin6);
        case U_NET_SSOCK:
            return a->type == U_NET_TCP4 ?
                u_net_tcp4_ssock(&a->sa.sin, 0, U_NET_BACKLOG) :
                u_net_tcp6_ssock(&a->sa.sin6, 0, U_NET_BACKLOG);
        default:
            warn("unknown socket mode");
            return -1;
    }
}

int u_net_sock_unix (u_net_addr_t *a, int mode)
{
    dbg_return_if (a == NULL, -1);
    switch (mode) { default: break; } /* TODO */
    return -1;
}

int u_net_sock_udp (u_net_addr_t *a,  int mode)
{
    dbg_return_if (a == NULL, -1);
    dbg_return_if (a->type != U_NET_UDP4 || a->type != U_NET_UDP6, -1);
    switch (mode) { default: break; } 
    return -1;
}

int u_net_tcp6_ssock (struct sockaddr_in6 *sad, int reuse, int backlog)
{
    int rv, lsd = -1;
    int on = reuse ? 1 : 0;

    dbg_return_if (sad == NULL, -1);
    
    lsd = socket(AF_INET6, SOCK_STREAM, 0);
    dbg_err_if (lsd == -1);

    rv = setsockopt(lsd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on);
    dbg_err_if (rv == -1);

    rv = bind(lsd, (struct sockaddr *) sad, sizeof *sad);
    dbg_err_if (rv == -1);

    rv = listen(lsd, backlog);
    dbg_err_if (rv == -1);

    return lsd;

err:
    dbg_strerror(errno);
    U_CLOSE(lsd);
    return -1;
}

int u_net_tcp4_ssock (struct sockaddr_in *sad, int reuse, int backlog)
{
    int rv, lsd = -1;
    int on = reuse ? 1 : 0;

    dbg_return_if (sad == NULL, -1);
    
    lsd = socket(AF_INET, SOCK_STREAM, 0);
    dbg_err_if (lsd == -1);

    rv = setsockopt(lsd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on);
    dbg_err_if (rv == -1);

    rv = bind(lsd, (struct sockaddr *) sad, sizeof *sad);
    dbg_err_if (rv == -1);

    rv = listen(lsd, backlog);
    dbg_err_if (rv == -1);

    return lsd;

err:
    dbg_strerror(errno);
    U_CLOSE(lsd);
    return -1;
}

int u_net_tcp4_csock (struct sockaddr_in *sad)
{
    int csd = -1;

    dbg_return_if (sad == NULL, -1);

    csd = socket(AF_INET, SOCK_STREAM, 0);
    dbg_err_if (csd == -1);

    dbg_err_if (connect(csd, (struct sockaddr *) sad, sizeof *sad) == -1);

    return csd;

err:
    dbg_strerror(errno);
    U_CLOSE(csd);
    return -1;
}

int u_net_tcp6_csock (struct sockaddr_in6 *sad)
{
    int csd = -1;
    int rv;

    dbg_return_if (sad == NULL, -1);

    csd = socket(AF_INET6, SOCK_STREAM, 0);
    dbg_err_if (csd == -1);

    rv = connect(csd, (struct sockaddr *) sad, sizeof *sad);
    dbg_err_if (rv == -1);

    return csd;

err:
    dbg_strerror(errno);
    U_CLOSE(csd);
    return -1;
}

int u_net_uri2addr (const char *uri, u_net_addr_t **pa)
{
    u_uri_t *u = NULL;
    u_net_addr_t *a = NULL;
    
    dbg_return_if (uri == NULL, ~0);
    dbg_return_if (pa == NULL, ~0);

    *pa = NULL;
    
    dbg_err_if (u_uri_parse(uri, &u));

    if (!strcasecmp(u->scheme, "tcp4"))
    {
        dbg_err_if (u_net_addr_new(U_NET_TCP4, &a));
        dbg_err_if (u_net_uri2sin(u, &a->sa.sin));
    }
#ifdef OS_UNIX
    else if (!strcasecmp(u->scheme, "unix"))
    {    
        dbg_err_if (u_net_addr_new(U_NET_UNIX, &a));
        dbg_err_if (u_net_uri2sun(u, &a->sa.sun));
    }
#endif /* OS_UNIX */    
    else /* tcp6, udp[46] unsupported */
        warn_err("unsupported URI scheme: %s", u->scheme); 
    
    *pa = a;
    u_uri_free(u);
    return 0;
err:
    if (a) u_net_addr_free(a);
    if (u) u_uri_free(u);
    return ~0;
}

#ifdef OS_UNIX
int u_net_uri2sun (u_uri_t *uri, struct sockaddr_un *sad)
{
    dbg_return_if (uri == NULL, ~0);
    dbg_return_if (sad == NULL, ~0);
    /* TODO */
    return ~0;
}
#endif /* OS_UNIX */

int u_net_addr_new (int type, u_net_addr_t **pa)
{
    u_net_addr_t *a = NULL;

    dbg_return_if (pa == NULL, ~0);
    dbg_return_if (type <= U_NET_TYPE_MIN || type >= U_NET_TYPE_MAX, ~0);

    a = u_zalloc(sizeof(u_net_addr_t));
    dbg_err_if (a == NULL);
    a->type = type;
    *pa = a;

err:
    if (a) u_net_addr_free(a);
    return ~0;
}

void u_net_addr_free (u_net_addr_t *a)
{
    U_FREE(a);
    return;
}
   
/* translate u_uri_t into struct sockaddr_in */
int u_net_uri2sin (u_uri_t *uri, struct sockaddr_in *sad)
{
    struct hostent *hp = NULL;
    in_addr_t saddr;

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
        warn_err("invalid host name: \'%s\'", uri->host);

    return 0;
err:
    return ~0;    
}

/**
 * \brief   Wrapper function around write(2) for stream sockets
 *
 * Try to write a chunk of \a nbytes bytes starting from \a buf to the 
 * object referenced by the descriptor \a sd.
 * 
 * \param sd        the file descriptor on which write(2) is performed
 * \param buf       the data chunk starts from this position
 * \param nbytes    the number of bytes for this chunk
 *  
 * \return  A \c ~0 is returned if a write(2) error other than \c EINTR or
 *          \c EAGAIN has occurred, or if the requested amount of data could 
 *          not be entirely written.  A \c 0 is returned on success.
 */ 
int u_net_write (int sd, void *buf, size_t nbytes)
{
    return u_net_io(write, sd, buf, nbytes);
}

/**
 * \brief   Wrapper function around read(2) for stream sockets
 *
 * Try to read - atomically - a chunk of \a nbytes bytes from the object 
 * referenced by the descriptor \p sd and place it into the buffer starting 
 * at \p buf.
 *
 * \param sd        the file descriptor on which read(2) is performed
 * \param buf       the data chunk is placed starting from this position
 * \param nbytes    the number of bytes for this chunk
 * 
 * \return  A \c ~0 is returned if a read(2) error other than \c EINTR or 
 *          \c EAGAIN has occurred, or if the requested amount of data could 
 *          not be entirely read.  A \c 0 is returned on success.
 */
int u_net_read (int sd, void *buf, size_t nbytes)
{
    return u_net_io(read, sd, buf, nbytes);
}

int u_net_io (ssize_t (*f) (int, void *, size_t), int sd, void *buf, size_t l)
{
    char *p = buf;
    ssize_t nret;
    size_t nleft = 0;

    while (l > nleft) 
    {
        if ((nret = (f) (sd, p + nleft, l - nleft)) == -1)
        {
            if (errno == EINTR || errno == EAGAIN)
                continue;
            else
            {
                warn_strerror(errno);
                return ~0;
            }
        }

        /* test EOF */
        if (nret == 0)
            goto end;
        
        nleft += nret;
    }

end:
    return nleft ? ~0 : 0;
}

/**
 *      \}
 */
