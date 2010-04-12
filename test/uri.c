#include <u/libu.h>

int test_suite_uri_register (u_test_t *t);

static int test_uri_parser (u_test_case_t *tc);

/* shall match struct u_uri_s */
typedef struct 
{
    const char *scheme, *user, *pwd, *host, *port, *path, *query;
} u_uri_exp_t;

static int test_uri_parser (u_test_case_t *tc)
{
    struct vt_s
    {
        const char *in;
        u_uri_exp_t exp;
    } vt[] = {
        { 
            "tcp4://www.kame.net:http/index.html",
            {
                .scheme = "tcp4",
                .user = NULL,
                .pwd = NULL,
                .host = "www.kame.net",
                .port = "http",
                .path = "/index.html"
            }
        },
        { 
            "http://wiki.koanlogic.com/doku.php?id=libu",
            {
                .scheme = "http",
                .user = NULL,
                .pwd = NULL,
                .host = "wiki.koanlogic.com",
                .port = NULL,
                .path = "/doku.php",
                .query = "id=libu"
            }
        },
        { 
            "http://[2001:200::8002:203:47ff:fea5:3085]:80/index.html",
            {
                .scheme = "http",
                .user = NULL,
                .pwd = NULL,
                .host = "2001:200::8002:203:47ff:fea5:3085",
                .port = "80",
                .path = "/index.html"
            }
        },
        { 
            NULL,
            {
                .scheme = NULL,
                .user = NULL,
                .pwd = NULL,
                .host = NULL,
                .port = NULL,
                .path = NULL
            } 
        }
    };

    int i;
    u_uri_t *u = NULL;

#define CHECK_EXP_MSG(field) do {   \
    if (((vt[i].exp.field == NULL) ?                                           \
            strlen(u_uri_get_##field(u)) :                                     \
            strcmp(u_uri_get_##field(u), vt[i].exp.field)))                    \
    {                                                                          \
        u_test_case_printf(tc, "%s != %s at idx %d",                           \
                u_uri_get_##field(u), vt[i].exp.field, i);                     \
        goto err;                                                              \
    }                                                                          \
} while (0)

    for (i = 0; vt[i].in; i++)
    {
        u_test_err_if (u_uri_crumble(vt[i].in, U_URI_OPT_NONE, &u));

        CHECK_EXP_MSG(scheme);
        CHECK_EXP_MSG(user);
        CHECK_EXP_MSG(pwd);
        CHECK_EXP_MSG(host);
        CHECK_EXP_MSG(port);
        CHECK_EXP_MSG(path);

        u_uri_free(u), u = NULL;
    }

    return U_TEST_SUCCESS;
err:
    u_uri_free(u);
    return U_TEST_FAILURE;
}

int test_suite_uri_register (u_test_t *t)
{
    u_test_suite_t *ts = NULL;

    con_err_if (u_test_suite_new("URI", &ts));

    con_err_if (u_test_case_register("u_uri_crumble", test_uri_parser, ts));

    return u_test_suite_add(ts, t);
err:
    u_test_suite_free(ts);
    return ~0;
}
