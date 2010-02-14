#include <u/libu.h>

U_TEST_SUITE(pwd);

static int test_u_pwd(void)
{
    enum { 
        PWD_NUM = 1024,
        INT_SZ = 16
    };
    u_pwd_t *pwd = NULL; 
    char user[INT_SZ];
    char pass[INT_SZ];
    int i;

    con_err_if (u_pwd_init_file("passwd", NULL, 0, 1, &pwd));

    for (i = 0; i < PWD_NUM; i++)  {
        con_err_if (u_snprintf(user, INT_SZ, "user%d", i));
        con_err_if (u_snprintf(pass, INT_SZ, "pass%d", i));
        con_err_if (u_pwd_auth_user(pwd, user, pass));
    }

    u_pwd_term(pwd);

    return U_TEST_EXIT_SUCCESS;
err:

    U_FREEF(pwd, u_pwd_term);

    return U_TEST_EXIT_FAILURE;
}

U_TEST_SUITE( pwd )
{
    U_TEST_CASE_ADD( test_u_pwd );

    return 0;                                      
}
