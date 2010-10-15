#include <u/libu.h>

int test_suite_pwd_register (u_test_t *t);

static int test_u_pwd (u_test_case_t *tc);

static int test_u_pwd (u_test_case_t *tc)
{
    enum { 
        PWD_NUM = 1024,
        INT_SZ = 16
    };
    u_pwd_t *pwd = NULL; 
    char user[INT_SZ];
    char pass[INT_SZ];
    int i;

    u_test_err_if (u_pwd_init_file("passwd", NULL, 0, 1, &pwd));

    for (i = 0; i < PWD_NUM; i++)  {
        u_test_err_if (u_snprintf(user, INT_SZ, "user%d", i));
        u_test_err_if (u_snprintf(pass, INT_SZ, "pass%d", i));
        u_test_err_if (u_pwd_auth_user(pwd, user, pass));
    }

    u_pwd_term(pwd);

    return U_TEST_SUCCESS;
err:
    U_FREEF(pwd, u_pwd_term);

    return U_TEST_FAILURE;
}

int test_suite_pwd_register (u_test_t *t)
{
    u_test_suite_t *ts = NULL;

    con_err_if (u_test_suite_new("Password", &ts));

    con_err_if (u_test_case_register("Plain text auth", test_u_pwd, ts));

    return u_test_suite_add(ts, t);
err:
    u_test_suite_free(ts);
    return ~0;
}
