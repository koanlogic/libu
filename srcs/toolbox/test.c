/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <strings.h>
#include <time.h>

#include <toolbox/carpal.h>
#include <toolbox/misc.h>
#include <toolbox/test.h>

#if defined(HAVE_WAIT3) || defined(HAVE_WAIT)
  #include <sys/wait.h>
#endif  /* HAVE_WAIT3 || HAVE_WAIT */
#ifdef HAVE_STRUCT_RUSAGE
  #include <sys/resource.h>
#endif  /* HAVE_STRUCT_RUSAGE */
#if defined(HAVE_FORK)
  #define U_TEST_SANDBOX_ENABLED  1
#endif  /* HAVE_FORK */
#ifdef HAVE_UNAME
  #include <sys/utsname.h>
#endif  /* HAVE_UNAME */

static char g_interrupted = 0;
static char g_debug = 0;
static char g_verbose = 0;   /* If true, be chatty */
#define CHAT(...)   \
    do { if (g_verbose) { printf("[CHAT] "); printf(__VA_ARGS__); } } while (0)

#define U_TEST_CASE_PID_INITIALIZER   -1

typedef enum { U_TEST_CASE_T, U_TEST_SUITE_T } u_test_what_t;

/* Generic test objects list header. */
typedef struct 
{ 
    unsigned int currank, nchildren;
    LIST_HEAD(, u_test_obj_s) objs;
} TO;

/* Test dependencies list header. */
typedef LIST_HEAD(, u_test_dep_s) TD;

/* A test suite/case dependency (identified by its label). */
struct u_test_dep_s
{
    char id[U_TEST_ID_MAX];           /* Tag of the declared dependency */
    LIST_ENTRY(u_test_dep_s) links;   /* Sibling deps */
    struct u_test_obj_s *upref;       /* Reference to the resolved dependency */
};

typedef struct u_test_dep_s u_test_dep_t;

/* Synoptical test results view. */
struct u_test_syn_s
{
    unsigned int total, pass, fail, abrt, skip;
};

typedef struct u_test_syn_s u_test_syn_t;

/* Generic test data container.  The attributes shared by both test suites and
 * cases are stored here.  The parent container (be it suite or case depends 
 * on '.what' attribute value) is accessed via the T[SC]_HANDLE() macro. */
struct u_test_obj_s
{
    u_test_what_t what;             /* Kind of test object (suite or case) */
    TO *parent;                     /* Head of the list we're attached to */
    char sequenced;                 /* True if sequenced */
    unsigned int rank;              /* Scheduling rank: low has higher prio */
    char id[U_TEST_ID_MAX];         /* Test object identifier: MUST be unique */
    int status;                     /* Test exit code */
    time_t start, stop;             /* Test case/suite begin/end timestamps */
    LIST_ENTRY(u_test_obj_s) links; /* Sibling test cases/suites */
    TD deps;                        /* Test cases/suites we depend on */
};

typedef struct u_test_obj_s u_test_obj_t;

/* A test case. */
struct u_test_case_s
{
    int (*func) (struct u_test_case_s *);   /* The unit test function */
    u_test_obj_t o;                         /* Test case attributes */
    pid_t pid;                              /* Proc Id when exec'd in subproc */
#ifdef HAVE_STRUCT_RUSAGE
    struct rusage stats;                    /* Resources returned by wait3 */
#endif  /* HAVE_STRUCT_RUSAGE */
    struct u_test_suite_s *pts;             /* Parent test suite */
};

/* A test suite. */
struct u_test_suite_s
{
    TO u_test_cases;        /* Head of TCs list */
    u_test_obj_t o;         /* Test suite attributes */
    u_test_syn_t syn;       /* Test cases' synoptical */
    struct u_test_s *pt;    /* Parent test */
};

/* Test report functions. */
struct u_test_rep_s
{
    u_test_rep_f t_cb;
    u_test_suite_rep_f ts_cb;
    u_test_case_rep_f tc_cb;
};

/* A test. */
struct u_test_s
{
    unsigned int currank;       /* Current TS rank when sequencing */
    char id[U_TEST_ID_MAX];     /* Test id */
    TO u_test_suites;           /* Head of TSs list */
    char outfn[4096];           /* Output file name */
    time_t start, stop;         /* Test begin/end timestamps */
    char sandboxed;             /* True if TC's exec'd in subproc */
    unsigned int max_parallel;  /* Max # of test cases executed in parallel */
    u_test_syn_t syn;           /* Test suites' synoptical */
    char host[1024];            /* Host info */

    /* Test report hook functions */
    struct u_test_rep_s reporters;
};

/* Get test suite handle by its '.o(bject)' field. */
#define TS_HANDLE(optr)    \
    (u_test_suite_t *) ((char *) optr - offsetof(u_test_suite_t, o))

/* Get test case handle by its '.o(bject)' field. */
#define TC_HANDLE(optr) \
    (u_test_case_t *) ((char *) optr - offsetof(u_test_case_t, o))

/* Test objects routines. */
static int u_test_obj_dep_add (u_test_dep_t *td, u_test_obj_t *to);
static int u_test_obj_depends_on (const char *id, const char *depid, 
        TO *parent);
static int u_test_obj_evict_id (TO *h, const char *id);
static int u_test_obj_init (const char *id, u_test_what_t what, 
        u_test_obj_t *to);
static int u_test_obj_scheduler (TO *h, int (*sched)(u_test_obj_t *));
static int u_test_obj_sequencer (TO *h);
static u_test_obj_t *u_test_obj_pick_top (TO *h);
static u_test_obj_t *u_test_obj_search (TO *h, const char *id);
static void u_test_obj_free (u_test_obj_t *to);
static void u_test_obj_print (char nindent, u_test_obj_t *to);

/* Test dependencies routines. */
static int u_test_dep_new (const char *id, u_test_dep_t **ptd);
static u_test_dep_t *u_test_dep_search (TD *h, const char *id);
static char u_test_dep_dry (TD *h);
static char u_test_dep_failed (TD *h);
static void u_test_dep_free (u_test_dep_t *td);
static void u_test_dep_print (char nindent, u_test_dep_t *td);

/* Test case routines. */
static void u_test_case_print (u_test_case_t *tc);
static int u_test_case_dep_add (u_test_dep_t *td, u_test_case_t *tc);
static int u_test_case_scheduler (u_test_obj_t *to);
static int u_test_case_report_txt (FILE *fp, u_test_case_t *tc);
static int u_test_case_report_xml (FILE *fp, u_test_case_t *tc);
static int u_test_case_exec (u_test_case_t *tc);

/* Test cases routines. */
static int u_test_cases_reap (TO *h);
static u_test_case_t *u_test_cases_search_by_pid (TO *h, pid_t pid);

/* Test suite routines. */
static int u_test_suite_scheduler (u_test_obj_t *to);
static void u_test_suite_print (u_test_suite_t *ts);
static int u_test_suite_dep_add (u_test_dep_t *td, u_test_suite_t *ts);
static int u_test_suite_report_txt (FILE *fp, u_test_suite_t *ts, 
        u_test_rep_tag_t tag);
static int u_test_suite_report_xml (FILE *fp, u_test_suite_t *ts, 
        u_test_rep_tag_t tag);
static int u_test_suite_update_all_status (u_test_suite_t *ts, int status);

/* Test routines. */
static int u_test_sequencer (u_test_t *t);
static int u_test_scheduler (u_test_t *t);
static void u_test_print (u_test_t *t);
static int u_test_getopt (int ac, char *av[], u_test_t *t);
static void u_test_usage (const char *prog, const char *msg);
static int u_test_set_outfmt (u_test_t *t, const char *fmt);
static int u_test_reporter (u_test_t *t);
static int u_test_report_txt (FILE *fp, u_test_t *t, u_test_rep_tag_t tag);
static int u_test_report_xml (FILE *fp, u_test_t *t, u_test_rep_tag_t tag);
static int u_test_uname (u_test_t *t);

/* Test synoptical info routines. */
static int u_test_syn_update (u_test_syn_t *syn, TO *h);

/* Misc routines. */
static const char *u_test_status_str (int status);
static int u_test_init (u_test_t *t);
static int u_test_signals (void);
static void u_test_interrupted (int signo);
static void u_test_bail_out (TO *h);
static int u_test_obj_ts_fmt (u_test_obj_t *to, char b[80], char e[80], 
        char d[80]);
#ifdef HAVE_STRUCT_RUSAGE
static int u_test_case_rusage_fmt (u_test_case_t *tc, char uti[80], 
        char sti[80]);
static int __timeval_fmt (struct timeval *tv, char t[80]);
#endif  /* HAVE_STRUCT_RUSAGE */

/* Pre-cooked reporters. */
struct u_test_rep_s xml_reps = {
    u_test_report_xml,
    u_test_suite_report_xml,
    u_test_case_report_xml
};

struct u_test_rep_s txt_reps = {
    u_test_report_txt,
    u_test_suite_report_txt,
    u_test_case_report_txt
};

/**
    \defgroup test Unit testing
    \{
        The \ref test module offers a set of interfaces by means of which a 
        programmer can organise its own software validation suite.  
        Basically you define a test suite, which in turn comprises a set of
        test cases (aka unit tests), and recall it in the main test program.
        Each test case is associated to a ::u_test_f routine which 
        takes its own ::u_test_case_t reference as argument and is requested 
        to return ::U_TEST_SUCCESS in case the unit test succeeds and 
        ::U_TEST_FAILURE otherwise. 
        A test case is attached to its "mother" test suite with the
        ::u_test_case_register function.  On the other hand, a test suite is 
        attached to the main test program with the ::u_test_suite_add function,
        as shown in the following example:
    \code

    int test_case_MY_TC (u_test_case_t *tc)
    {
        // unit test assertions here
        ...
        return U_TEST_SUCCESS;
    err:
        return U_TEST_FAILURE;
    }

    int test_suite_MY_TS_register (u_test_t *t)
    {
        u_test_suite_t *ts;

        u_test_suite_new("MY_TS", &ts);
        u_test_case_register("MY_TC", test_case_MY_TC, ts);

        return u_test_suite_add(ts, t);
    }
    \endcode

        To build the main test program (let's call it \c runtest) you define a 
        \c main function like the one in the example below, import your test 
        suite invoking the test suites' register functions, and then call the 
        ::u_test_run function to execute the tests:
    \code
    int main (int argc, char *argv[])
    {
        u_test_t *t = NULL;

        u_test_new("MY_TESTS", &t);
        test_suite_MY_TS_register(t);

        return u_test_run(argc, argv, t)
    }
    \endcode

        The \c runtest program has a number of built-in command line options 
        to tweak some test execution features, the most important of which are:
        - \c -f to specify a format for the test report file: \c txt or \c xml
        - \c -o to pass a specific name other than the default as report file
        - \c -s to force serialization in unit tests' execution (instead of
          the default which is to parallelize as much as possible).  This
          option (which could be useful when trying to analize a test bug) also
          turns off the sandboxing mechanism by which tests are executed in
          safe, isolated bubbles.

       An \link test_report.xsl xsl file \endlink file - together with its twin
       \link test_report.css css \endlink - is provided in \c contrib/ as an 
       example to automatically build an HTML report page from the xml report 
       file.  Let \c runtest produce an xml ouput and supply it to \c xsltproc
       together with the xsl template:
    \code
    $ ./runtest -f xml -o my_test_report.xml
    $ xstlproc test_report.xsl my_test_report.xml 
    \endcode
    A single html file is produced which gets its style definitions from
    the \c test_report.css file (which must be in the same directory).

    If in doubt, or in search for inspiration, take a look at LibU \c test/ 
    directory.
 */

/** 
 *  \brief  Run tests.
 *
 *  Run tests. It shall be called by the \c main() function and will return 
 *  when all tests have been executed or an error occurred.
 *
 *  \param  ac  \c main's \c argc argument
 *  \param  av  \c main's \c argv argument
 *  \param  t   an ::u_test_t object handler
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_test_run (int ac, char *av[], u_test_t *t)
{
    con_err_if (u_test_init(t));
    con_err_if (u_test_getopt(ac, av, t));
    con_err_if (u_test_sequencer(t));

    /* Take a look at dependencies after sequencing. */
    if (g_debug)
        u_test_print(t);  

    con_err_if (u_test_scheduler(t));
    con_err_if (u_test_reporter(t));

    return (t->syn.total == t->syn.pass) ? 0 : ~0;
err:
    return ~0;
}

/** 
 *  \brief  Register a dependency for the given test suite
 *
 *  Add the test suite named \p id as a dependency for the test suite \p ts
 *
 *  \param  id  the id of an already ::u_test_suite_add'ed test suite
 *  \param  ts  handler of the current test suite
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_test_suite_dep_register (const char *id, u_test_suite_t *ts)
{
    u_test_dep_t *td = NULL;

    dbg_err_if (u_test_dep_new(id, &td));
    dbg_err_if (u_test_suite_dep_add(td, ts));

    return 0;
err:
    u_test_dep_free(td);
    return ~0;
}

/** 
 *  \brief  An alternative way to describe dependencies between test cases
 *
 *  Add the test case named \p depid as a dependency for the test case named
 *  \p tcid in the context of the \p ts test suite.
 *
 *  \param  tcid    a test case identified by its name
 *  \param  depid   name of the test case on which \p tcid depends on
 *  \param  ts      The parent test suite handler
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 *
 *  \sa u_test_case_dep_register
 */
int u_test_case_depends_on (const char *tcid, const char *depid, 
        u_test_suite_t *ts)
{
    return u_test_obj_depends_on(tcid, depid, &ts->u_test_cases);
}

/** 
 *  \brief  An alternative way to describe dependencies between test suites
 *
 *  Add the test suite named \p depid as a dependency for the test suite named
 *  \p tsid in the context of the \p t test.
 *
 *  \param  tsid    a test suite identified by its name
 *  \param  depid   name of the test suite on which \p tsid depends on
 *  \param  t       The parent test handler
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 *
 *  \sa u_test_suite_dep_register
 */
int u_test_suite_depends_on (const char *tsid, const char *depid, u_test_t *t)
{
    return u_test_obj_depends_on(tsid, depid, &t->u_test_suites);
}

/** 
 *  \brief  Register a dependency for the given test case
 *
 *  Add the test case named \p id as a dependency for the test case \p ts
 *
 *  \param  id  the id of an already ::u_test_case_add'ed test case
 *  \param  tc  handler of the current test case
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_test_case_dep_register (const char *id, u_test_case_t *tc)
{
    u_test_dep_t *td = NULL;

    dbg_err_if (u_test_dep_new(id, &td));
    dbg_err_if (u_test_case_dep_add(td, tc));

    return 0;
err:
    u_test_dep_free(td);
    return ~0;
}

/** 
 *  \brief  Create and register a new test case
 *
 *  Register a new test case named \p id to its parent test suite \p ts.
 *  The function \p func, which must match the ::u_test_f prototype, contains 
 *  the test code to be executed.
 *
 *  \param  id      the name of the new test case
 *  \param  func    the unit test function
 *  \param  ts      the parent test suite handler
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 *
 *  \sa ::u_test_case_new, ::u_test_case_add
 */
int u_test_case_register (const char *id, u_test_f func, u_test_suite_t *ts)
{
    u_test_case_t *tc = NULL;

    dbg_err_if (u_test_case_new(id, func, &tc));
    dbg_err_if (u_test_case_add(tc, ts));

    return 0;
err:
    u_test_case_free(tc);
    return ~0;
}

/**
 *  \brief  printf-like function to be called from inside the test case function
 *
 *  A printf-like function to be called from inside the test case function
 *
 *  \param  tc  the test case handler
 *  \param  fmt printf-like format string
 *  \param  ... variable number of arguments that feed \p fmt
 *
 *  \return the number of characters printed (not including the trailing NUL)
 *          or a negative value if an output error occurs 
 */
int u_test_case_printf (u_test_case_t *tc, const char *fmt, ...)
{
    int rc;
    va_list ap;

    dbg_return_if (tc == NULL, -1);

    (void) printf("{%s} ", tc->o.id);

    va_start(ap, fmt);
    rc = vprintf(fmt, ap);
    va_end(ap);

    if (rc > 0)
        (void) printf("\n");

    return rc;
}

/**
 *  \brief  Create a new test suite
 *
 *  Create a new test suite named \p id and return its handler as a result 
 *  argument
 *
 *  \param  id  the name of the new test suite
 *  \param  pts test suite handler as a result argument
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_test_suite_new (const char *id, u_test_suite_t **pts)
{
    u_test_suite_t *ts = NULL;

    dbg_return_if (id == NULL, ~0);
    dbg_return_if (pts == NULL, ~0);

    dbg_err_sif ((ts = u_malloc(sizeof *ts)) == NULL);
    LIST_INIT(&ts->u_test_cases.objs);
    ts->u_test_cases.currank = 0;
    memset(&ts->syn, 0, sizeof ts->syn);

    dbg_err_if (u_test_obj_init(id, U_TEST_SUITE_T, &ts->o));

    *pts = ts;

    return 0;
err:
    u_test_suite_free(ts);
    return ~0;
}

/**
 *  \brief  Free resources allocated to a test suite
 *
 *  Free resources allocated to the supplied test suite (including its test 
 *  cases and related dependencies).
 *
 *  \param  ts  handler of the test suite that shall be disposed
 *
 *  \return nothing
 */
void u_test_suite_free (u_test_suite_t *ts)
{
    u_test_case_t *tc;
    u_test_obj_t *to, *nto;

    if (ts == NULL)
        return;

    /* Free dependencies. */
    u_test_obj_free(&ts->o);

    /* Free test cases. */
    LIST_FOREACH_SAFE (to, &ts->u_test_cases.objs, links, nto)
    {
        tc = TC_HANDLE(to);
        u_test_case_free(tc);
    }

    u_free(ts);

    return;
}

/**
 *  \brief  Free resources allocated to a test
 *
 *  Free resources allocated to the supplied test (including its test suites, 
 *  test cases and attached dependencies).
 *
 *  \param  t   handler of the test that shall be disposed
 *
 *  \return nothing
 */
void u_test_free (u_test_t *t)
{
    u_test_suite_t *ts;
    u_test_obj_t *to, *nto;

    if (t == NULL)
        return;

    /* Free all children test suites. */
    LIST_FOREACH_SAFE (to, &t->u_test_suites.objs, links, nto)
    {
        ts = TS_HANDLE(to);
        u_test_suite_free(ts);
    } 

    /* Free the container. */
    u_free(t);

    return;
}

/**
 *  \brief  Set a custom test reporter function
 *
 *  Set \p func as the reporter function for the given test \p t.  The supplied
 *  function must match the ::u_test_rep_f prototype.
 *
 *  \param  t       handler for the test
 *  \param  func    the custom reporter function
 *
 *  \retval  0  on success
 *  \retval ~0  on failure (i.e. a \c NULL parameter was supplied)
 */
int u_test_set_u_test_rep (u_test_t *t, u_test_rep_f func)
{
    dbg_return_if (t == NULL, ~0);
    dbg_return_if (func == NULL, ~0);

    t->reporters.t_cb = func;

    return 0;
}

/**
 *  \brief  Set a custom test case reporter function
 *
 *  Set \p func as the test case reporter function for the given test \p t.  
 *  The supplied function must match the ::u_test_case_rep_f prototype.
 *
 *  \param  t       handler for the test
 *  \param  func    the custom test case reporter function
 *
 *  \retval  0  on success
 *  \retval ~0  on failure (i.e. a \c NULL parameter was supplied)
 */
int u_test_set_u_test_case_rep (u_test_t *t, u_test_case_rep_f func)
{
    dbg_return_if (t == NULL, ~0);
    dbg_return_if (func == NULL, ~0);

    t->reporters.tc_cb = func;

    return 0;
}

/**
 *  \brief  Set a custom test suite reporter function
 *
 *  Set \p func as the test suite reporter function for the given test \p t.  
 *  The supplied function must match the ::u_test_suite_rep_f prototype.
 *
 *  \param  t       handler for the test
 *  \param  func    the custom test suite reporter function
 *
 *  \retval  0  on success
 *  \retval ~0  on failure (i.e. a \c NULL parameter was supplied)
 */
int u_test_set_u_test_suite_rep (u_test_t *t, u_test_suite_rep_f func)
{
    dbg_return_if (t == NULL, ~0);
    dbg_return_if (func == NULL, ~0);

    t->reporters.ts_cb = func;

    return 0;
}

/**
 *  \brief  Set the output file name for the test report
 *
 *  Set the output file name for the test report to \p outfn
 *
 *  \param  t       a test handler
 *  \param  outfn   file path name
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_test_set_outfn (u_test_t *t, const char *outfn)
{
    dbg_return_if (t == NULL, ~0);
    dbg_return_if (outfn == NULL, ~0);

    dbg_return_if (u_strlcpy(t->outfn, outfn, sizeof t->outfn), ~0);

    return 0;
}

/**
 *  \brief  Create a new test
 *
 *  Create a new test named \p id and return its handler via the result 
 *  argument \p pt
 *
 *  \param  id  the name of the test
 *  \param  pt  handler for the newly created test as a result argument
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_test_new (const char *id, u_test_t **pt)
{
    u_test_t *t = NULL;

    dbg_return_if (pt == NULL, ~0);

    dbg_err_sif ((t = u_malloc(sizeof *t)) == NULL);

    LIST_INIT(&t->u_test_suites.objs);
    t->u_test_suites.currank = 0;
    dbg_err_if (u_strlcpy(t->id, id, sizeof t->id));
    t->currank = 0;
    t->start = t->stop = -1;
    (void) u_strlcpy(t->outfn, U_TEST_OUTFN_DFL, sizeof t->outfn);
#ifdef U_TEST_SANDBOX_ENABLED
    /* The default is to spawn sub-processes to execute test cases - 
     * on systems where the fork(2) syscall is available. */
    t->sandboxed = 1;   
#else   /* !U_TEST_SANDBOX_ENABLED */
    t->sandboxed = 0;
#endif  /* U_TEST_SANDBOX_ENABLED */
    t->max_parallel = U_TEST_MAX_PARALLEL;

    /* Set default report routines (may be overwritten). */
    t->reporters = txt_reps;

    /* Zeroize synoptical test info. */
    memset(&t->syn, 0, sizeof t->syn);  

    *pt = t;

    return 0;
err:
    u_free(t);
    return ~0;
}

/**
 *  \brief  Free resources allocated to a test case
 *
 *  Free resources allocated to the supplied test case (including its 
 *  dependencies).
 *
 *  \param  tc  handler of the test case that shall be disposed
 *
 *  \return nothing
 */
void u_test_case_free (u_test_case_t *tc)
{
    if (tc == NULL)
        return;

    /* Free the attached dependencies. */
    u_test_obj_free(&tc->o);

    /* Free the container. */
    u_free(tc);

    return;
}

/** 
 *  \brief  Create a new test case
 *
 *  Create a new test case named \p id with \p func as its unit test procedure,
 *  and return its handler via the result argument \p ptc.  The function must 
 *  match the ::u_test_f prototype.
 *
 *  \param  id      the name of the new test case
 *  \param  func    the unit test function
 *  \param  ptc     the parent test suite handler
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_test_case_new (const char *id, u_test_f func, u_test_case_t **ptc)
{
    u_test_case_t *tc = NULL;

    dbg_return_if (id == NULL, ~0);
    dbg_return_if (ptc == NULL, ~0);

    dbg_err_sif ((tc = u_malloc(sizeof *tc)) == NULL);
    dbg_err_if (u_test_obj_init(id, U_TEST_CASE_T, &tc->o));

    tc->func = func;
    tc->pid = U_TEST_CASE_PID_INITIALIZER;
#ifdef HAVE_STRUCT_RUSAGE
    memset(&tc->stats, 0, sizeof tc->stats);
#endif  /* HAVE_STRUCT_RUSAGE */

    *ptc = tc;

    return 0;
err:
    u_test_case_free(tc);
    return ~0;
}

/**
 *  \brief  Add a test case to its parent test suite
 *
 *  Add the test case referenced by \p tc to the test suite \p ts.
 *
 *  \param  tc  a test case handler
 *  \param  ts  its parent test suite
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_test_case_add (u_test_case_t *tc, u_test_suite_t *ts)
{
    dbg_return_if (tc == NULL, ~0);
    dbg_return_if (ts == NULL, ~0);

    LIST_INSERT_HEAD(&ts->u_test_cases.objs, &tc->o, links);
    tc->o.parent = &ts->u_test_cases;
    tc->pts = ts;

    return 0;
}

/**
 *  \brief  Add a test suite to its parent test
 *
 *  Add the test suite referenced by \p ts to the test \p t.
 *
 *  \param  ts  a test suite handler
 *  \param  t   its parent test
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_test_suite_add (u_test_suite_t *ts, u_test_t *t)
{
    dbg_return_if (t == NULL, ~0);
    dbg_return_if (ts == NULL, ~0);

    LIST_INSERT_HEAD(&t->u_test_suites.objs, &ts->o, links);
    ts->o.parent = &t->u_test_suites;
    ts->pt = t;

    return 0;
}

/**
 *  \brief  Print the test structure
 *
 *  Try to pretty print the whole test structure.  Use it as a debug hint when
 *  things behaves unexpectedly, or to look at how the information is organized
 *  internally.
 *
 *  \param  t   the handler of the test to print
 *
 *  \return nothing
 */
void u_test_print (u_test_t *t)
{
    u_test_obj_t *to;

    dbg_return_if (t == NULL, );

    u_con("[test] %s", t->id);  

    LIST_FOREACH (to, &t->u_test_suites.objs, links)
        u_test_suite_print(TS_HANDLE(to));

    return;
}

/**
 *  \}
 */ 

static void u_test_suite_print (u_test_suite_t *ts)
{
    u_test_dep_t *td;
    u_test_obj_t *to;

    dbg_return_if (ts == NULL, );

    u_test_obj_print(4, &ts->o);

    LIST_FOREACH (td, &ts->o.deps, links)
        u_test_dep_print(4, td);

    LIST_FOREACH (to, &ts->u_test_cases.objs, links)
        u_test_case_print(TC_HANDLE(to));

    return;
}

static void u_test_case_print (u_test_case_t *tc)
{
    u_test_dep_t *td;

    dbg_return_if (tc == NULL, );

    u_test_obj_print(8, &tc->o);

    LIST_FOREACH (td, &tc->o.deps, links)
        u_test_dep_print(8, td);

    return;
}

static void u_test_obj_print (char nindent, u_test_obj_t *to)
{
    dbg_return_if (to == NULL, );

    u_con("%*c=> [%s] %s", 
            nindent, ' ', to->what == U_TEST_CASE_T ? "case" : "suite", to->id);

    /* attributes */
    u_con("%*c    .rank = %u", nindent, ' ', to->rank);  
    u_con("%*c    .seq = %s", nindent, ' ', to->sequenced ? "true" : "false");

    return;
}

static void u_test_dep_print (char nindent, u_test_dep_t *td)
{
    u_con("%*c    .<dep> = %s", nindent, ' ', td->id);
    return;
}

static int u_test_dep_new (const char *id, u_test_dep_t **ptd)
{
    u_test_dep_t *td = NULL;

    dbg_return_if (id == NULL, ~0);
    dbg_return_if (ptd == NULL, ~0);

    dbg_err_sif ((td = u_malloc(sizeof *td)) == NULL);
    dbg_err_if (u_strlcpy(td->id, id, sizeof td->id));
    td->upref = NULL;   /* When non-NULL it is assumed as resolved. */

    *ptd = td;

    return 0;
err:
    u_test_dep_free(td);
    return ~0;
}

static void u_test_dep_free (u_test_dep_t *td)
{
    if (td)
        u_free(td);

    return;
}

static u_test_dep_t *u_test_dep_search (TD *h, const char *id)
{
    u_test_dep_t *td;

    dbg_return_if (h == NULL, NULL);
    dbg_return_if (id == NULL, NULL);

    LIST_FOREACH (td, h, links)
    {
        if (!strcmp(td->id, id))
            return td;
    }

    return NULL;
}

static u_test_obj_t *u_test_obj_search (TO *h, const char *id)
{
    u_test_obj_t *to;

    dbg_return_if (h == NULL, NULL);
    dbg_return_if (id == NULL, NULL);

    LIST_FOREACH (to, &h->objs, links)
    {
        if (!strcmp(to->id, id))
            return to;
    }

    return NULL;
}

static int u_test_suite_dep_add (u_test_dep_t *td, u_test_suite_t *ts)
{
    return u_test_obj_dep_add(td, &ts->o);
}

static int u_test_case_dep_add (u_test_dep_t *td, u_test_case_t *tc)
{
    return u_test_obj_dep_add(td, &tc->o);
}

/* It MUST be called AFTER the test case/suite on which it is established 
 * has been added. */
static int u_test_obj_dep_add (u_test_dep_t *td, u_test_obj_t *to)
{
    dbg_return_if (to == NULL, ~0);
    dbg_return_if (td == NULL, ~0);

    /* See if the dependency isn't already recorded, in which case
     * insert it into the dependency list for this test case/suite. */
    if (u_test_dep_search(&to->deps, to->id) == NULL)
    {
        LIST_INSERT_HEAD(&to->deps, td, links);
    }
    else
        u_free(td);

    return 0;
}

static void u_test_obj_free (u_test_obj_t *to)
{
    u_test_dep_t *td, *ntd;

    /* Just free the attached deps list: expect 'to' being a pointer to a
     * test object in a bigger malloc'd chunk (suite or case) which will be
     * later free'd by its legitimate owner. */
    LIST_FOREACH_SAFE (td, &to->deps, links, ntd)
        u_test_dep_free(td);

    return;
}

static char u_test_dep_dry (TD *h)
{
    u_test_dep_t *td;

    dbg_return_if (h == NULL, 1);

    LIST_FOREACH (td, h, links)
    {
        /* Search for any non-resolved dependency. */
        if (td->upref == NULL)
            return 0;
    }

    /* We get here also when the dependency list is empty. */
    return 1;
}

static char u_test_dep_failed (TD *h)
{
    u_test_dep_t *td;

    dbg_return_if (h == NULL, 0);

    LIST_FOREACH (td, h, links)
    {
        if (td->upref && td->upref->status != U_TEST_SUCCESS)
            return 1;
    }

    return 0;
}

static u_test_obj_t *u_test_obj_pick_top (TO *h)
{
    /* Top element is a (not already sequenced) test object with no deps. */
    u_test_obj_t *to;

    LIST_FOREACH (to, &h->objs, links)
    {
        if (!to->sequenced && u_test_dep_dry(&to->deps))
        {
            /* Record the reached depth: it will be used by the
             * next evicted element. */
            to->parent->currank = U_MAX(to->rank, to->parent->currank);
            return to;
        }
    }

    return NULL;
}

/* Do a (variation on) topological sorting over the partially ordered sets
 * of test suites/cases.  We are primarily concerned in equally ranking all
 * test objects which are at the same "dependency-depth" so that the scheduler
 * can exec tests in the same rank-pool in parallel.  We are also interested 
 * in creating a partial order for cases where the scheduler is forced to 
 * run sequentially (i.e. no system fork(2) or by explicit user request).
 * In this respect, the ranking algorithm creates a partition of the 
 * case/suite's sets into equivalency classes from where any element can be 
 * picked at will (until EC exhaustion) to create a suitable sequence.
 * NOTE that higher ranks mean lower priority */
static int u_test_obj_sequencer (TO *h)
{
    u_test_obj_t *to, *fto;
    u_test_suite_t *ts;

    dbg_return_if (h == NULL, ~0);

    /* Pick top test cases/suites one by one until they are all sequenced.*/
    while ((to = u_test_obj_pick_top(h)) != NULL)
    {
        dbg_err_if (u_test_obj_evict_id(h, to->id));
    }

    /* Test if we got out because of a cycle in deps, i.e. check if we 
     * still have any non-sequenced test case/suite. */
    LIST_FOREACH (to, &h->objs, links)
    {
        dbg_err_ifm (!to->sequenced, 
                "%s not sequenced: dependency loop !", to->id);
    }

    /* A test suite need to recur into its test cases. */
    if ((fto = LIST_FIRST(&h->objs)) && fto->what == U_TEST_SUITE_T)
    {
        LIST_FOREACH (to, &h->objs, links)
        {
            ts = TS_HANDLE(to);
            dbg_err_if (u_test_obj_sequencer(&ts->u_test_cases));
        }
    }

    return 0;
err:
    return ~0;
}

static int u_test_obj_evict_id (TO *h, const char *id)
{
    u_test_obj_t *to;
    u_test_dep_t *td;

    dbg_return_if (id == NULL, ~0); 

    /* Delete from the deps list of every test suite, all the records
     * matching the given id. */
    LIST_FOREACH (to, &h->objs, links)
    {
        LIST_FOREACH (td, &to->deps, links)
        {
            if (!strcmp(td->id, id))
            {
                /* Set the rank of the evicted object to the actual
                 * depth +1 in the sequencing array */
                to->rank = to->parent->currank + 1;
                /* Link the dependency to its resolved test object. */
                td->upref = u_test_obj_search(to->parent, id);
                break;
            }
        }

        /* Eviction consists in asserting the '.sequenced' attribute of
         * the chosen test object. */
        if (!strcmp(to->id, id))
            to->sequenced = 1;
    }

    return 0;
}

static int u_test_sequencer (u_test_t *t)
{
    dbg_return_if (t == NULL, ~0);

    return u_test_obj_sequencer(&t->u_test_suites);
}

static int u_test_obj_init (const char *id, u_test_what_t what, 
        u_test_obj_t *to)
{
    dbg_return_if (id == NULL, ~0);
    dbg_return_if (to == NULL, ~0);

    LIST_INIT(&to->deps);
    to->what = what;
    to->parent = NULL;
    to->sequenced = 0;
    to->rank = 0;
    to->status = U_TEST_SUCCESS;
    to->start = to->stop = -1;
    dbg_err_if (u_strlcpy(to->id, id, sizeof to->id)); 

    return 0;
err:
    return ~0;
}

static int u_test_obj_depends_on (const char *id, const char *depid, TO *parent)
{
    u_test_obj_t *to;
    u_test_dep_t *td = NULL;

    /* The object for which we are adding the dependency must be already
     * in place. */
    dbg_err_if ((to = u_test_obj_search(parent, id)) == NULL);

    /* Check if the same dependency is already recorded. */
    if ((td = u_test_dep_search(&to->deps, depid)))
        return 0;

    /* In case it is a new dependency, create and stick it to the object
     * deps list. */
    dbg_err_if (u_test_dep_new(depid, &td));
    dbg_err_if (u_test_obj_dep_add(td, to));

    return 0;
err:
    u_test_dep_free(td);
    return ~0;
}

static int u_test_obj_scheduler (TO *h, int (*sched)(u_test_obj_t *))
{
    char simple_sched = 0;
    unsigned int r, parallel = 0;
    u_test_obj_t *to, *fto, *tptr;
    u_test_case_t *ftc;

    dbg_return_if (h == NULL, ~0);
    dbg_return_if (sched == NULL, ~0);

    /* Pop the first element in list which will be used to reconnect to its 
     * upper layer relatives (i.e. test suite & test). */
    if ((fto = LIST_FIRST(&h->objs)) == NULL)
        return 0;   /* List empty, go out. */

    /* Initialize the spawned children (i.e. test cases) counter to 0.
     * It will be incremented on each u_test_case_exec() invocation, when 
     * in sandboxed mode. */
    h->nchildren = 0;

    /* See which kind of scheduler we need to use. 
     * Systems without fork(2) have the .sandboxed attribute hardcoded to 
     * 'false', hence any U_TEST_CASE_T will be assigned to the "simple"
     * scheduler. */
    if (fto->what == U_TEST_SUITE_T)
        simple_sched = 1;
    else
        simple_sched = (ftc = TC_HANDLE(fto), ftc->pts->pt->sandboxed) ? 0 : 1;

    /* Go through all the test objs and select those at the current scheduling 
     * rank: first lower rank TO's (i.e. highest priority). */
    for (r = 0; r <= h->currank && !g_interrupted; ++r)
    {
        /* TS's and non-sandboxed TC's are simply scheduled one-by-one. */
        if (simple_sched)
        {
            LIST_FOREACH (to, &h->objs, links)
            {
                if (to->rank == r)
                {
                    /* Check for any dependency failure. */
                    if (u_test_dep_failed(&to->deps))
                    {
                        CHAT("Skip %s due to dependency failure\n", to->id);

                        to->status = U_TEST_SKIPPED; 

                        /* Update test cases status. */
                        if (to->what == U_TEST_SUITE_T)
                        {
                            (void) u_test_suite_update_all_status(TS_HANDLE(to),
                                    U_TEST_SKIPPED);
                        }

                        continue;
                    }

                    (void) sched(to);
                }
            }
        }
        else    
        {
            /* Sandboxed TC's scheduling logic is slightly more complex:
             * we run them in chunks of U_TEST_MAX_PARALLEL cardinality, then
             * we reap the whole chunk and start again with another one, until
             * the list has been traversed. 'tptr' is the pointer to the next 
             * TC to be run. */
            u_test_case_t *tc;

            tptr = LIST_FIRST(&h->objs);
            tc = TC_HANDLE(tptr);

        resched:
            for (to = tptr; to != NULL; to = LIST_NEXT(to, links))
            {
                if (to->rank == r)
                {
                    /* Avoid scheduling a TO that has failed dependencies. */
                    if (u_test_dep_failed(&to->deps))
                    {
                        CHAT("Skip %s due to dependency failure\n", to->id);
                        to->status = U_TEST_SKIPPED; 
                        continue;
                    }

                    /* If no dependency failed, we can schedule the TC at the 
                     * current rank. */
                    dbg_if (sched(to));
            
                    /* See if we reached this chunk's upper bound. */
                    if ((parallel += 1) == tc->pts->pt->max_parallel)
                    {
                        /* Save restart reference and jump to reap. */
                        tptr = LIST_NEXT(to, links);
                        goto reap;
                    }
                }
                    
                /* Make sure 'tptr' is unset, so that, in case we go out on 
                 * loop exhaustion, the reaper branch knows that we don't need
                 * to be restarted. */
                tptr = NULL;
            }

        reap:
            dbg_err_if (u_test_cases_reap(h));

            /* If 'tptr' was set, it means that we need to restart. */
            if (tptr != NULL)
            {
                parallel = 0;       /* Reset parallel counter. */
                goto resched;       /* Go to sched again. */
            }
        }
    }

    return 0;
err:
    return ~0;
}

static int u_test_cases_reap (TO *h)
{
#ifdef U_TEST_SANDBOX_ENABLED
    int status;
    pid_t child;
    u_test_case_t *tc;
#ifdef HAVE_STRUCT_RUSAGE
    struct rusage rusage;
#endif  /* HAVE_STRUCT_RUSAGE */

    dbg_return_if (h == NULL, ~0);

    while (h->nchildren > 0)
    {
#ifdef HAVE_WAIT3
        if ((child = wait3(&status, 0, &rusage)) == -1)
#else   /* i.e. HAVE_WAIT */
        if ((child = wait(&status)) == -1)
#endif
        {
            /* Interrupted (should be by user interaction) or no more child 
             * processes */
            if (errno == EINTR || errno == ECHILD)
                break;
            else    /* THIS IS BAD */
                con_err_sifm (1, "wait3 failed badly, test aborted");
        }

        if ((tc = u_test_cases_search_by_pid(h, child)) == NULL)
        {
            CHAT("not a waited test case: %d\n", child);
            continue;
        }

        /* We assume that a test case which received a SIGSTOP
         * is going to be resumed (i.e. SIGCONT'inued) independently. */
        if (WIFSTOPPED(status))
            continue;

        /* Reset pid to the default. */
        tc->pid = U_TEST_CASE_PID_INITIALIZER;
 
        /* Update test case stats. */
        if (WIFEXITED(status))
        {
            /* The test case terminated normally, that is, by calling exit(3) 
             * or _exit(2), or by returning from main().  
             * We expect a compliant test case to return one of U_TEST_FAILURE
             * or U_TEST_SUCCESS. */
            tc->o.status = WEXITSTATUS(status);
            dbg_if (tc->o.status != U_TEST_SUCCESS && 
                    tc->o.status != U_TEST_FAILURE);
        }
        else if (WIFSIGNALED(status))
        {
            /* The test case was terminated by a signal. */
            tc->o.status = U_TEST_ABORTED;
            CHAT("test case %s terminated by signal %d\n", tc->o.id, 
                    WTERMSIG(status));
        }

#ifdef HAVE_STRUCT_RUSAGE
        tc->stats = rusage;
#endif  /* HAVE_STRUCT_RUSAGE */

        tc->o.stop = time(NULL);

        h->nchildren -= 1;
    }

    if (h->nchildren != 0)
    {
        CHAT("%u child(ren) still in wait !\n", h->nchildren);
        goto err;
    }

    return 0;
err:
    return ~0;
#else   /* !U_TEST_SANDBOX_ENABLED */
    u_con("What are you doing here ?");
    return ~0;
#endif  /* U_TEST_SANDBOX_ENABLED */
}

static u_test_case_t *u_test_cases_search_by_pid (TO *h, pid_t pid)
{
    u_test_obj_t *to;
    u_test_case_t *tc;

    LIST_FOREACH (to, &h->objs, links)
    {
        if (tc = TC_HANDLE(to), tc->pid == pid)
            return tc;
    }

    return NULL;
}

static int u_test_syn_update (u_test_syn_t *syn, TO *h)
{
    u_test_obj_t *to;

    dbg_return_if (h == NULL, ~0);
    dbg_return_if (syn == NULL, ~0);

    LIST_FOREACH (to, &h->objs, links)
    {
        syn->total += 1;

        switch (to->status)
        {
            case U_TEST_SUCCESS:
                syn->pass += 1;
                break;
            case U_TEST_FAILURE:
                syn->fail += 1;
                break;
            case U_TEST_ABORTED:
                syn->abrt += 1;
                break;
            case U_TEST_SKIPPED:
                syn->skip += 1;
                break;
            default:
                u_warn("unknown status %d in test obj %s", to->status, to->id);
        }
    }

    return 0;
}

static int u_test_scheduler (u_test_t *t)
{
    int rc;

    dbg_return_if (t == NULL, ~0);

    /* Run the test scheduler. */ 
    rc = u_test_obj_scheduler(&t->u_test_suites, u_test_suite_scheduler);

    /* Collect synoptical stats. */
    (void) u_test_syn_update(&t->syn, &t->u_test_suites);

    return rc;
}

static int u_test_suite_scheduler (u_test_obj_t *to)
{
    int rc;
    u_test_obj_t *tco;
    u_test_suite_t *ts;

    dbg_return_if (to == NULL || (ts = TS_HANDLE(to)) == NULL, ~0);

    CHAT("now scheduling test suite %s\n", ts->o.id);

    /* Record test suite begin timestamp. */
    ts->o.start = time(NULL);

    /* Go through children test cases. */
    rc = u_test_obj_scheduler(&ts->u_test_cases, u_test_case_scheduler);

    /* See if we've been interrupted, in which case we need to cleanup
     * as much as possible before exit'ing. */
    if (g_interrupted)
        u_test_bail_out(&ts->u_test_cases);

    /* Record test suite end timestamp. */
    ts->o.stop = time(NULL);

    /* Update stats for reporting. */
    LIST_FOREACH (tco, &ts->u_test_cases.objs, links)
    {
        /* If any of our test cases failed, set the overall status 
         * to FAILURE */
        if (tco->status != U_TEST_SUCCESS)
        {
            ts->o.status = U_TEST_FAILURE;
            break;
        }
    }

    /* Collect synoptical stats. */
    (void) u_test_syn_update(&ts->syn, &ts->u_test_cases);

    return rc;
}

static int u_test_suite_update_all_status (u_test_suite_t *ts, int status)
{
    u_test_obj_t *to;

    dbg_return_if (ts == NULL, ~0);

    LIST_FOREACH (to, &ts->u_test_cases.objs, links)
        to->status = status;

    return 0;
}

static int u_test_case_scheduler (u_test_obj_t *to)
{
    dbg_return_if (to == NULL, ~0);

    CHAT("now scheduling test case %s\n", to->id);

    return u_test_case_exec(TC_HANDLE(to));
}

static int u_test_case_exec (u_test_case_t *tc)
{
    int rc;
    pid_t pid;

    dbg_return_if (tc == NULL, ~0);

    if (tc->func == NULL)
        return 0;

    /* Record test case begin timestamp. */
    tc->o.start = time(NULL);

    /* The following condition is always asserted on systems with no fork(2)
     * syscall.  See u_test_new(). */
    if (!tc->pts->pt->sandboxed)
    {
        dbg_if ((rc = tc->func(tc)) != U_TEST_SUCCESS && rc != U_TEST_FAILURE);

        /* Update test case stats. */
        tc->o.status = rc;
        tc->o.stop = time(NULL);

        return 0;
    }

#ifdef U_TEST_SANDBOX_ENABLED
    switch (pid = fork())
    {
        case -1:
            u_warn("test case %s spawn failed: %s", tc->o.id, strerror(errno));
            return ~0;
        case 0:
            exit(tc->func(tc));
        default:
            break;
    }

    tc->pid = pid;                  /* Set test case process pid */
    tc->o.parent->nchildren += 1;   /* Tell the u_test_case's head we have one
                                       more child */

    CHAT("started test case %s with pid %d\n", tc->o.id, (int) tc->pid);
#endif  /* U_TEST_SANDBOX_ENABLED */

    /* always return ok here, the reaper will know the test exit status */
    return 0;
}

static int u_test_reporter (u_test_t *t)
{
    FILE *fp;
    u_test_obj_t *tso, *tco;
    u_test_suite_t *ts;

    dbg_return_if (t == NULL, ~0);

    /* Assume that each report function is set correctly (we trust the
     * setters having done a good job). */

    if (strcmp(t->outfn, "-"))
        dbg_err_sif ((fp = fopen(t->outfn, "w")) == NULL);
    else
        fp = stdout;

    dbg_err_if (t->reporters.t_cb(fp, t, U_TEST_REP_HEAD));

    LIST_FOREACH (tso, &t->u_test_suites.objs, links)
    {
        ts = TS_HANDLE(tso);

        dbg_err_if (t->reporters.ts_cb(fp, ts, U_TEST_REP_HEAD));

        LIST_FOREACH (tco, &ts->u_test_cases.objs, links)
            dbg_err_if (t->reporters.tc_cb(fp, TC_HANDLE(tco)));

        dbg_err_if (t->reporters.ts_cb(fp, ts, U_TEST_REP_TAIL));
    }

    dbg_err_if (t->reporters.t_cb(fp, t, U_TEST_REP_TAIL));

    (void) fclose(fp);

    return 0;
err:
    return ~0;
}

static int u_test_report_txt (FILE *fp, u_test_t *t, u_test_rep_tag_t tag)
{
    u_test_syn_t *syn = &t->syn;

    if (tag == U_TEST_REP_HEAD)
    {
        (void) fprintf(fp, "%s", t->id);

        /* Host info. */
        (void) fprintf(fp, " (%s)\n", t->host);
    }

    if (tag == U_TEST_REP_TAIL)
    {
        (void) fprintf(fp, "Number of test suites: %u\n", syn->total);
        (void) fprintf(fp, "               Passed: %u\n", syn->pass);
        (void) fprintf(fp, "               Failed: %u\n", syn->fail);
        (void) fprintf(fp, "              Aborted: %u\n", syn->abrt);
        (void) fprintf(fp, "              Skipped: %u\n", syn->skip);
    }

    return 0;
}

static int u_test_suite_report_txt (FILE *fp, u_test_suite_t *ts, 
        u_test_rep_tag_t tag)
{
    int status;
    char b[80] = { '\0' }, e[80] = { '\0' }, d[80] = { '\0' };
    u_test_syn_t *syn = &ts->syn;

    dbg_return_if (fp == NULL, ~0);
    dbg_return_if (ts == NULL, ~0);

    if (tag == U_TEST_REP_HEAD)
    {
        status = ts->o.status;

        (void) fprintf(fp, "\t* [%s] %s\n", 
                u_test_status_str(status), ts->o.id);

        /* Timing info is relevant only in case test succeeded. */ 
        if (status == U_TEST_SUCCESS)
        {
            (void) u_test_obj_ts_fmt(&ts->o, b, e, d);
            (void) fprintf(fp, "\t       begin: %s\n", b);
            (void) fprintf(fp, "\t         end: %s\n", e);
            (void) fprintf(fp, "\t     elapsed: %s\n", d);
        }
    }

    if (tag == U_TEST_REP_TAIL)
    {
        (void) fprintf(fp, "\tNumber of test cases: %u\n", syn->total);
        (void) fprintf(fp, "\t              Passed: %u\n", syn->pass);
        (void) fprintf(fp, "\t              Failed: %u\n", syn->fail);
        (void) fprintf(fp, "\t             Aborted: %u\n", syn->abrt);
        (void) fprintf(fp, "\t             Skipped: %u\n", syn->skip);
    }

    return 0;
}

static int u_test_case_report_txt (FILE *fp, u_test_case_t *tc)
{
    int status;
#ifdef HAVE_STRUCT_RUSAGE
    char s[80] = { '\0' }, u[80] = { '\0' };
#endif  /* HAVE_STRUCT_RUSAGE */
    char d[80] = { '\0' };

    dbg_return_if (fp == NULL, ~0);
    dbg_return_if (tc == NULL, ~0);

    status = tc->o.status;

    (void) fprintf(fp, "\t\t* [%s] %s\n", u_test_status_str(status), tc->o.id);

    if (status == U_TEST_SUCCESS)
    {
#ifdef HAVE_STRUCT_RUSAGE
        if (tc->pts->pt->sandboxed)
        {
            (void) u_test_case_rusage_fmt(tc, u, s);
            (void) fprintf(fp, "\t\t    sys time: %s\n", s);
            (void) fprintf(fp, "\t\t   user time: %s\n", u);
        }
        else
#endif  /* HAVE_STRUCT_RUSAGE */
        {
            (void) u_test_obj_ts_fmt(&tc->o, NULL, NULL, d);
            (void) fprintf(fp, "\t\t     elapsed:%s\n", d);
        }
    }

    return 0;
}

static int u_test_report_xml (FILE *fp, u_test_t *t, u_test_rep_tag_t tag)
{
    u_test_syn_t *syn = &t->syn;

    dbg_return_if (t == NULL, ~0);
    dbg_return_if (fp == NULL, ~0);

    if (tag == U_TEST_REP_HEAD)
    {
        (void) fprintf(fp, "<?xml version=\"1.0\"?>\n");
        (void) fprintf(fp, "<test id=\"%s\">\n", t->id);

        /* Add synoptical info. */
        (void) fprintf(fp, "\t<total>%u</total>\n", syn->total);
        (void) fprintf(fp, "\t<passed>%u</passed>\n", syn->pass);
        (void) fprintf(fp, "\t<failed>%u</failed>\n", syn->fail);
        (void) fprintf(fp, "\t<aborted>%u</aborted>\n", syn->abrt);
        (void) fprintf(fp, "\t<skipped>%u</skipped>\n", syn->skip);

        /* Host info. */
        (void) fprintf(fp, "\t<host>%s</host>\n", t->host);
    }

    if (tag == U_TEST_REP_TAIL)
        (void) fprintf(fp, "</test>\n");

    return 0;
}

static int u_test_suite_report_xml (FILE *fp, u_test_suite_t *ts, 
        u_test_rep_tag_t tag)
{
    int status;
    u_test_syn_t *syn;

    dbg_return_if (fp == NULL, ~0);
    dbg_return_if (ts == NULL, ~0);

    syn = &ts->syn;

    if (tag == U_TEST_REP_HEAD)
    {
        status = ts->o.status;

        (void) fprintf(fp, "\t<test_suite id=\"%s\">\n", ts->o.id);

        /* Status first. */
        (void) fprintf(fp, "\t\t<status>%s</status>\n", 
                u_test_status_str(status));

        /* Then timings information. */
        if (status == U_TEST_SUCCESS)
        {
            char b[80], d[80], e[80];

            (void) u_test_obj_ts_fmt(&ts->o, b, e, d);

            (void) fprintf(fp, "\t\t<begin>%s</begin>\n", b);
            (void) fprintf(fp, "\t\t<end>%s</end>\n", e);
            (void) fprintf(fp, "\t\t<elapsed>%s</elapsed>\n", d);
        }

        /* Add synoptical info. */
        (void) fprintf(fp, "\t\t<total>%u</total>\n", syn->total);
        (void) fprintf(fp, "\t\t<passed>%u</passed>\n", syn->pass);
        (void) fprintf(fp, "\t\t<failed>%u</failed>\n", syn->fail);
        (void) fprintf(fp, "\t\t<aborted>%u</aborted>\n", syn->abrt);
        (void) fprintf(fp, "\t\t<skipped>%u</skipped>\n", syn->skip);
    }

    if (tag == U_TEST_REP_TAIL)
        (void) fprintf(fp, "\t</test_suite>\n");

    return 0;
}

static int u_test_case_report_xml (FILE *fp, u_test_case_t *tc)
{
    int status;

    dbg_return_if (fp == NULL, ~0);
    dbg_return_if (tc == NULL, ~0);

    status = tc->o.status;

    (void) fprintf(fp, "\t\t<test_case id=\"%s\">\n", tc->o.id);

    (void) fprintf(fp, "\t\t\t<status>%s</status>\n", 
            u_test_status_str(status));

    if (status == U_TEST_SUCCESS)
    {
        char d[80];
#ifdef HAVE_STRUCT_RUSAGE
        char u[80], s[80];

        /* When sandboxed we have rusage info. */
        if (tc->pts->pt->sandboxed)
        {
            (void) u_test_case_rusage_fmt(tc, u, s);
            (void) fprintf(fp, "\t\t\t<sys_time>%s</sys_time>\n", s);
            (void) fprintf(fp, "\t\t\t<user_time>%s</user_time>\n", u);
        }
        else
#endif  /* HAVE_STRUCT_RUSAGE */
        {
            (void) u_test_obj_ts_fmt(&tc->o, NULL, NULL, d);
            (void) fprintf(fp, "\t\t\t<elapsed>%s</elapsed>\n", d);
        }
    }

    (void) fprintf(fp, "\t\t</test_case>\n");

    return 0;
}

static const char *u_test_status_str (int status)
{
    switch (status)
    {
        case U_TEST_SUCCESS:  return "PASS";
        case U_TEST_FAILURE:  return "FAIL";
        case U_TEST_ABORTED:  return "ABRT";
        case U_TEST_SKIPPED:  return "SKIP";
    }

    return "?";
}

static int u_test_getopt (int ac, char *av[], u_test_t *t)
{
    int c, mp;

    dbg_return_if (t == NULL, ~0);

    while ((c = getopt(ac, av, "df:ho:p:sv")) != -1)
    {
        switch (c)
        {
            case 'o':   /* Report file. */
                (void) u_strlcpy(t->outfn, optarg, sizeof t->outfn);
                break;
            case 'f':   /* Report format. */
                if (u_test_set_outfmt(t, optarg))
                    u_test_usage(av[0], "bad report format");
                break;
            case 'v':   /* Increment verbosity. */
                g_verbose = 1;
                break;
            case 'd':   /* Print out some internal info. */
                g_debug = 1;
                break;
#ifdef U_TEST_SANDBOX_ENABLED
            case 'p':   /* Max number of test cases running in parallel. */
                if (u_atoi(optarg, &mp) || mp < 0)
                    u_test_usage(av[0], "bad max parallel");
                t->max_parallel = (unsigned int) mp;
                break;
            case 's':   /* Serialized (i.e. !sandboxed) test cases. */
                t->sandboxed = 0;
                break;
#endif  /* U_TEST_SANDBOX_ENABLED */
            case 'h': default:
                u_test_usage(av[0], NULL);
                break;
        } 
    }

    return 0;
}

static void u_test_usage (const char *prog, const char *msg)
{
    const char *us = 
        "                                                                   \n"
        "Synopsis: %s [options]                                             \n"
        "                                                                   \n"
        "   where \'options\' is a combination of the following:            \n"
        "                                                                   \n"
        "       -o <file>           Set the report output file              \n"
        "       -f <txt|xml>        Choose report output format             \n"
#ifdef U_TEST_SANDBOX_ENABLED
        "       -p <number>         Set the max number of parallel tests    \n"
        "       -s                  Serialize test cases (non sandboxed)    \n"
#endif  /* U_TEST_SANDBOX_ENABLED */
        "       -d                  Debug mode                              \n"
        "       -v                  Be chatty                               \n"
        "       -h                  Print this help                         \n"
        "                                                                   \n"
        ;
    
    if (msg)
        (void) fprintf(stderr, "\nError: %s\n", msg);

    (void) fprintf(stderr, us, prog ? prog : "unitest");

    exit(1);
}

static int u_test_set_outfmt (u_test_t *t, const char *fmt)
{
    dbg_return_if (t == NULL, ~0);
    dbg_return_if (fmt == NULL, ~0);

    if (!strcasecmp(fmt, "txt"))
        t->reporters = txt_reps;
    else if (!strcasecmp(fmt, "xml"))
        t->reporters = xml_reps;
    else
        return ~0;

    return 0;
}

static int u_test_init (u_test_t *t)
{
    dbg_return_if (t == NULL, ~0);

    dbg_err_if (u_test_signals());
    dbg_err_if (u_test_uname(t));

    return 0;
err:
    return ~0;
}

static int u_test_uname (u_test_t *t)
{
#ifdef HAVE_UNAME
    struct utsname h;
#endif  /* HAVE_UNAME */

    dbg_return_if (t == NULL, ~0);

#ifdef HAVE_UNAME
    dbg_return_sif (uname(&h) == -1, ~0);

    (void) u_snprintf(t->host, sizeof t->host, "%s - %s %s %s", 
            h.nodename, h.sysname, h.release, h.machine);
#else   
    /* TODO refine this ! */
    (void) u_strlcpy(t->host, "unknown host name", sizeof t->host);
#endif  /* HAVE_UNAME */

    return 0; 
}

static int u_test_signals (void)
{
#ifdef HAVE_SIGACTION
    struct sigaction sa;

    sa.sa_handler = u_test_interrupted;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;    /* Don't restart. */
        
    dbg_err_sif (sigaction(SIGINT, &sa, NULL) == -1);
    dbg_err_sif (sigaction(SIGTERM, &sa, NULL) == -1);
    dbg_err_sif (sigaction(SIGQUIT, &sa, NULL) == -1);
#else   /* !HAVE_SIGACTION */
    dbg_err_sif (signal(SIGINT, u_test_interrupted) == SIG_ERR);
    dbg_err_sif (signal(SIGTERM, u_test_interrupted) == SIG_ERR);
#endif  /* HAVE_SIGACTION */

    return 0;
err:
    return ~0;
}

static void u_test_interrupted (int signo)
{
    u_unused_args(signo);
    g_interrupted = 1;
}

#ifdef HAVE_STRUCT_RUSAGE
static int u_test_case_rusage_fmt (u_test_case_t *tc, char uti[80], 
        char sti[80])
{
    dbg_return_if (tc == NULL, ~0);

    if (uti)
        (void) __timeval_fmt(&tc->stats.ru_utime, uti);

    if (sti)
        (void) __timeval_fmt(&tc->stats.ru_stime, sti);

    /* TODO Other rusage fields depending on UNIX implementation. 
     * By POSIX we're only guaranteed about the existence of ru_utime and 
     * ru_stime fields. */

    return 0;
}

static int __timeval_fmt (struct timeval *tv, char t[80])
{
    char _tv[80];
    time_t secs;

    dbg_return_if (t == NULL, ~0);
    dbg_return_if (tv == NULL, ~0);

    secs = tv->tv_sec;

    dbg_if (strftime(_tv, sizeof _tv, "%M:%S", localtime(&secs)) == 0);
    dbg_if (u_snprintf(t, 80, "%s.%06.6d", _tv, tv->tv_usec));

    return 0;
}
#endif  /* HAVE_STRUCT_RUSAGE */

static int u_test_obj_ts_fmt (u_test_obj_t *to, char b[80], char e[80], 
        char d[80])
{
    time_t elapsed;
   
    dbg_return_if (to == NULL, ~0);

    elapsed = to->stop - to->start;

    if (b)
    {
        dbg_if (strftime(b, 80, "%a %Y-%m-%d %H:%M:%S %Z", 
                    localtime(&to->start)) == 0);
    }

    if (e)
    {
        dbg_if (strftime(e, 80, "%a %Y-%m-%d %H:%M:%S %Z", 
                    localtime(&to->stop)) == 0);
    }

    if (d)
        dbg_if (strftime(d, 80, "%M:%S", localtime(&elapsed)) == 0);

    return 0;
}

static void u_test_bail_out (TO *h)
{
    int status;
    u_test_obj_t *to;
    u_test_case_t *tc;

    CHAT("Bailing out, as requested by the user\n");
    
#ifdef U_TEST_SANDBOX_ENABLED
    /* Assume 'h' is a test cases' list head. */

    /* Brutally kill all (possibly) running test cases. 
     * There may be a harmless race here, in case a child has already
     * called exit(2), but we've been interrupted before reaping it. */
    LIST_FOREACH (to, &h->objs, links)
    {
        tc = TC_HANDLE(to);

        if (tc->pid != U_TEST_CASE_PID_INITIALIZER)
        {
            CHAT("Killing test case %s [%d]\n", to->id, (int) tc->pid);
            dbg_if (kill(tc->pid, SIGKILL) == -1);
        }
    }

    while (waitpid(-1, &status, 0) != -1) ;

#endif  /* U_TEST_SANDBOX_ENABLED */

    exit(1);
}
