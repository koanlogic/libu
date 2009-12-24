/* $Id: main.c,v 1.2 2009/12/24 16:39:00 tho Exp $ */

#include <stdlib.h>
#include <u/libu.h>

int facility = LOG_LOCAL0;

int main (int argc, char *argv[])
{
    int sd = -1, asd = -1;
    struct sockaddr sa;
    socklen_t sa_len;
    char buf[1024];
    ssize_t rb;

    con_err_ifm (argc != 2, "usage: %s <bind uri>", argv[0]);

    con_err_sif ((sd = u_net_sock(argv[1], U_NET_SSOCK)) == -1);
    con_err_sif ((asd = u_accept(sd, &sa, &sa_len)) == -1);
    con_err_sif ((rb = u_read(asd, buf, sizeof(buf))) == -1);
    con("read: %s", buf);

    (void) close(sd);
    (void) close(asd);
    
    return EXIT_SUCCESS;
err:
    U_CLOSE(sd);
    U_CLOSE(asd);
    return EXIT_FAILURE;
}
