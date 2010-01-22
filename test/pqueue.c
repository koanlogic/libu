#include <stdlib.h>
#include <u/libu.h>

U_TEST_SUITE(pqueue);

/* push 'mode's */
enum { FIX_SEQ = 0, RND_SEQ = 1 };

static int push (int keeplowest, int pqueue_len, int push_count, int mode);
static int test_bounds (int which);
static int double_cmp (const void *d0, const void *d1);

/* test cases */
static int test1_desc (void);
static int test1_asc (void);
static int test_highest (void);
static int test_lowest (void);

static int double_cmp (const void *d0, const void *d1)
{
    const double *a = (const double *) d0;
    const double *b = (const double *) d1;

    return *a - *b;
}

static int push (int keeplowest, int pqueue_len, int push_count, int mode)
{
    enum { SCORES_LEN = 10240 };
    u_pq_t *q = NULL;
    double sc, scores[SCORES_LEN];
    int i, t, nexpected; /* expected results count */
    unsigned int seed = (mode == RND_SEQ) ? time(NULL) : 12345;

    u_test_err_if (push_count >= SCORES_LEN);
    u_test_err_if (u_pq_create(pqueue_len, keeplowest, &q));

    srand(seed);

    for (i = 0; i < push_count; ++i)
    {
        double score = (mode == RND_SEQ) ? (rand() % 999) : (- rand() % 999);

        if ((mode == RND_SEQ) && (rand() % 2) != 0)
            score = -score;

        scores[i] = score;

        (void) u_pq_push(q, scores[i], &scores[i]);
    }

    qsort(scores, push_count, sizeof(double), double_cmp);

    i = 0;
    nexpected = U_MIN(pqueue_len, push_count);

    u_pq_foreach (q, t)
    {
        int x = (keeplowest ? i : push_count - 1 - i);
        nexpected--;
        sc = scores[x];
        u_test_err_if (u_pq_prio(q, t) != sc);
        i++;
    }

    u_test_err_if (nexpected != 0);

    u_pq_free(q);

    return 0;
err:
    if(q)
        u_pq_free(q);

    return ~0;
}

/* 'which' is one of U_PQ_KEEP_LOWEST | U_PQ_KEEP_HIGHEST */
static int test_bounds (int which)
{
    enum { PQUEUE_LEN_MAX = 128 };
    enum { MAX_PUSHES = 512 };
    int pqueue_len, t;

    for (pqueue_len = 2; pqueue_len < PQUEUE_LEN_MAX; ++pqueue_len)
    {
        for (t = 2; t < MAX_PUSHES; ++t)
            u_test_err_if (push(which, pqueue_len, t, RND_SEQ));
    }

    return U_TEST_EXIT_SUCCESS;
err:
    return U_TEST_EXIT_FAILURE;
}

static int test_lowest (void)
{
    return test_bounds(U_PQ_KEEP_LOWEST);
}

static int test_highest (void)
{
    return test_bounds(U_PQ_KEEP_HIGHEST);
}

static int test1_asc (void)
{
    u_test_err_if (push(1, 3, 2, FIX_SEQ));
    u_test_err_if (push(1, 2, 3, FIX_SEQ));
    u_test_err_if (push(1, 2, 4, FIX_SEQ));

    return U_TEST_EXIT_SUCCESS;
err:
    return U_TEST_EXIT_FAILURE;
}

static int test1_desc (void)
{
    u_test_err_if (push(0, 3, 2, FIX_SEQ));
    u_test_err_if (push(0, 2, 3, FIX_SEQ));
    u_test_err_if (push(0, 2, 5, FIX_SEQ));
    u_test_err_if (push(0, 2, 4, FIX_SEQ));

    return U_TEST_EXIT_SUCCESS;
err:
    return U_TEST_EXIT_FAILURE;
}

static int test2_asc (void)
{
    u_test_err_if (push(1, 3, 2, RND_SEQ));
    u_test_err_if (push(1, 2, 3, RND_SEQ));

    return U_TEST_EXIT_SUCCESS;
err:
    return U_TEST_EXIT_FAILURE;
}

static int test2_desc (void)
{
    u_test_err_if (push(0, 3, 2, RND_SEQ));
    u_test_err_if (push(0, 2, 3, RND_SEQ));

    return U_TEST_EXIT_SUCCESS;
err:
    return U_TEST_EXIT_FAILURE;
}

U_TEST_MODULE( pqueue )
{
    U_TEST_CASE_ADD( test1_asc );
    U_TEST_CASE_ADD( test1_desc );
    U_TEST_CASE_ADD( test2_asc );
    U_TEST_CASE_ADD( test2_desc );
    U_TEST_CASE_ADD( test_highest );
    U_TEST_CASE_ADD( test_lowest );

    return 0;
}
