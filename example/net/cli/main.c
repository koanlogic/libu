/* $Id: main.c,v 1.7 2009/12/25 19:04:07 tho Exp $ */

#include <stdlib.h>
#include <u/libu.h>

int facility = LOG_LOCAL0;

int main (int argc, char *argv[])
{
    int i, csd = -1;
    char s[1024] = { 0 };

    con_err_ifm (argc < 2, "usage: %s <server_uri> [string...]", argv[0]);

    if (argc > 2)
    {
        for (i = 2; i < argc; ++i)
            con_err_ifm (u_strlcat(s, argv[i], sizeof s) || 
                    u_strlcat(s, " ", sizeof s), "string too long");
    }
    else
        (void) u_strlcpy(s, "test string", sizeof s);

    con_err_sif ((csd = u_net_sock(argv[1], U_NET_CSOCK)) == -1);
    con_err_sif (u_write(csd, s, strlen(s) + 1) == -1);
    (void) close(csd);
    
    return EXIT_SUCCESS;
err:
    U_CLOSE(csd);
    return EXIT_FAILURE;
}
