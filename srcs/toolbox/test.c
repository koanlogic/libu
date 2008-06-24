/* 
 * Copyright (c) 2005-2008 by KoanLogic s.r.l. - All rights reserved.  
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

    fprintf(stderr, us, prog);

    for(p = _mods_nm; p != _top_nm; ++p)
        fprintf(stderr, "        %s\n", *p);
    fprintf(stderr, "\n");

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

int u_test_run(int argc, char **argv)
{
    test_runner_t *p;
    int i;

    dbg_err_if(parse_opt(argc, argv));

    if(narg)
    {
        /* run only user provided modules */
        for(i = 0; i < narg; ++i)
            dbg_err_if(run_test_module(arg[i]));
    } else {
        /* run all modules */
        for(i = 0, p = _mods; p < _top; ++p, ++i)
            (*p)();
    }

    printf("%d test run, %d failed\n", _test_cnt, _test_fail);

    return 0;
err:
    return ~0;
}


