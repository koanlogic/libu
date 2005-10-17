/* $Id: main.c,v 1.3 2005/10/17 18:21:59 tat Exp $ */

#include <stdlib.h>
#include <err.h>
#include <u/net.h>
#include <u/carpal.h>
#include <u/misc.h>

int main (int argc, char *argv[])
{
    int csd = -1;

    if (argc != 2)
        errx(1, "usage: cli <server_uri>");

    dbg_err_if ((csd = u_net_sock(argv[1], U_NET_CSOCK)) == -1);
    dbg_err_if (u_net_writen(csd, "hello", strlen("hello") + 1));
    U_CLOSE(csd);
    
    return EXIT_SUCCESS;
err:
    dbg_strerror(errno);
    U_CLOSE(csd);
    return EXIT_FAILURE;
}
