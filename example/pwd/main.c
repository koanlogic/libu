#include <pwd.h>
#include <unistd.h>
#include <u/libu.h>

int facility = LOG_LOCAL0;

static void file_rewind (void *res_handler);
static char *file_fgets(char *str, int size, void *res_handler);

int main (int argc, char *argv[])
{
    int i, rc;
    FILE *fp = NULL;
    u_pwd_t *pwd = NULL;
    char prompt[128];

    con_err_sif ((fp = fopen("./passwd", "r")) == NULL);
    con_err_if (u_pwd_init(fp, file_fgets, NULL, 0, &pwd));
    (void) u_pwd_reload_if_mod(pwd, file_rewind);

    for (i = 1; i < argc; i++)
    {
        u_snprintf(prompt, sizeof prompt, "%s: ", argv[i]);
        rc = u_pwd_auth_user(pwd, argv[i], getpass(prompt));
        con("auth %s", rc ? "failed" : "ok");
    }

    return EXIT_SUCCESS;
err:
    return EXIT_FAILURE;
}

static void file_rewind (void *res_handler)
{
    rewind((FILE *) res_handler);
    return;
}

static char *file_fgets(char *str, int size, void *res_handler)
{
    return fgets(str, size, (FILE *) res_handler); 
}
