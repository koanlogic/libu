/*
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.
 */

#ifndef _U_NET_H_
#define _U_NET_H_

#include <u/libu_conf.h>

#ifdef OS_UNIX
  #include <sys/types.h>
  #ifdef HAVE_SYS_SOCKET
    #include <sys/socket.h>
  #endif
  #ifdef HAVE_NETINET_IN
    #include <netinet/in.h>
  #endif
  #ifdef HAVE_NETINET_TCP
    #include <netinet/tcp.h>
  #endif
  #ifdef HAVE_NETINET_SCTP
    #include <netinet/sctp.h>
  #endif
  #ifdef HAVE_SYSUIO
    #include <sys/uio.h>
  #endif
  #include <arpa/inet.h>
  #include <netdb.h>
  #ifndef NO_UNIXSOCK
    #include <sys/un.h>
  #endif
#endif  /* OS_UNIX */

#ifdef HAVE_FCNTL
  #include <fcntl.h>
#endif  /* HAVE_FCNTL */

#ifdef OS_WIN
  #include <windows.h>
  /* #include <winsock.h> not compatible with ws2tcpip.h */
  #include <winsock2.h>
  #include <ws2tcpip.h>

  #define EINPROGRESS   WSAEWOULDBLOCK
  #define ETIMEDOUT     WSAETIMEDOUT
  #define EAFNOSUPPORT  WSAEAFNOSUPPORT
#endif  /* OS_WIN */

#ifndef HAVE_IN_ADDR_T
  typedef unsigned long in_addr_t;
#endif

#ifndef HAVE_SOCKLEN_T
  typedef int u_socklen_t;
#else   /* HAVE_SOCKLEN_T */
  typedef socklen_t u_socklen_t;
#endif  /* !HAVE_SOCKLEN_T */

#ifndef HAVE_INET_ADDRSTRLEN
#define U_INET_ADDRSTRLEN 16
#else   /* HAVE_INET_ADDRSTRLEN */
#define U_INET_ADDRSTRLEN   INET_ADDRSTRLEN
#endif  /* !HAVE_INET_ADDRSTRLEN */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \addtogroup net
 *  \{
 */

/** \brief  Default backlog queue size supplied to listen(2) */
#define U_NET_BACKLOG 300

/* forward decl */
struct u_net_addr_s;

/** \brief  Base type of the net module: holds all the addressing and 
 *          semantics information needed when creating the corresponding
 *          socket */
typedef struct u_net_addr_s u_net_addr_t;

/** \brief  Socket creation semantics: passive or active, i.e. the \p mode in 
 *          ::u_net_sd and ::u_net_uri2addr */
typedef enum { 
    U_NET_SSOCK = 0,    /**< the address is used for a passive socket */
    U_NET_CSOCK = 1     /**< the address is used for an active socket */
} u_net_mode_t;
#define U_NET_IS_MODE(m) (m == U_NET_SSOCK || m == U_NET_CSOCK)


/** \brief  Address options, can be supplied (by or'ing them together through 
 *          ::u_net_addr_set_opts and/or ::u_net_addr_add_opts, in order to 
 *          tweek the socket creation machinery */
typedef enum {
    U_NET_OPT_DONT_REUSE_ADDR = (1 << 0),
    /**<  Disable local address reuse when creating a passive socket */

    U_NET_OPT_DONT_CONNECT = (1 << 1),  
    /**< Do not \c connect(2) when creating an (UDP) active socket */

    U_NET_OPT_SCTP_ONE_TO_MANY = (1 << 2),  
    /**< Use one-to-many model when creating SCTP sockets */ 

    U_NET_OPT_SCTP_DATA_IO_EVENT = (1 << 3),
    /**< SCTP only: get extra information as ancillary data with the 
     *   receive calls, e.g. the stream number */

    U_NET_OPT_SCTP_ASSOCIATION_EVENT = (1 << 4),
    /**< SCTP only: get notification about changes in associations, 
     *   including the arrival of new associations */

    U_NET_OPT_SCTP_ADDRESS_EVENT = (1 << 5),
    /**< SCTP only: get notfication when some event occurs concerning one 
     *   of the peer's addresses, e.g. addition, deletion, reachability, 
     *   unreachability */

    U_NET_OPT_SCTP_SEND_FAILURE_EVENT = (1 << 6),
    /**< SCTP only: when a send fails, the data is returned with an error */

    U_NET_OPT_SCTP_PEER_ERROR_EVENT = (1 << 7),
    /**< SCTP only: get peer error messages (as TLV) from the stack */

    U_NET_OPT_SCTP_SHUTDOWN_EVENT = (1 << 8),
    /**< SCTP only: get notifications about the peer having closed or 
     *   shutdown an association */

    U_NET_OPT_SCTP_PARTIAL_DELIVERY_EVENT = (1 << 9),
    /**< SCTP only: get notified about issues that may occur in the partial 
     *   delivery API */

    U_NET_OPT_SCTP_ADAPTATION_LAYER_EVENT = (1 << 10),
    /**< SCTP only: get events coming from the adaptation layer */

    U_NET_OPT_SCTP_AUTHENTICATION_EVENT = (1 << 11),
    /**< SCTP only: get authentication events, e.g. activation of new keys */

    U_NET_OPT_DGRAM_BROADCAST = (1 << 20)
    /**< DGRAM only: automatically sets broadcast option in client socket using
     *   setsockopt() */

} u_net_opts_t;

#ifdef HAVE_GETADDRINFO
typedef struct addrinfo u_addrinfo_t;
#else
struct u_addrinfo_s;
typedef struct u_addrinfo_s u_addrinfo_t;
#endif  /* HAVE_GETADDRINFO */

/** \brief ::u_io specialisation for output ops */
#define u_net_write(sd, buf, nbytes, nw, iseof) \
    u_io((iof_t) write, sd, buf, nbytes, nw, iseof)

/** \brief ::u_io specialisation for input ops */
#define u_net_read(sd, buf, nbytes, nr, iseof) \
    u_io(read, sd, buf, nbytes, nr, iseof)

/** \brief  Try to write a chunk of \p nbytes data to descriptor \p sd */
#define u_net_writen(sd, buf, nbytes) \
    u_io((iof_t) write, sd, buf, nbytes, NULL, NULL)

/** \brief  Try to read a chunk of \p nbytes data from descriptor \p sd */
#define u_net_readn(sd, buf, nbytes) \
    u_io(read, sd, buf, nbytes, NULL, NULL)

/**
 *  \}
 */

/* hi-level socket creation */
int u_net_sd_ex (const char *uri, u_net_mode_t mode, int opts, 
        struct timeval *timeout);
int u_net_sd (const char *uri, u_net_mode_t mode, int opts);
int u_net_sd_by_addr_ex (u_net_addr_t *a, struct timeval *timeout);
int u_net_sd_by_addr (u_net_addr_t *a);

/* address ctor/dtor & co. */
int u_net_uri2addr (const char *uri, u_net_mode_t mode, u_net_addr_t **pa);
void u_net_addr_free (u_net_addr_t *a);
int u_net_addr_can_accept (u_net_addr_t *a);

/* address opts manipulation (applies to socket creation) */
void u_net_addr_set_opts (u_net_addr_t *a, int opts);
void u_net_addr_add_opts (u_net_addr_t *a, int opts);

/* misc */
int u_net_nagle_off (int sd);
int u_net_set_nonblocking (int sd);
int u_net_unset_nonblocking (int sd);

/* networking syscall wrappers */
int u_socket (int domain, int type, int protocol);
int u_connect_ex (int sd, const struct sockaddr *addr, u_socklen_t addrlen,
        struct timeval *timeout);
int u_connect (int sd, const struct sockaddr *addr, u_socklen_t addrlen);
int u_listen (int sd, int backlog);
int u_accept(int ld, struct sockaddr *addr, u_socklen_t *addrlen);
int u_bind (int sd, const struct sockaddr *addr, u_socklen_t addrlen);
int u_setsockopt (int sd, int lev, int name, const void *val, u_socklen_t len);
int u_getsockopt (int sd, int lev, int name, void *val, u_socklen_t *len);

/* addressing conversion */
const char *u_sa_ntop (const struct sockaddr *sa, char *dst, size_t dst_len);
const char *u_inet_ntop (int af, const void *src, char *dst, u_socklen_t len);

#ifdef __cplusplus
}
#endif

#endif /* !_U_NET_H_ */
