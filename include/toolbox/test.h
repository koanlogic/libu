#ifndef _LIBU_TEST_H_
#define _LIBU_TEST_H_
#include <u/libu.h>

/**
 *  \defgroup test Unit testing
 *  \{
 *
 *      Tests are bundles in test modules defined by the TEST_MODULE macro; 
 *      each module contain one or more test functions; such functions will 
 *      be considered succesfull when returning 0. Function should be run
 *      from within the module main function wrapped by the RUN_TEST macro.
 *      
 *
 *      Example: mymodule.c
 *      \code
 * 
 *          int test_feature_1(void)
 *          {
 *              dbg_err_if(...)
 *
 *              return 0; 
 *          err:
 *              return ~0;
 *          }
 *
 *          int test_feature_2(void)
 *          {
 *              dbg_err_if(
 *
 *              return 0; 
 *          err:
 *              return ~0;
 *          }
 *
 *          TEST_MODULE( mymodule )
 *          {
 *              RUN_TEST( test_feature_1 );
 *              RUN_TEST( test_feature_2 );
 *          
 *              return 0;                                                
 *          }
 *      \endcode
 * 
 *
 *      To build the 'runtest' executable define a main() function like 
 *      the example below and import your test modules using the 
 *      IMPORT_TEST_MODULE macro. 
 *
 *      Example: main.c
 *      \code
 *          #include <u/libu.h>
 *
 *          int facility = LOG_LOCAL0;
 *
 *          int main(int argc, char **argv)
 *          {
 *              IMPORT_TEST_MODULE( mymodule );
 *              IMPORT_TEST_MODULE( mymodule2 );
 *          
 *              return run_tests(argc, argv);
 *          }
 *      \endcode
 *
 *
 *      To run all tests call the built executable with no arguments. At the
 *      end of execution it will print the number of tests run and the number
 *      ot tests failed.
 *          \verbatim
 *          ./runtest # runs all modules' test
 *          \endverbatim
 *
 *      If you want to run just selected modules add their name to the 'runtest'
 *      command line:
 *          \verbatim
 *          ./runtest mymodule # runs just 'mymodule' tests
 *          ./runtest mymodule othermod # runs 'mymodule' and 'othermod' tests
 *          \endverbatim
 */

/** \brief Define a test module
 *
 * Defines a test module named \c name. 
 *
 * \param name  module name
 */
#define TEST_MODULE( name ) \
    int run_tests_ ## name (void)

/** \brief Define a test function
 *
 * Defines a test function.
 *
 * \param f     the function name
 *
 * \return \c 0 on success, not zero on failure (i.e. test failed)
 */
#define RUN_TEST( f ) \
    if( f () ) { test_cnt++; test_fail++; con("%s: failed", #f); } \
    else { test_cnt++; test_ok++; if(verbose) con("%s: ok", #f); }


/** \brief Import a test module in the test program
 *
 * Import a test module to be use by the test program. 
 *
 * \param name     the name of the module
 *
 */
#define IMPORT_TEST_MODULE( name ) \
    do {    \
        int run_tests_ ## name (void); \
        *top = run_tests_ ## name; ++top; *top = NULL; \
        *top_nm = u_strdup( #name ); ++top_nm; *top_nm = NULL; \
    } while(0)

/** \brief Run tests
 *
 *  Run tests. Must be called by the main() function. Returns when all tests
 *  have been run.
 *
 * \param argc  main() argc argument
 * \param argv  main() argv argument
 *
 * \return \c 0 on success, not zero on failure
 */
int run_tests(int argc, char **argv);

/** */
typedef int (*test_runner_t)(void);
/** */
extern test_runner_t mods[], *top;
/** */
extern char *mods_nm[], **top_nm;
/** */
extern int test_cnt, test_ok, test_fail;
/** */
extern int verbose;

/**
 *  \}
 */

#endif
