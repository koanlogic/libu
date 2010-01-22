/* 
 * Copyright (c) 2005-2010 by KoanLogic s.r.l. - All rights reserved.  
 */

#include <u/libu.h>

enum { MAX_MODS = 1024 };

u_test_runner_t _mods[MAX_MODS] = { NULL };
u_test_runner_t *_top = _mods;

char *_mods_nm[MAX_MODS] = { NULL };
char **_top_nm = _mods_nm;

int _test_cnt = 0;
int _test_fail = 0;
int _test_ok = 0;
int _verbose = 0;

static char **arg;
static int narg;

static int parse_opt(int argc, char **argv);
static void usage(const char *prog);
static int run_test_module(const char *module);

/**
    \defgroup test Unit testing
    \{
        The \ref test module offers a set of interfaces by means of which a 
        user can organise its own software validation suite.  
        Basically you define a test suite, which in turn comprises a set of
        test cases (aka unit tests), and recall it in the main test program.
        Each test case is associated to a ::u_test_runner_t routine which 
        takes no arguments and, by convention, is requested to return 
        ::U_TEST_EXIT_SUCCESS in case the unit test succeeds and 
        ::U_TEST_EXIT_FAILURE otherwise.
        A test case is attached to its "mother" test suite with the
        ::U_TEST_CASE_ADD macro; on the other hand, a test suite is attached 
        to the main test program with the ::U_TEST_SUITE_ADD macro, as shown
        in the following example:
    \code
    int test_case_1 (void)
    {
        dbg_err_if (...);

        return U_TEST_EXIT_SUCCESS; 
    err:
        return U_TEST_EXIT_FAILURE;
    }

    int test_case_2 (void)
    {
        dbg_err_if (...);

        return U_TEST_EXIT_SUCCESS; 
    err:
        return U_TEST_EXIT_FAILURE;
    }

    U_TEST_SUITE( mysuite1 )
    {
        U_TEST_CASE_ADD( test_case_1 );
        U_TEST_CASE_ADD( test_case_2 );
        return 0;
    }
    \endcode

        To build the main test program (let's call it \c runtest) you define a 
        \c main function like the one in the example below, import your test 
        suite using the ::U_TEST_SUITE_ADD macro, and then call the 
        ::u_test_run function to execute all tests one after another:
    \code
    #include <u/libu.h>

    int facility = LOG_LOCAL0;

    int main (int argc, char **argv)
    {
        U_TEST_SUITE_ADD( mysuite1 );
        U_TEST_SUITE_ADD( mysuite2 );
    
        return u_test_run(argc, argv);
    }
    \endcode

        To run every included test suite, call \c runtest with no arguments. 
        At the end of execution it will print the number of tests run and 
        the number of tests that have failed.  Add the \c -v switch if you 
        want a more verbose output.
    \code
    $ ./runtest [-v] # run all the included test suites
    \endcode

        If you want to run just selected suites, add the name used when 
        ::U_TEST_CASE_ADD'ing them on the \c runtest command line, e.g.:
    \code
    $ ./runtest mymodule # just run 'mymodule' tests
    $ ./runtest mymodule othermod # run 'mymodule' and 'othermod' tests
    \endcode
 */

/** 
 *  \brief Run tests
 *
 *  Run tests. It shall be called by the \c main() function and will return 
 *  when all tests have been run.
 *
 *  \param  argc    \c main() \c argc argument
 *  \param  argv    \c main() \c argv argument
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_test_run(int argc, char **argv)
{
    u_test_runner_t *p;
    char **pn;
    int i;

    dbg_err_if(parse_opt(argc, argv));

    if(narg)
    {
        /* run only user provided modules */
        for(i = 0; i < narg; ++i)
            dbg_err_if(run_test_module(arg[i]));
    } else {
        /* run all modules */
        for(p = _mods; p < _top; ++p)
            (*p)();
    }

    /* free strdup'd module names */
    for(pn = _mods_nm; pn != _top_nm; ++pn)
        u_free(*pn), *pn = NULL;

    u_con("%d test run, %d failed", _test_cnt, _test_fail);

    return 0;
err:
    return ~0;
}

/**
 *  \}
 */

static void usage(const char *prog)
{
    static const char *us = 
        "usage: %s OPTIONS [ MODULE ... ]\n"
        "\n"
        "    -h          display this help   \n"
        "    -v          be verbose          \n"
        "\n"
        "    Available modules:\n";
    char **p;

    u_con(us, prog);

    for(p = _mods_nm; p != _top_nm; ++p)
        u_con("        %s", *p);

    exit(1);
}

static int parse_opt(int argc, char **argv)
{
    int ret;

    while((ret = getopt(argc, argv, "vh")) != -1)
    {
        switch(ret)
        {
        case 'v': 
            _verbose = 1;
            break;
        default:
        case 'h': 
            usage(argv[0]);
        }
    }

    narg = argc - optind;
    arg = argv + optind;

    return 0;
}

static int run_test_module(const char *module)
{
    char **m;
    int i;

    for(i = 0, m = _mods_nm; m < _top_nm; ++m, ++i)
    {
        if(!strcasecmp(*m, module))
        {
            _mods[i]();  /* run module tests */
            return 0;
        }
    }

    con_return_ifm (1, ~0, "unknown module %s", module);
}

