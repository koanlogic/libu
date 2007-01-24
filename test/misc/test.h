#ifndef _LIBU_TEST_H_
#define _LIBU_TEST_H_
#include <u/libu.h>

#define RUN_TEST( f ) \
    if( f () ) { test_cnt++; test_fail++; con("%s: failed", #f); } \
    else { test_cnt++; test_ok++; if(verbose) con("%s: ok", #f);}

/* internal */
typedef int (*test_runner_t)(void);

extern test_runner_t mods[], *top;
extern char *mods_nm[], **top_nm;
extern int test_cnt, test_ok, test_fail;
extern int verbose;

#define TEST_MODULE( name ) int run_tests_ ## name (void)

#define IMPORT_TEST_MODULE( name ) \
    do {    \
        int run_tests_ ## name (void); \
        *top = run_tests_ ## name; ++top; *top = NULL; \
        *top_nm = u_strdup( #name ); ++top_nm; *top_nm = NULL; \
    } while(0)

int run_tests(int, char **);

#endif
