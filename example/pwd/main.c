#include <pwd.h>
#include <unistd.h>
#include <u/libu.h>

int facility = LOG_LOCAL0;

int main (int argc, char *argv[])
{
    char c;
    int i, rc, in_memory = 0;
    u_pwd_t *pwd = NULL;
    char prompt[128];

    while ((c = getopt(argc, argv, "m")) != -1)
    {
        switch (c)
        {
            case 'm':
                ++in_memory;
                break;
            default:
                con_err("usage: pwd [-m] user ...");
        }
    }
    
    argc -= optind;
    argv += optind;

    con_err_if (u_pwd_init_file("./passwd", NULL, 0, in_memory, &pwd));

    for (i = 0; i < argc; i++)
    {
        u_snprintf(prompt, sizeof prompt, "%s: ", argv[i]);
        rc = u_pwd_auth_user(pwd, argv[i], getpass(prompt));
        u_con("auth %s", rc ? "failed" : "ok");
    }

    u_pwd_term(pwd);

    return EXIT_SUCCESS;
err:
    return EXIT_FAILURE;
}
