#ifndef _TEST__H_
#define _TEST__H_

struct test_case_s;
typedef struct test_case_s test_case_t;

struct test_suite_s;
typedef struct test_suite_s test_suite_t;

struct test_s;
typedef struct test_s test_t;

typedef enum { TEST_CASE_T, TEST_SUITE_T } test_what_t;

typedef int (*test_f)(test_case_t *);

int test_case_new (const char *id, test_f func, test_case_t **ptc);
int test_case_add (test_case_t *tc, test_suite_t *ts);
void test_case_free (test_case_t *tc);
int test_case_register (const char *id, test_f func, test_suite_t *ts);
int test_case_dep_register (const char *id, test_case_t *tc);
int test_case_depends_on (const char *tcid, const char *depid, 
        test_suite_t *ts);

int test_suite_add (test_suite_t *to, test_t *t);
void test_suite_free (test_suite_t *ts);
int test_suite_new (const char *id, test_suite_t **pts);
int test_suite_dep_register (const char *id, test_suite_t *ts);
int test_suite_depends_on (const char *tsid, const char *depid, test_t *t);

int test_new (const char *id, test_t **pt);
void test_free (test_t *t);
int test_run(test_t *t);

#endif  /* !_TEST__H_ */
