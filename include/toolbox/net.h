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

#ifndef HAVE_SOCKLEN_T
  typedef int u_socklen_t;
#else   /* !HAVE_SOCKLEN_T */
  typedef socklen_t u_socklen_t;
#endif  /* HAVE_SOCKLEN_T */

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

/* socket creation semantics: server or client (the 'mode' in u_net_sock) */
enum { 
    U_NET_SSOCK = 0, 
    U_NET_CSOCK = 1 
};
#define U_NET_IS_MODE(m) (m == U_NET_SSOCK || m == U_NET_CSOCK)

/** \brief  Options possibly used when creating the socket */
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
int u_net_sock (const char *uri, int mode, ...);
int u_net_sock_by_addr (u_net_addr_t *a);

/* address ctor/dtor */
int u_net_uri2addr (const char *uri, int mode, u_net_addr_t **pa);
void u_net_addr_free (u_net_addr_t *a);

/* address opts manipulation (applies to socket creation) */
void u_net_addr_set_opts (u_net_addr_t *a, int opts);
void u_net_addr_add_opts (u_net_addr_t *a, int opts);

/* misc */
int u_net_nagle_off (int sd);

/* networking syscall wrappers */
int u_socket (int domain, int type, int protocol);
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
