#include <u/libu.h>

int test_suite_lexer_register (u_test_t *t);

static int test_scan0 (u_test_case_t *tc);
static int test_scan1 (u_test_case_t *tc);
static int test_match (u_test_case_t *tc);

static int scan (u_test_case_t *tc, int (*f)(u_lexer_t *, char *));

/* 'f' is one of u_lexer_next() or u_lexer_skip() */
static int scan (u_test_case_t *tc, int (*f)(u_lexer_t *, char *))
{
    char c;
    int i = 0;
    u_lexer_t *l = NULL;
    const char *s = "abc  AB\tC\n1 2    3 ";
    const char *ex0 = "abcABC123", *ex1 = s;
    char dest[1024] = { '\0' };

    u_test_err_if (u_lexer_new(s, &l));

    /* First char under cursor. */
    c = u_lexer_peek(l);
    dest[i] = c;

    while (f(l, &c) != -1)
        dest[++i] = c;

    dest[++i] = '\0';
    
    if (f == u_lexer_next)
        u_test_err_if (strcmp(dest, ex1));
    else if (f == u_lexer_skip)
        u_test_err_if (strcmp(dest, ex0));
    else
        u_test_err_ifm (1, "uh?");

    u_lexer_free(l);
    return U_TEST_SUCCESS;
err:
    u_lexer_free(l);
    return U_TEST_FAILURE;
}

static int test_scan0 (u_test_case_t *tc)
{
    return scan(tc, u_lexer_next);
}

static int test_scan1 (u_test_case_t *tc)
{
    return scan(tc, u_lexer_skip);
}


static int test_match (u_test_case_t *tc)
{
    char c;
    u_lexer_t *l = NULL;
    char match[U_TOKEN_SZ];
#define EXP "*match me*"
    const char *s = "abc " EXP " ABC";

    u_test_err_if (u_lexer_new(s, &l));

    /* get left-hand bookmark, i.e. first '*' */
    while (u_lexer_next(l, &c) != -1)
    {
        if (c == '*')
        {
            u_lexer_record_lmatch(l);
            break;
        }
    }

    /* now go for the right-hand bookmark, i.e. second '*' */
    while (u_lexer_next(l, &c) != -1)
    {
        if (c == '*')
        {
            u_lexer_record_rmatch(l);
            break;
        }
    }
    
    u_test_err_if (strcmp(u_lexer_get_match(l, match), EXP));
#undef EXP
    u_lexer_free(l);
    return U_TEST_SUCCESS;
err:
    u_lexer_free(l);
    return U_TEST_FAILURE;
}

int test_suite_lexer_register (u_test_t *t)
{
    u_test_suite_t *ts = NULL;

    con_err_if (u_test_suite_new("Lexer", &ts));

    con_err_if (u_test_case_register("scan (no skip ws)", test_scan0, ts));
    con_err_if (u_test_case_register("scan (skip ws)", test_scan1, ts));
    con_err_if (u_test_case_register("match", test_match, ts));

    return u_test_suite_add(ts, t);
err:
    u_test_suite_free(ts);
    return ~0;
}
