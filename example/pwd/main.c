#include <pwd.h>
#include <unistd.h>
#include <u/libu.h>

int facility = LOG_LOCAL0;

int main (int argc, char *argv[])
{
    int i, rc;
    u_pwd_t *pwd = NULL;
    char prompt[128];

    /* in memory db snapshot */
    con_err_if (u_pwd_init_file("./passwd", NULL, 0, 0, &pwd));

    for (i = 1; i < argc; i++)
    {
        u_snprintf(prompt, sizeof prompt, "%s: ", argv[i]);
        rc = u_pwd_auth_user(pwd, argv[i], getpass(prompt));
        con("auth %s", rc ? "failed" : "ok");
    }

    u_pwd_term(pwd);

    return EXIT_SUCCESS;
err:
    return EXIT_FAILURE;
}
