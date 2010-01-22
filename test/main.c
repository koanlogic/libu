#include <u/libu.h>

int facility = LOG_LOCAL0;

int main(int argc, char **argv)
{
    U_TEST_SUITE_ADD(misc);
    U_TEST_SUITE_ADD(string);
#ifndef NO_HMAP
    U_TEST_SUITE_ADD(hmap);
#endif
#ifndef NO_LIST
    U_TEST_SUITE_ADD(list);
#endif
#ifndef NO_ARRAY
    U_TEST_SUITE_ADD(array);
#endif
#ifndef NO_NET
    U_TEST_SUITE_ADD(uri);
#endif
#ifndef NO_PQUEUE
    U_TEST_SUITE_ADD(pqueue);
#endif

    return u_test_run(argc, argv);
}
