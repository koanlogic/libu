/* $Id: main.c,v 1.3 2009/12/25 14:36:40 tho Exp $ */

#include <stdlib.h>
#include <u/libu.h>

int facility = LOG_LOCAL0;

int main (int argc, char *argv[])
{
    u_net_addr_t *a = NULL;
    int sd = -1, asd = -1, at;
    struct sockaddr sa;
    socklen_t sa_len;
    char buf[1024];
    ssize_t rb;

    con_err_ifm (argc != 2, "usage: %s <bind uri>", argv[0]);

    con_err_if (u_net_uri2addr(argv[1], &a));
    con_err_sif ((sd = u_net_sock_by_addr(a, U_NET_SSOCK)) == -1);

    if ((at = u_net_addr_get_type(a)) != U_NET_UDP4 && at != U_NET_UDP6)
        con_err_sif ((asd = u_accept(sd, &sa, &sa_len)) == -1);
    else
        asd = sd;

    con_err_sif ((rb = u_read(asd, buf, sizeof(buf))) == -1);
    con("read: %s", buf);

    u_net_addr_free(a);
    (void) close(sd);
    (void) close(asd);
    
    return EXIT_SUCCESS;
err:
    if (a)
        u_net_addr_free(a);
    U_CLOSE(sd);
    U_CLOSE(asd);
    return EXIT_FAILURE;
}
