/* 
 * Copyright (c) 2005-2010 by KoanLogic s.r.l. - All rights reserved.  
 */

#include <u/libu.h>

enum { MAX_MODS = 1024 };

test_runner_t _mods[MAX_MODS] = { NULL };
test_runner_t *_top = _mods;

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
        Tests are bundles in test modules defined by the ::U_TEST_MODULE macro; 
        each module contain one or more test functions; such functions will 
        be considered succesfull when returning \c 0. Function should be run
        from within the module main function wrapped by the ::U_TEST_RUN macro.
  
        Example: mymodule.c
    \code
    int test_feature_1(void)
    {
        dbg_err_if(...);

        return 0; 
    err:
        return ~0;
    }

    int test_feature_2(void)
    {
        dbg_err_if(...);

        return 0; 
    err:
        return ~0;
    }

    U_TEST_MODULE( mymodule )
    {
        U_TEST_RUN( test_feature_1 );
        U_TEST_RUN( test_feature_2 );
    
        return 0;                                                
    }
    \endcode

        To build a \c runtest executable define a \c main() function like 
        the example below and import your test modules using the 
        ::U_TEST_USE_MODULE macro. 
  
        Example: main.c
    \code
    #include <u/libu.h>

    int facility = LOG_LOCAL0;

    int main(int argc, char **argv)
    {
        U_TEST_USE_MODULE( mymodule );
        U_TEST_USE_MODULE( mymodule2 );
    
        return u_test_run(argc, argv);
    }
    \endcode

        To run all tests call \c runtest with no arguments. At the end of 
        execution it will print the number of tests run and the number of 
        tests that have failed.  Add the \c -v switch if you want a more
        verbose output.
    \code
    $ ./runtest [-v] # run testers of all the defined U_TEST_USE_MODULE's 
    \endcode

        If you want to run just selected modules add their name to the 
        \c runtest command line:
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
    test_runner_t *p;
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

