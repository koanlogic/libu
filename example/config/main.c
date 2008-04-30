#include <u/libu.h>

int facility = LOG_LOCAL0;

int main(int argc, char **argv)
{
    u_config_t *c = NULL;

    con_err_ifm(argc < 2, "usage: uconfig FILE");

    con_err_if(u_config_load_from_file(argv[1], &c));

    u_config_print(c, 0);

    return 0;
err:
    return 1;
}
