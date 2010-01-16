#ifndef _LIBU_TEST_H_
#define _LIBU_TEST_H_
#include <u/libu.h>

/**
 *  \addtogroup test
 *  \{
 */ 

/** 
 *  \brief Define a test module
 *
 *  Defines a test module named \c name. 
 *
 *  \param name  module name
 */
#define U_TEST_MODULE( name )   \
    int u_test_run_ ## name (void)

/** 
 *  \brief Define a test function
 *
 *  Defines a test function.
 *
 *  \param  f   the function name
 */
#define U_TEST_RUN( f ) \
    if( f () ) { _test_cnt++; _test_fail++; u_con("%s: failed", #f); }  \
    else { _test_cnt++; _test_ok++; if(_verbose) u_con("%s: ok", #f); }

/** 
 *  \brief Import a test module in the test program
 *
 *  Import a test module to be use by the test program. 
 *
 *  \param  name    the name of the module
 */
#define U_TEST_MODULE_USE( name )                                   \
    do {                                                            \
        int u_test_run_ ## name (void);                             \
        *_top = u_test_run_ ## name; ++_top; *_top = NULL;          \
        *_top_nm = u_strdup( #name ); ++_top_nm; *_top_nm = NULL;   \
    } while(0)

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
