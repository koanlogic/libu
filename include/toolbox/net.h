/*
 * Copyright (c) 2005-2009 by KoanLogic s.r.l. - All rights reserved.
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
 *    {tc,ud}p[46]://<host>:<port> for TCP/UDP over IPv[46] addresses,
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
    U_NET_TYPE_MAX = 6
};
#define U_NET_IS_VALID_ADDR_TYPE(a) (a > U_NET_TYPE_MIN && a < U_NET_TYPE_MAX)

/* socket creation semantics: server or client (the 'mode' in u_net_sock) */
enum { 
    U_NET_SSOCK = 0, 
    U_NET_CSOCK = 1 
};
#define U_NET_IS_MODE(m) (m == U_NET_SSOCK || m == U_NET_CSOCK)

/**
 * \addtogroup net
 *  \{
 */

/** \brief u_io specialisation for output ops */
#define u_net_write(sd, buf, nbytes, nw, iseof) \
    u_io((iof_t) write, sd, buf, nbytes, nw, iseof)

/** \brief u_io specialisation for input ops */
#define u_net_read(sd, buf, nbytes, nw, iseof) \
    u_io(read, sd, buf, nbytes, nw, iseof)

/** \brief  Try to write a chunk of \p nbytes data to descriptor \p sd */
#define u_net_writen(sd, buf, nbytes) \
    u_io((iof_t) write, sd, buf, nbytes, 0, 0)

/** \brief  Try to read a chunk of \p nbytes data from descriptor \p sd */
#define u_net_readn(sd, buf, nbytes) \
    u_io(read, sd, buf, nbytes, 0, 0)

/**
 *  \}
 */

/* hi-level socket creation */
int u_net_sock (const char *uri, int mode);
int u_net_sock_by_addr (u_net_addr_t *addr, int mode);

int u_net_sock_tcp (u_net_addr_t *addr, int mode) __LIBU_DEPRECATED;
int u_net_sock_udp (u_net_addr_t *addr, int mode) __LIBU_DEPRECATED;
int u_net_sock_unix (u_net_addr_t *addr, int mode) __LIBU_DEPRECATED;

/* low-level socket creation */
int u_net_tcp4_ssock (struct sockaddr_in *sad, int reuse, int backlog);
int u_net_tcp4_csock (struct sockaddr_in *sad);
int u_net_uri2sin (u_uri_t *uri, struct sockaddr_in *sad);

#ifndef NO_IPV6
int u_net_tcp6_ssock (struct sockaddr_in6 *sad, int reuse, int backlog);
int u_net_tcp6_csock (struct sockaddr_in6 *sad);
int u_net_uri2sin6 (const char *uri, struct sockaddr_in6 *sad);
#endif /* !NO_IPV6 */

#ifndef NO_UNIXSOCK
int u_net_unix_ssock (struct sockaddr_un *sad, int backlog);
int u_net_unix_csock (struct sockaddr_un *sad);
int u_net_uri2sun (const char *uri, struct sockaddr_un *sad);
#endif /* !NO_UNIXSOCK */

/* address translation: uri string -> u_uri -> u_net_addr -> struct sockaddr */
int u_net_uri2addr (const char *uri, u_net_addr_t **pa);
int u_net_addr_new (int type, u_net_addr_t **pa);
void u_net_addr_free (u_net_addr_t *addr);

/* misc */
int u_net_nagle_off (int sd);
int u_accept(int ld, struct sockaddr *addr, int *addrlen);

#ifdef __cplusplus
}
#endif

#endif /* !_U_NET_H_ */
