/* $Id: main.c,v 1.6 2009/12/23 18:14:10 tho Exp $ */

#include <stdlib.h>
#include <u/libu.h>

int facility = LOG_LOCAL0;

int main (int argc, char *argv[])
{
    int csd = -1;

    con_err_ifm (argc != 2, "usage: %s <server_uri>", argv[0]);

    con_err_sif ((csd = u_net_sock(argv[1], U_NET_CSOCK)) == -1);
    con_err_sif (u_write(csd, "hello", strlen("hello")) == -1);
    (void) close(csd);
    
    return EXIT_SUCCESS;
err:
    U_CLOSE(csd);
    return EXIT_FAILURE;
}
