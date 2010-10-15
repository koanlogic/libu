#include <u/libu.h>

#define KEY_SZ  128

int test_suite_bst_register (u_test_t *t);

static u_bst_t *prepare_bst (u_test_case_t *tc, size_t nelems);
static void cmp_last_string (u_bst_node_t *node, void *dummy);

static int test_sort (u_test_case_t *tc);
static int test_search (u_test_case_t *tc);

static int test_sort (u_test_case_t *tc)
{
    enum { NELEMS = 1000000 };
    u_bst_t *bst = NULL;

    /* Prepare a BST with 'NELEMS' nodes. */
    u_test_err_if ((bst = prepare_bst(tc, NELEMS)) == NULL);

    u_test_case_printf(tc, "BST sorting %u elements", u_bst_count(bst));

    /* Check for monotonically increasing key values. */
    (void) u_bst_foreach(bst, cmp_last_string, tc);

    u_bst_free(bst);

    return U_TEST_SUCCESS;
err:
    if (bst)
        u_bst_free(bst);

    return U_TEST_FAILURE;
}

static int test_search (u_test_case_t *tc)
{
    enum { NELEMS = 1000000 };
    u_bst_t *bst = NULL;
    u_bst_node_t *node;

    /* Prepare a BST with 'NELEMS' nodes. */
    u_test_err_if ((bst = prepare_bst(tc, NELEMS)) == NULL);

    /* Push a needle into the haystack. */
    u_test_err_if (u_bst_push(bst, "needle", NULL));

    /* Search for it. */
    u_test_err_if ((node = u_bst_search(bst, "needle")) == NULL);

    u_test_case_printf(tc, "\'%s\' found !", 
            (const char *) u_bst_node_key(node));

    u_bst_free(bst);

    return U_TEST_SUCCESS;
err:
    if (bst)
        u_bst_free(bst);

    return U_TEST_FAILURE;
}

static u_bst_t *prepare_bst (u_test_case_t *tc, size_t nelems)
{
    size_t i;
    char key[KEY_SZ];
    u_bst_t *bst = NULL;

    /* Seed the PRNG. */
    srand((unsigned) getpid());

    u_test_err_if (u_bst_new(U_BST_OPT_NONE, &bst));
    
    /* Push 'nelem' random nodes with string keys. */
    for (i = 0; i < nelems; i++)
    {
        (void) u_snprintf(key, sizeof key, "%12.12d", rand());
        u_test_err_if (u_bst_push(bst, key, NULL));
    }

    return bst;
err:
    if (bst)
        u_bst_free(bst);
    return NULL;
}

static void cmp_last_string (u_bst_node_t *node, void *p)
{
    u_test_case_t *tc = (u_test_case_t *) p;
    const char *key = (const char *) u_bst_node_key(node);
    static char last[KEY_SZ] = { '\0' };

    /* Compare against last saved key. */
    if (strcmp(last, key) > 0)
    {
        /* FIXME: on failure should fire an abort() or set some global 
         *        to explicitly invalidate the test. */
        u_test_case_printf(tc, "SORT FAILED on key %s", key);
    }

    /* Save current node's key to 'last'. */
    (void) u_strlcpy(last, key, sizeof last);

    return ;
}

int test_suite_bst_register (u_test_t *t)
{
    u_test_suite_t *ts = NULL;

    con_err_if (u_test_suite_new("Binary Search Tree", &ts));

    con_err_if (u_test_case_register("Sort", test_sort, ts));
    con_err_if (u_test_case_register("Search", test_search, ts));

    return u_test_suite_add(ts, t);
err:
    u_test_suite_free(ts);
    return ~0;
}
