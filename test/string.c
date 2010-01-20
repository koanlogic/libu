#include <string.h>
#include <syslog.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <u/libu.h>

U_TEST_SUITE(string);

static int test_u_str(void)
{
    u_string_t *s = NULL;

    con_err_if(u_string_create("0", 1, &s));

    con_err_if(strcmp(u_string_c(s), "0"));

    con_err_if(u_string_sprintf(s, "%s", "1"));
    con_err_if(strcmp(u_string_c(s), "1"));

    con_err_if(u_string_aprintf(s, "%s", "23"));
    con_err_if(strcmp(u_string_c(s), "123"));

    con_err_if(u_string_cat(s, "45"));
    con_err_if(strcmp(u_string_c(s), "12345"));

    con_err_if(u_string_ncat(s, "6777", 2));
    con_err_if(strcmp(u_string_c(s), "1234567"));

    con_err_if(u_string_sprintf(s, "%s", "reset"));
    con_err_if(strcmp(u_string_c(s), "reset"));

    u_string_free(s);

    return 0;
err:
    return ~0;
}

U_TEST_SUITE( string )
{
    U_TEST_CASE_ADD( test_u_str );

    return 0;                                      
}
