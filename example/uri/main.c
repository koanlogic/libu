#include <u/libu.h>

int facility = LOG_LOCAL0;

int main (int ac, char *av[])
{
    u_uri_t *u = NULL;

    con_err_ifm (ac != 2, "uuri <uri string>");

    con_err_ifm (u_uri_crumble(av[1], 0, &u), "URI parse error");
    u_uri_print(u, 0);
    u_uri_free(u);

    return 0;
err:
    if (u)
        u_uri_free(u);
    return 1;
}
