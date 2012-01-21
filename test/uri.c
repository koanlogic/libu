#include <u/libu.h>

int test_suite_uri_register (u_test_t *t);

static int test_uri_parser (u_test_case_t *tc);
static int test_uri_builder (u_test_case_t *tc);

/* shall match struct u_uri_s */
typedef struct 
{
    unsigned int flags;
    const char *scheme, *user, *pwd, *host, *port, *path, *query, *fragment;
} u_uri_atoms_t;

static int test_uri_parser (u_test_case_t *tc)
{
    struct vt_s
    {
        const char *in;
        u_uri_atoms_t ex;
    } vt[] = {
        { 
            "tcp4://www.kame.net:http/index.html",
            {
                .flags = U_URI_FLAGS_NONE,
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
                .flags = U_URI_FLAGS_NONE,
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
                .flags = U_URI_FLAGS_HOST_IS_IPADDRESS | 
                         U_URI_FLAGS_HOST_IS_IPLITERAL,
                .scheme = "http",
                .user = NULL,
                .pwd = NULL,
                .host = "2001:200::8002:203:47ff:fea5:3085",
                .port = "80",
                .path = "/index.html"
            }
        },
        { 
            "coap://[::1]/.well-known/core",
            {
                .flags = U_URI_FLAGS_HOST_IS_IPADDRESS |
                         U_URI_FLAGS_HOST_IS_IPLITERAL,
                .scheme = "coap",
                .user = NULL,
                .pwd = NULL,
                .host = "::1",
                .port = NULL,
                .path = "/.well-known/core"
            }
        },
        { 
            "coaps://[::1]",
            {
                .flags = U_URI_FLAGS_HOST_IS_IPADDRESS |
                         U_URI_FLAGS_HOST_IS_IPLITERAL,
                .scheme = "coaps",
                .user = NULL,
                .pwd = NULL,
                .host = "::1",
                .port = NULL,
                .path = NULL
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
    if ((vt[i].ex.field == NULL) ?                                             \
            (strlen(u_uri_get_##field(u)) > 0) :                               \
            (strcmp(u_uri_get_##field(u), vt[i].ex.field) != 0))               \
    {                                                                          \
        u_test_case_printf(tc, "%s != %s at idx %d",                           \
                u_uri_get_##field(u), vt[i].ex.field, i);                      \
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
        u_test_err_if (u_uri_get_flags(u) != vt[i].ex.flags);

        u_uri_free(u), u = NULL;
    }

#undef CHECK_EXP_MSG

    return U_TEST_SUCCESS;
err:
    u_uri_free(u);
    return U_TEST_FAILURE;
}

static int test_uri_builder (u_test_case_t *tc)
{
    struct vt_s
    {
        u_uri_atoms_t in;
        const char *ex;
    } vt[] = {
        { 
            .in = {
                .scheme = "tcp4",
                .user = NULL,
                .pwd = NULL,
                .host = "www.kame.net",
                .port = "http",
                .path = "/index.html",
                .fragment = "overview"
            },
            .ex = "tcp4://www.kame.net:http/index.html#overview"
        },
        { 
            .in = {
                .scheme = "coap",
                .user = NULL,
                .pwd = NULL,
                .host = "::1",
                .port = NULL,
                .path = "/.well-known/core"
            },
            .ex = "coap://[::1]/.well-known/core"
        },
        { .ex = NULL }
    };

    int i;
    u_uri_t *u = NULL;

#define SET_URI_ATOM(field) do {    \
    if (vt[i].in.field != NULL)                                 \
        u_test_err_if (u_uri_set_##field(u, vt[i].in.field));   \
} while (0)

    for (i = 0; vt[i].ex; i++)
    {
        char s[U_URI_STRMAX];

        u_test_err_if (u_uri_new(0, &u));

        SET_URI_ATOM(scheme);
        SET_URI_ATOM(user);
        SET_URI_ATOM(pwd);
        SET_URI_ATOM(host);
        SET_URI_ATOM(port);
        SET_URI_ATOM(path);
        SET_URI_ATOM(query);
        SET_URI_ATOM(fragment);

        u_test_err_if (u_uri_knead(u, s));
        u_test_err_ifm (strcasecmp(s, vt[i].ex), "%s != %s", s, vt[i].ex);

        u_uri_free(u), u = NULL;
    }

#undef SET_URI_ATOM

    return U_TEST_SUCCESS;
err:
    if (u)    
        u_uri_free(u);
    return U_TEST_FAILURE;
}

int test_suite_uri_register (u_test_t *t)
{
    u_test_suite_t *ts = NULL;

    con_err_if (u_test_suite_new("URI", &ts));

    con_err_if (u_test_case_register("u_uri_crumble", test_uri_parser, ts));
    con_err_if (u_test_case_register("u_uri_knead", test_uri_builder, ts));

    /* uri depends on the lexer module */
    con_err_if (u_test_suite_dep_register("Lexer", ts));

    return u_test_suite_add(ts, t);
err:
    u_test_suite_free(ts);
    return ~0;
}
