#include <stddef.h>
#include <u/libu.h>
#include "test.h"

#define TEST_ID_MAX 128

typedef struct 
{ 
    unsigned int currank;
    TAILQ_HEAD(, test_obj_s) objs;
} TO;

typedef TAILQ_HEAD(, test_dep_s) TD;

/* test suite or case dependency */
struct test_dep_s
{
    char id[TEST_ID_MAX];           /* tag of the declared dependency */
    TAILQ_ENTRY(test_dep_s) links;  /* sibling deps */
};

typedef struct test_dep_s test_dep_t;

/* generic test data container */
struct test_obj_s
{
    TO *parent;                     /* ... */
    _Bool sequenced;                /* true if sequenced */
    unsigned int rank;              /* scheduling rank */
    char id[TEST_ID_MAX];           /* MUST be unique */
    TAILQ_ENTRY(test_obj_s) links;  /* sibling test cases/suites */
    TD deps;                        /* test cases/suites we depend on */
};

typedef struct test_obj_s test_obj_t;

/* a test case */
struct test_case_s
{
    int (*func) (struct test_case_s *);
    test_obj_t o;
};

/* a test suite */
struct test_suite_s
{
    TO test_cases;  /* head of tc list */
    test_obj_t o;
};

struct test_s
{
    unsigned int currank;   /* current TS rank when sequencing */
    char id[TEST_ID_MAX];   /* test id */
    TO test_suites;         /* head of TS list */
};

#define TS_HANDLE(optr)    \
    (test_suite_t *) ((char *) optr - offsetof(test_suite_t, o))

#define TC_HANDLE(optr) \
    (test_case_t *) ((char *) optr - offsetof(test_case_t, o))

static int test_obj_add (test_obj_t *to, TO *parent);
static int test_obj_evict_id (TO *h, const char *id);
static int test_obj_init (const char *id, test_obj_t *to);
static test_obj_t *test_obj_pick_top (TO *h);
static test_obj_t *test_obj_search (TO *h, const char *id);
static void test_obj_free (test_obj_t *to);
static void test_obj_print (char nindent, test_what_t what, test_obj_t *to);
static int test_obj_dep_add (test_dep_t *td, test_obj_t *to, test_what_t what);
static int test_obj_sequencer (TO *h, test_what_t what);

static void test_dep_free (test_dep_t *td);
static int test_dep_new (const char *id, test_dep_t **ptd);
static void test_dep_print (char nindent, test_dep_t *td);
static test_dep_t *test_dep_search (TD *h, const char *id);

static void test_case_print (test_case_t *tc);
static int test_case_dep_add (test_dep_t *td, test_case_t *tc);

static void test_suite_print (test_suite_t *ts);
static int test_suite_dep_add (test_dep_t *td, test_suite_t *ts);

static int test_sequencer (test_t *t);
static void test_print (test_t *t);

int test_run (test_t *t)
{
    u_con("** before sequencing");
    test_print(t);
    con_err_if (test_sequencer(t));
    u_con("\n** after sequencing");
    test_print(t);

    return 0;
err:
    return ~0;
}

int test_suite_dep_register (const char *id, test_suite_t *ts)
{
    test_dep_t *td = NULL;

    dbg_err_if (test_dep_new(id, &td));
    dbg_err_if (test_suite_dep_add(td, ts));

    return 0;
err:
    test_dep_free(td);
    return ~0;
}

int test_case_dep_register (const char *id, test_case_t *tc)
{
    test_dep_t *td = NULL;

    dbg_err_if (test_dep_new(id, &td));
    dbg_err_if (test_case_dep_add(td, tc));

    return 0;
err:
    test_dep_free(td);
    return ~0;
}

int test_case_register (const char *id, test_f func, test_suite_t *ts)
{
    test_case_t *tc = NULL;

    dbg_err_if (test_case_new(id, func, &tc));
    dbg_err_if (test_case_add(tc, ts));

    return 0;
err:
    test_case_free(tc);
    return ~0;
}

int test_suite_new (const char *id, test_suite_t **pts)
{
    test_suite_t *ts = NULL;

    dbg_return_if (id == NULL, ~0);
    dbg_return_if (pts == NULL, ~0);

    dbg_err_sif ((ts = u_malloc(sizeof *ts)) == NULL);
    TAILQ_INIT(&ts->test_cases.objs);
    ts->test_cases.currank = 0;

    dbg_err_if (test_obj_init(id, &ts->o));

    *pts = ts;

    return 0;
err:
    test_suite_free(ts);
    return ~0;
}

void test_suite_free (test_suite_t *ts)
{
    test_case_t *tc;
    test_obj_t *to, *nto;

    if (ts == NULL)
        return;

    /* Free deps (should be empty in a sequenced suite). */
    test_obj_free(&ts->o);

    /* Free test cases. */
    TAILQ_FOREACH_SAFE (to, &ts->test_cases.objs, links, nto)
    {
        tc = TC_HANDLE(to);
        test_case_free(tc);
    }

    u_free(ts);

    return;
}

void test_free (test_t *t)
{
    test_suite_t *ts;
    test_obj_t *to, *nto;

    if (t == NULL)
        return;

    /* Free all children test suites. */
    TAILQ_FOREACH_SAFE (to, &t->test_suites.objs, links, nto)
    {
        ts = TS_HANDLE(to);
        test_suite_free(ts);
    } 

    /* Free the container. */
    u_free(t);

    return;
}

int test_new (const char *id, test_t **pt)
{
    test_t *t = NULL;

    dbg_return_if (pt == NULL, ~0);

    dbg_err_sif ((t = u_malloc(sizeof *t)) == NULL);

    TAILQ_INIT(&t->test_suites.objs);
    t->test_suites.currank = 0;
    dbg_err_if (u_strlcpy(t->id, id, sizeof t->id));
    t->currank = 0;

    *pt = t;

    return 0;
err:
    u_free(t);
    return ~0;
}

void test_case_free (test_case_t *tc)
{
    if (tc == NULL)
        return;

    /* Free the attached dependencies (empty in a sequenced suite). */
    test_obj_free(&tc->o);

    /* Free the container. */
    u_free(tc);

    return;
}

int test_case_new (const char *id, test_f func, test_case_t **ptc)
{
    test_case_t *tc = NULL;

    dbg_return_if (id == NULL, ~0);
    dbg_return_if (ptc == NULL, ~0);

    dbg_err_sif ((tc = u_malloc(sizeof *tc)) == NULL);
    dbg_err_if (test_obj_init(id, &tc->o));

    tc->func = func;

    *ptc = tc;

    return 0;
err:
    test_case_free(tc);
    return ~0;
}

int test_case_add (test_case_t *tc, test_suite_t *ts)
{
    dbg_return_if (tc == NULL, ~0);
    dbg_return_if (ts == NULL, ~0);

    TAILQ_INSERT_HEAD(&ts->test_cases.objs, &tc->o, links);
    tc->o.parent = &ts->test_cases;

    return 0;
}

int test_suite_add (test_suite_t *ts, test_t *t)
{
    dbg_return_if (t == NULL, ~0);
    dbg_return_if (ts == NULL, ~0);

    TAILQ_INSERT_HEAD(&t->test_suites.objs, &ts->o, links);
    ts->o.parent = &t->test_suites;

    return 0;
}

void test_print (test_t *t)
{
    test_obj_t *to;

    dbg_return_if (t == NULL, );

    u_con("[test] %s", t->id);  

    TAILQ_FOREACH (to, &t->test_suites.objs, links)
        test_suite_print(TS_HANDLE(to));

    return;
}

static void test_suite_print (test_suite_t *ts)
{
    test_dep_t *td;
    test_obj_t *to;

    dbg_return_if (ts == NULL, );

    test_obj_print(4, TEST_SUITE_T, &ts->o);

    TAILQ_FOREACH (td, &ts->o.deps, links)
        test_dep_print(4, td);

    TAILQ_FOREACH (to, &ts->test_cases.objs, links)
        test_case_print(TC_HANDLE(to));

    return;
}

static void test_case_print (test_case_t *tc)
{
    test_dep_t *td;

    dbg_return_if (tc == NULL, );

    test_obj_print(8, TEST_CASE_T, &tc->o);

    TAILQ_FOREACH (td, &tc->o.deps, links)
        test_dep_print(8, td);

    return;
}

static void test_obj_print (char nindent, test_what_t what, test_obj_t *to)
{
    dbg_return_if (to == NULL, );

    u_con("%*c=> [%s] %s", 
            nindent, ' ', what == TEST_CASE_T ? "case" : "suite", to->id);

    /* attributes */
    u_con("%*c    .rank = %u", nindent, ' ', to->rank);  
    u_con("%*c    .seq = %s", nindent, ' ', to->sequenced ? "true" : "false");

    return;
}

static void test_dep_print (char nindent, test_dep_t *td)
{
    u_con("%*c    .<dep> = %s", nindent, ' ', td->id);
    return;
}

static int test_dep_new (const char *id, test_dep_t **ptd)
{
    test_dep_t *td = NULL;

    dbg_return_if (id == NULL, ~0);
    dbg_return_if (ptd == NULL, ~0);

    dbg_err_sif ((td = u_malloc(sizeof *td)) == NULL);
    dbg_err_if (u_strlcpy(td->id, id, sizeof td->id));

    *ptd = td;

    return 0;
err:
    test_dep_free(td);
    return ~0;
}

static void test_dep_free (test_dep_t *td)
{
    if (td)
        u_free(td);

    return;
}

static test_dep_t *test_dep_search (TD *h, const char *id)
{
    test_dep_t *td;

    dbg_return_if (h == NULL, NULL);
    dbg_return_if (id == NULL, NULL);

    TAILQ_FOREACH (td, h, links)
    {
        if (!strcmp(td->id, id))
            return td;
    }

    return NULL;
}

static test_obj_t *test_obj_search (TO *h, const char *id)
{
    test_obj_t *to;

    dbg_return_if (h == NULL, NULL);
    dbg_return_if (id == NULL, NULL);

    TAILQ_FOREACH (to, &h->objs, links)
    {
        if (!strcmp(to->id, id))
            return to;
    }

    return NULL;
}

static int test_obj_add (test_obj_t *to, TO *parent)
{
    dbg_return_if (to == NULL, ~0);
    dbg_return_if (parent == NULL, ~0);

    TAILQ_INSERT_HEAD(&parent->objs, to, links);
    to->parent = parent;

    return 0;
}

static int test_suite_dep_add (test_dep_t *td, test_suite_t *ts)
{
    return test_obj_dep_add(td, &ts->o, TEST_SUITE_T);
}

static int test_case_dep_add (test_dep_t *td, test_case_t *ts)
{
    return test_obj_dep_add(td, &ts->o, TEST_CASE_T);
}

/* It MUST be called AFTER the test case/suite on which it is established 
 * has been added. */
static int test_obj_dep_add (test_dep_t *td, test_obj_t *to, test_what_t what)
{
#if IMPLICIT_OBJ
    test_obj_t *nto = NULL;
    test_case_t *tc = NULL;
    test_suite_t *ts = NULL;
#endif
    
    dbg_return_if (to == NULL, ~0);
    dbg_return_if (td == NULL, ~0);

#if IMPLICIT_OBJ
    /* See if we have already added this test case/suite. 
     * In case it were not found, create it implicitly and stick it to the 
     * test cases/suites (depending on 'what') list. */
    if (test_obj_search(to->parent, to->id) == NULL)
    {
        switch (what) 
        {
            case TEST_CASE_T:
                dbg_err_sif (test_case_new(td->id, &tc));
                nto = &tc->o;
                break;
            case TEST_SUITE_T:
                dbg_err_sif (test_suite_new(td->id, &ts));
                nto = &ts->o;
                break;
        }

        dbg_err_if (test_obj_add(nto, to->parent));
        ts = NULL, tc = NULL;   /* ownership lost */
    }
#endif

    /* See if the dependency is not already been recorded, in which case
     * insert it into the dependency list for this test case/suite. */
    if (test_dep_search(&to->deps, to->id) == NULL)
    {
        TAILQ_INSERT_HEAD(&to->deps, td, links);
    }
    else
        u_free(td);

    return 0;
#if IMPLICIT_OBJ
err:
    if (ts)
        test_suite_free(ts);
    if (tc)
        test_case_free(tc);
    return ~0;
#endif
}

static void test_obj_free (test_obj_t *to)
{
    test_dep_t *td, *ntd;

    /* Just free the attached deps list: expect 'to' being a pointer to a
     * test object in a bigger malloc'd chunk (suite or case). */
    TAILQ_FOREACH_SAFE (td, &to->deps, links, ntd)
        test_dep_free(td); 

    return;
}

static test_obj_t *test_obj_pick_top (TO *h)
{
    /* Top element is a (not already sequenced) test object with no deps. */
    test_obj_t *to;

    TAILQ_FOREACH (to, &h->objs, links)
    {
        if (to->sequenced == false && TAILQ_EMPTY(&to->deps))
        {
            /* Record the reached depth: it will be used by the
             * next evicted element. */
            to->parent->currank = to->rank;
            return to;
        }
    }

    return NULL;
}

/* Do a (variation on) topological sorting over the partially ordered sets
 * of test suites/cases.  We are primarily concerned in equally ranking all
 * test objects which are at the same "dependency-depth" so that the scheduler
 * can exec tests in the same rank-pool in parallel.  We are also interested 
 * in creating a a partial order for cases where the scheduler is forced to 
 * run sequentially (i.e. no system fork(2) or by explicit user request) [TODO].
 * NOTE that higher ranks mean lower priority */
static int test_obj_sequencer (TO *h, test_what_t what)
{
    test_obj_t *to;
    test_suite_t *ts;

    dbg_return_if (h == NULL, ~0);

    /* sequence test cases/suites */
    for (;;)
    {
        if ((to = test_obj_pick_top(h)) == NULL)
           break;

        dbg_err_if (test_obj_evict_id(h, to->id));
    }

    /* Test if we got out because of a cycle in deps, i.e. check if we 
     * still have any non-sequenced test case/suite. */
    TAILQ_FOREACH (to, &h->objs, links)
    {
        dbg_err_ifm (to->sequenced == false, 
                "%s not sequenced: dependency loop !", to->id);
    }

    /* A test suite need to recur into its test cases. */
    if (what == TEST_SUITE_T)
    {
        TAILQ_FOREACH (to, &h->objs, links)
        {
            ts = TS_HANDLE(to);
            dbg_err_if (test_obj_sequencer(&ts->test_cases, TEST_CASE_T)); 
        }
    }

    return 0;
err:
    return ~0;
}

static int test_obj_evict_id (TO *h, const char *id)
{
    test_obj_t *to;
    test_dep_t *td;

    dbg_return_if (id == NULL, ~0); 

    /* Delete from the deps list of every test suite, all the records
     * matching the given id. */
    TAILQ_FOREACH (to, &h->objs, links)
    {
        TAILQ_FOREACH (td, &to->deps, links)
        {
            if (!strcmp(td->id, id))
            {
                /* Set the rank of the evicted object to the actual
                 * depth +1 in the sequencing array */
                to->rank = to->parent->currank + 1;
                TAILQ_REMOVE(&to->deps, td, links); 
                test_dep_free(td);
                break;
            }
        }

        /* Eviction consists in asserting the '.sequenced' attribute of
         * the chosen test object. */
        if (!strcmp(to->id, id))
            to->sequenced = true;
    }

    return 0;
}

static int test_sequencer (test_t *t)
{
    dbg_return_if (t == NULL, ~0);

    return test_obj_sequencer(&t->test_suites, TEST_SUITE_T);
}

static int test_obj_init (const char *id, test_obj_t *to)
{
    dbg_return_if (id == NULL, ~0);
    dbg_return_if (to == NULL, ~0);

    TAILQ_INIT(&to->deps);
    to->parent = NULL;
    to->sequenced = false;
    to->rank = 0;
    dbg_err_if (u_strlcpy(to->id, id, sizeof to->id)); 

    return 0;
err:
    return ~0;
}
