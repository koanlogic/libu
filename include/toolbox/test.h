/* 
 * Copyright (c) 2005-2010 by KoanLogic s.r.l. - All rights reserved.  
 */

#ifndef _LIBU_TEST_H_
#define _LIBU_TEST_H_

#include <u/libu_conf.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

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
 *  \param  f   a function with ::u_test_runner_t prototype
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

/* carpal additions for tests */
#define u_test_err_if(expr)             msg_err_if(u_test_, expr)
#define u_test_err_ifm(expr, ...)       msg_err_ifm(u_test_, expr, __VA_ARGS__)
#define u_test_( err, ...)              u_test_err_write( err, __VA_ARGS__)
#define u_test_err_write(err, ...)  \
    (printf("[%s:%d:%s] ", __FILE__, __LINE__, __FUNCTION__),   \
     printf(__VA_ARGS__), printf("\n"))

/* 1.x compat names */
#define U_TEST_MODULE_USE   U_TEST_SUITE_ADD
#define U_TEST_RUN          U_TEST_CASE_ADD
#define U_TEST_MODULE       U_TEST_SUITE

int u_test_run(int argc, char **argv);

/** \brief  Return code for ::u_test_runner_t functions */
typedef enum
{
    U_TEST_EXIT_SUCCESS = 0,    /**< test case succeeded */
    U_TEST_EXIT_FAILURE = 1     /**< test case failed */
} u_test_ret_t;

/** \brief  The prototype for a test function.  The function must return
 *          ::U_TEST_EXIT_SUCCESS on success, ::U_TEST_EXIT_FAILURE on error */
typedef int (*u_test_runner_t)(void);

extern u_test_runner_t _mods[], *_top;
extern char *_mods_nm[], **_top_nm;
extern int _test_cnt, _test_ok, _test_fail;
extern int _verbose;

/**
 *  \}
 */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* !_LIBU_TEST_H_ */
