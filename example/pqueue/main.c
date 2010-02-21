#include <unistd.h>
#include <stdlib.h>
#include <u/libu.h>
#include "pqueue.h"

int facility = LOG_LOCAL0;

static int sort (void);
static int stack (void);

int main (void)
{
    con_err_if (sort());
    con_err_if (stack());

    return EXIT_SUCCESS;
err:
    return EXIT_FAILURE;
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
        //u_con("key: %lf", key);
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
