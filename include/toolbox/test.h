#ifndef _LIBU_TEST_H_
#define _LIBU_TEST_H_
#include <u/libu.h>

/**
 *  \addtogroup test
 *  \{
 */ 

/** 
 *  \brief Define and/or declare a test suite
 *
 *  Define and/or declare a test suite named \c name. 
 *
 *  \param name  module name
 */
#define U_TEST_SUITE( name )   \
    int u_test_run_ ## name (void)

/** 
 *  \brief Add a test case function
 *
 *  Add the test case function \p f to the test pool
 *
 *  \param  f   a function with ::test_runner_t prototype
 */
#define U_TEST_CASE_ADD( f ) \
    if( f () ) { _test_cnt++; _test_fail++; u_con("[FAILED] %s", #f); }  \
    else { _test_cnt++; _test_ok++; if(_verbose) u_con("[OK] %s", #f); }

/** 
 *  \brief Import a test suite in the test program
 *
 *  Import a test suite that will be run by the test program. 
 *
 *  \param  name    the name of the test suite
 */
#define U_TEST_SUITE_ADD( name )                                    \
    do {                                                            \
        int u_test_run_ ## name (void);                             \
        *_top = u_test_run_ ## name; ++_top; *_top = NULL;          \
        *_top_nm = u_strdup( #name ); ++_top_nm; *_top_nm = NULL;   \
    } while(0)

/* 1.x compat names */
#define U_TEST_MODULE_USE   U_TEST_SUITE_ADD
#define U_TEST_RUN          U_TEST_CASE_ADD
#define U_TEST_MODULE       U_TEST_SUITE

int u_test_run(int argc, char **argv);

/** \brief  The prototype for a test function.  The function must return
 *          \c 0 on success, \c ~0 on error. */
typedef int (*test_runner_t)(void);

/** \brief  The array which holds all the test functions (INTERNAL) */
extern test_runner_t _mods[], *_top;

/** \brief  The array which holds all modules names (INTERNAL) */
extern char *_mods_nm[], **_top_nm;

/** \brief  Various internal counters */
extern int _test_cnt, _test_ok, _test_fail;

/** \brief  If set, via <code>runtest -v</code>, more info about the running
 *          tests are printed to standard error */
extern int _verbose;

/**
 *  \}
 */

#endif
