/*
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.
 */

#ifndef _U_TEST_H_
#define _U_TEST_H_

#include <u/libu_conf.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 *  \addtogroup test
 *  \{
 */ 

struct u_test_case_s;
struct u_test_suite_s;
struct u_test_s;

/** \brief  Carpal-like msg_err_if macro. */
#define u_test_err_if(a)        \
    do { if (a) { u_test_case_printf(tc, "%s", #a); goto err; } } while (0)

/** \brief  Carpal-like msg_err_ifm macro. */
#define u_test_err_ifm(a, ...)  \
    do { if (a) { u_test_case_printf(tc, __VA_ARGS__); goto err; } } while (0)

/** \brief  Test case handler. */
typedef struct u_test_case_s u_test_case_t;

/** \brief  Test suite handler. */
typedef struct u_test_suite_s u_test_suite_t;

/** \brief  Test handler. */
typedef struct u_test_s u_test_t;

/** \brief  Exit status of unit tests. */
enum { 
    U_TEST_SUCCESS = 0,
    /**< All test assertions got right. */

    U_TEST_FAILURE = 1,
    /**< Any test assertion has failed. */

    U_TEST_ABORTED = 2,
    /**< Catch any non-regular execution condition (e.g. SIGSEGV). */

    U_TEST_SKIPPED = 3
    /**< A previous dependency failure prevents test execution. */
};

/** \brief  Tags used to tell if the reporter routine has been called
 *          on element opening or closure. */
typedef enum { U_TEST_REP_HEAD, U_TEST_REP_TAIL } u_test_rep_tag_t;

/** \brief  Unit test function prototype. */
typedef int (*u_test_f)(u_test_case_t *);

/** \brief  Report functions' prototypes. */
typedef int (*u_test_rep_f)(FILE *, u_test_t *, u_test_rep_tag_t);
typedef int (*u_test_case_rep_f)(FILE *, u_test_case_t *);
typedef int (*u_test_suite_rep_f)(FILE *, u_test_suite_t *, u_test_rep_tag_t);

/** \brief  Maximum number of running test cases. */
#ifndef U_TEST_MAX_PARALLEL
#define U_TEST_MAX_PARALLEL   32
#endif  /* !U_TEST_MAX_PARALLEL */

/** \brief  Maximum length of a test suite/case identifier. */
#ifndef U_TEST_ID_MAX
#define U_TEST_ID_MAX     128
#endif  /* !U_TEST_ID_MAX */

/** \brief  Default test report file name. */
#ifndef U_TEST_OUTFN_DFL
#define U_TEST_OUTFN_DFL  "./unitest-report.out"
#endif  /* !U_TEST_OUTFN_DFL */

int u_test_case_new (const char *id, u_test_f func, u_test_case_t **ptc);
int u_test_case_add (u_test_case_t *tc, u_test_suite_t *ts);
void u_test_case_free (u_test_case_t *tc);
int u_test_case_register (const char *id, u_test_f func, u_test_suite_t *ts);
int u_test_case_dep_register (const char *id, u_test_case_t *tc);
int u_test_case_depends_on (const char *tcid, const char *depid, 
        u_test_suite_t *ts);
int u_test_case_printf (u_test_case_t *tc, const char *fmt, ...);

int u_test_suite_add (u_test_suite_t *to, u_test_t *t);
void u_test_suite_free (u_test_suite_t *ts);
int u_test_suite_new (const char *id, u_test_suite_t **pts);
int u_test_suite_dep_register (const char *id, u_test_suite_t *ts);
int u_test_suite_depends_on (const char *tsid, const char *depid, u_test_t *t);

int u_test_new (const char *id, u_test_t **pt);
int u_test_set_outfn (u_test_t *t, const char *outfn);
int u_test_set_u_test_suite_rep (u_test_t *t, u_test_suite_rep_f func);
int u_test_set_u_test_case_rep (u_test_t *t, u_test_case_rep_f func);
int u_test_set_u_test_rep (u_test_t *t, u_test_rep_f func);

void u_test_free (u_test_t *t);
int u_test_run (int ac, char *av[], u_test_t *t);

/**
 *  \}
 */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* !_U_TEST_H_ */
