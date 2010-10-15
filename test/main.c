#include <u/libu.h>

int facility = LOG_LOCAL0;

int test_suite_misc_register (u_test_t *t);
int test_suite_string_register (u_test_t *t);
int test_suite_hmap_register (u_test_t *t);
int test_suite_list_register (u_test_t *t);
int test_suite_array_register (u_test_t *t);
int test_suite_uri_register (u_test_t *t);
int test_suite_pqueue_register (u_test_t *t);
int test_suite_rb_register (u_test_t *t);
int test_suite_pwd_register (u_test_t *t);
int test_suite_json_register (u_test_t *t);
int test_suite_lexer_register (u_test_t *t);
int test_suite_bst_register (u_test_t *t);

int main(int argc, char **argv)
{
    int rc;
    u_test_t *t = NULL;

    con_err_if (u_test_new("LibU Unit Tests", &t));

    con_err_if (test_suite_misc_register(t));
    con_err_if (test_suite_string_register(t));
    con_err_if (test_suite_lexer_register(t));

#ifndef NO_ARRAY
    con_err_if (test_suite_array_register(t));
#endif  /* !NO_ARRAY */
#ifndef NO_LIST
    con_err_if (test_suite_list_register(t));
#endif  /* !NO_LIST */
#ifndef NO_NET
    con_err_if (test_suite_uri_register(t));
#endif  /* !NO_NET */
#ifndef NO_RB
    con_err_if (test_suite_rb_register(t));
#endif  /* !NO_RB */
#ifndef NO_PWD
    con_err_if (test_suite_pwd_register(t));
#endif  /* !NO_PWD */
#ifndef NO_HMAP
    con_err_if (test_suite_hmap_register(t));
#endif  /* !NO_HMAP */
#ifndef NO_PQUEUE
    con_err_if (test_suite_pqueue_register(t));
#endif  /* !NO_PQUEUE */
#ifndef NO_BST
    con_err_if (test_suite_bst_register(t));
#endif  /* !NO_BST */
#ifndef NO_JSON
    con_err_if (test_suite_json_register(t));
#endif  /* !NO_JSON */

    rc = u_test_run(argc, argv, t);
    u_test_free(t);

    return rc;
err:
    u_test_free(t);
    return EXIT_FAILURE;
}
