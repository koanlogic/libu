/*
 * Copyright (c) 2005-2010 by KoanLogic s.r.l. - All rights reserved.
 */

#ifndef _U_NET_H_
#define _U_NET_H_

#include <u/libu.h>

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

#ifdef OS_WIN
  #include <windows.h>
  /* #include <winsock.h> not compatible with ws2tcpip.h */
  #include <winsock2.h>
  #include <ws2tcpip.h>
#endif  /* OS_WIN */

#ifndef HAVE_IN_ADDR_T
  typedef unsigned long in_addr_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define U_NET_BACKLOG 300

/* The Net module defines the following (private) URI types: 
 *    {tc,ud,sct}p[46]://<host>:<port> for TCP/UDP/SCTP over IPv[46] addresses,
 *    unix://<abs_path> for UNIX IPC endpoints. 
 * Internally, they are translated into an u_net_addr_t structure. */
struct u_net_addr_s;
typedef struct u_net_addr_s u_net_addr_t;

enum { 
    U_NET_TYPE_MIN = 0,
    U_NET_TCP4 = 1,
    U_NET_TCP6 = 2,
    U_NET_UDP4 = 3,
    U_NET_UDP6 = 4,
    U_NET_UNIX = 5,
    U_NET_SCTP4 = 6,
    U_NET_SCTP6 = 7,
    U_NET_TYPE_MAX = 8
};
#define U_NET_IS_VALID_ADDR_TYPE(a) (a > U_NET_TYPE_MIN && a < U_NET_TYPE_MAX)

/* socket creation semantics: server or client (the 'mode' in u_net_sock) */
enum { 
    U_NET_SSOCK = 0, 
    U_NET_CSOCK = 1 
};
#define U_NET_IS_MODE(m) (m == U_NET_SSOCK || m == U_NET_CSOCK)

/* opts for u_net_addr_{set,add}_opts() */
#define U_NET_OPT_DONT_REUSE_ADDR               (1 << 0)
#define U_NET_OPT_DONT_CONNECT                  (1 << 1)
#define U_NET_OPT_SCTP_ONE_TO_MANY              (1 << 2)
#define U_NET_OPT_SCTP_DATA_IO_EVENT            (1 << 3)
#define U_NET_OPT_SCTP_ASSOCIATION_EVENT        (1 << 4)
#define U_NET_OPT_SCTP_ADDRESS_EVENT            (1 << 5)
#define U_NET_OPT_SCTP_SEND_FAILURE_EVENT       (1 << 6)
#define U_NET_OPT_SCTP_PEER_ERROR_EVENT         (1 << 7)
#define U_NET_OPT_SCTP_SHUTDOWN_EVENT           (1 << 8)
#define U_NET_OPT_SCTP_PARTIAL_DELIVERY_EVENT   (1 << 9)
#define U_NET_OPT_SCTP_ADAPTATION_LAYER_EVENT   (1 << 10)
#define U_NET_OPT_SCTP_AUTHENTICATION_EVENT     (1 << 11)

/**
 * \addtogroup net
 *  \{
 */

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
int u_net_sock (const char *uri, int mode);
int u_net_sock_by_addr (u_net_addr_t *addr, int mode);

/* deprecated: use u_net_sock_by_addr instead */
int u_net_sock_tcp (u_net_addr_t *addr, int mode) __LIBU_DEPRECATED;
int u_net_sock_udp (u_net_addr_t *addr, int mode) __LIBU_DEPRECATED;
int u_net_sock_unix (u_net_addr_t *addr, int mode) __LIBU_DEPRECATED;
#ifndef NO_SCTP
int u_net_sock_sctp (u_net_addr_t *addr, int mode) __LIBU_DEPRECATED;
#endif  /* !NO_SCTP */

/* low-level socket creation */
int u_net_tcp4_ssock (struct sockaddr_in *sad, int opts, int backlog);
int u_net_tcp4_csock (struct sockaddr_in *sad, int opts);
int u_net_udp4_ssock (struct sockaddr_in *sad, int opts);
int u_net_udp4_csock (struct sockaddr_in *sad, int opts);
#ifndef NO_SCTP
int u_net_sctp4_ssock (struct sockaddr_in *sad, int opts, int backlog);
int u_net_sctp4_csock (struct sockaddr_in *sad, int opts);
#endif /* !NO_SCTP */
int u_net_uri2sin (const char *uri, struct sockaddr_in *sad);

#ifndef NO_IPV6
int u_net_tcp6_ssock (struct sockaddr_in6 *sad, int opts, int backlog);
int u_net_tcp6_csock (struct sockaddr_in6 *sad, int opts);
int u_net_udp6_ssock (struct sockaddr_in6 *sad, int opts);
int u_net_udp6_csock (struct sockaddr_in6 *sad, int opts);
#ifndef NO_SCTP
int u_net_sctp6_ssock (struct sockaddr_in6 *sad, int opts, int backlog);
int u_net_sctp6_csock (struct sockaddr_in6 *sad, int opts);
#endif /* !NO_SCTP */
int u_net_uri2sin6 (const char *uri, struct sockaddr_in6 *sad);
#endif /* !NO_IPV6 */

#ifndef NO_UNIXSOCK
int u_net_unix_ssock (struct sockaddr_un *sad, int opts, int backlog);
int u_net_unix_csock (struct sockaddr_un *sad, int opts);
int u_net_uri2sun (const char *uri, struct sockaddr_un *sad);
#endif /* !NO_UNIXSOCK */

/* address translation: uri string -> u_uri -> u_net_addr -> struct sockaddr */
int u_net_uri2addr (const char *uri, u_net_addr_t **pa);
int u_net_addr_new (int type, u_net_addr_t **pa);
void u_net_addr_free (u_net_addr_t *addr);

/* address accessors */
int u_net_addr_get_type (u_net_addr_t *a);
struct sockaddr *u_net_addr_get_sa (u_net_addr_t *a);

/* address opts manipulation (applies to socket creation) */
void u_net_addr_set_opts (u_net_addr_t *addr, int opts);
void u_net_addr_add_opts (u_net_addr_t *addr, int opts);

/* misc */
int u_net_nagle_off (int sd);

/* networking syscall wrappers */
int u_socket (int domain, int type, int protocol);
#ifdef HAVE_SOCKLEN_T
int u_connect (int sd, const struct sockaddr *addr, socklen_t addrlen);
int u_accept(int ld, struct sockaddr *addr, socklen_t *addrlen);
int u_bind (int sd, const struct sockaddr *addr, socklen_t addrlen);
int u_setsockopt (int sd, int lev, int oname, const void *oval, socklen_t olen);
int u_getsockopt (int sd, int lev, int oname, void *oval, socklen_t *olen);
const char *u_inet_ntop (int af, const void *src, char *dst, socklen_t len);
#else
int u_connect (int sd, const struct sockaddr *addr, int addrlen);
int u_accept(int ld, struct sockaddr *addr, int *addrlen);
int u_bind (int sd, const struct sockaddr *addr, int addrlen);
int u_setsockopt (int sd, int lev, int oname, const void *oval, int olen);
int u_getsockopt (int sd, int lev, int oname, void *oval, int *olen);
const char *u_inet_ntop (int af, const void *src, char *dst, size_t len);
#endif  /* HAVE_SOCKLEN_T */
int u_listen (int sd, int backlog);
int u_inet_pton (int af, const char *src, void *dst);
const char *u_sa_ntop (const struct sockaddr *sa, char *dst, size_t dst_len);

#ifdef HAVE_GETADDRINFO
int u_net_sock_by_ai (struct addrinfo *ai, int mode, int opts, 
        struct sockaddr **psa);
int u_net_uri2ai (const char *uri, struct addrinfo **pai);
int u_net_resolver (const char *host, const char *port, int family, int type,
        int proto, int passive, struct addrinfo **pai);
#endif  /* HAVE_GETADDRINFO */

#ifdef __cplusplus
}
#endif

#endif /* !_U_NET_H_ */
