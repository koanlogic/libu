#include "test.h"

enum { MAX_MODS = 1024 };

test_runner_t mods[MAX_MODS] = { NULL };
test_runner_t *top = mods;

char *mods_nm[MAX_MODS] = { NULL };
char **top_nm = mods_nm;

int test_cnt = 0;
int test_fail = 0;
int test_ok = 0;
int verbose = 0;

static char **arg;
static int narg;

static void usage()
{
    static const char *us = 
        "usage: runtest OPTIONS [ MODULE ... ]\n"
        "\n"
        "    -h          display this help   \n"
        "    -v          be verbose          \n"
        "\n"
        "    Available modules:\n";
    char **p;

    fprintf(stderr, us);

    for(p = mods_nm; p != top_nm; ++p)
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
            verbose = 1;
            break;
        default:
        case 'h': 
            usage();
        }
    }

    narg = argc - optind;
    arg = argv + optind;

    return 0;
err:
    return ~0;
}

static int run_test_module(const char *module)
{
    char **m;
    int i;

    for(i = 0, m = mods_nm; m < top_nm; ++m, ++i)
    {
        if(!strcasecmp(*m, module))
        {
            mods[i]();  /* run module tests */
            return 0;
        }
    }

    con_err("unknown module %s", module);

    return 0;
err:
    return ~0;
}

int run_tests(int argc, char **argv)
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
        for(i = 0, p = mods; p < top; ++p, ++i)
            (*p)();
    }

    printf("%d test run, %d failed\n", test_cnt, test_fail);

    return 0;
err:
    return ~0;
}


