#include <unistd.h>
#include <stdlib.h>
#include <float.h>
#include <u/libu.h>
#include "pqueue.h"

int facility = LOG_LOCAL0;

static int sort (void);
static int stack (void);
static int top10 (void);

int main (void)
{
    con_err_if (top10());
    con_err_if (sort());
    con_err_if (stack());

    return EXIT_SUCCESS;
err:
    return EXIT_FAILURE;
}

static int top10 (void)
{
    size_t i;
    double key, keymax = DBL_MAX;
    pq_t *pq = NULL;

    srandom((unsigned long) getpid());

    con_err_if (pq_create(10, &pq));

    for (i = 0; i < 1000000; i++)
    {
        if (!pq_empty(pq))
            (void) pq_peekmax(pq, &keymax);

        if ((key = (double) random()) > keymax)
            continue;

        if (pq_full(pq))
            (void) pq_delmax(pq, NULL);

        con_err_if (pq_push(pq, key, NULL));
    }

    for (i = 0; i < 10; i++)
    {
        (void) pq_delmax(pq, &key);
        u_con("%zu: %lf", i, key);
    }

    return 0;
err:
    pq_free(pq);
    return ~0;
}

static int sort (void)
{
    size_t i;
    double key, prev_key = -1;
    pq_t *pq = NULL;

    srandom((unsigned long) getpid());

    con_err_if (pq_create(1000, &pq));

    for (i = 0; i < 999; i++)
        con_err_if (pq_push(pq, (double) random(), NULL));

    while (!pq_empty(pq))
    {
        (void) pq_delmax(pq, &key);
        con_err_if (prev_key != -1 && key < prev_key);
        prev_key = key;
    }

    pq_free(pq);

    return 0;
err:
    pq_free(pq);
    return ~0;
}

static int stack (void)
{
    double key, key2;
    pq_t *pq = NULL;

    con_err_if (pq_create(10, &pq));

    for (key = 0; key < 1000; key++)
    {
        con_err_if (pq_push(pq, key, NULL));
        (void) pq_delmax(pq, &key2);
        con_err_if (key != key2);
    }

    pq_free(pq);

    return 0;
err:
    pq_free(pq);
    return ~0;
}
