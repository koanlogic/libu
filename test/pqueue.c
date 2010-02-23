#include <u/libu.h>
#include <float.h>

int test_suite_pqueue_register (u_test_t *t);

static int test_top10 (u_test_case_t *tc);
static int test_heapsort (u_test_case_t *tc);

static int test_top10 (u_test_case_t *tc)
{
    size_t i;
    double key, keymax = DBL_MAX;
    u_pq_t *pq = NULL;

    srandom((unsigned long) getpid());

    con_err_if (u_pq_create(10, &pq));

    for (i = 0; i < 10000000; i++)
    {
        if (!u_pq_empty(pq))
            (void) u_pq_peekmax(pq, &keymax);

        if ((key = (double) random()) > keymax)
            continue;

        if (u_pq_full(pq))
            (void) u_pq_delmax(pq, NULL);

        con_err_if (u_pq_push(pq, key, NULL));
    }

    for (i = 0; i < 10; i++)
    {
        (void) u_pq_delmax(pq, &key);
        u_test_case_printf(tc, "%zu: %lf", i, key);
    }

    return U_TEST_SUCCESS;
err:
    u_pq_free(pq);
    return U_TEST_FAILURE;
}

static int test_heapsort (u_test_case_t *tc)
{
    size_t i;
    double key, prev_key = -1;
    u_pq_t *pq = NULL;

    srandom((unsigned long) getpid());

    con_err_if (u_pq_create(1000000, &pq));

    for (i = 0; i < 999999; i++)
        con_err_if (u_pq_push(pq, (double) random(), NULL));

    while (!u_pq_empty(pq))
    {
        (void) u_pq_delmax(pq, &key);
        con_err_if (prev_key != -1 && key < prev_key);
        prev_key = key;
    }

    u_pq_free(pq);

    return U_TEST_SUCCESS;
err:
    u_pq_free(pq);
    return U_TEST_FAILURE;
}

int test_suite_pqueue_register (u_test_t *t)
{
    u_test_suite_t *ts = NULL;

    con_err_if (u_test_suite_new("Priority Queues", &ts));

    con_err_if (u_test_case_register("Top 10 in 10 million", test_top10, ts));
    con_err_if (u_test_case_register("Heap sort 1 million", test_heapsort, ts));

    return u_test_suite_add(ts, t);
err:
    u_test_suite_free(ts);
    return ~0;
}
