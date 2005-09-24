/* $Id: main.c,v 1.1 2005/09/24 18:35:13 tho Exp $ */

#include <stdlib.h>
#include <err.h>
#include <u/net.h>
#include <u/debug.h>
#include <u/misc.h>

int main (int argc, char *argv[])
{
    int csd = -1;

    if (argc != 2)
        errx(1, "usage: cli <server_uri>");

    dbg_err_if ((csd = u_net_sock(argv[1], U_NET_CSOCK)) == -1);
    dbg_err_if (u_net_write(csd, "hello", strlen("hello") + 1, NULL));
    U_CLOSE(csd);
    
    return EXIT_SUCCESS;
err:
    dbg_strerror(errno);
    U_CLOSE(csd);
    return EXIT_FAILURE;
}
