#include <u/libu.h>

int facility = LOG_LOCAL0;

int main(int argc, char **argv)
{
    U_TEST_MODULE_USE(misc);
    U_TEST_MODULE_USE(string);
#ifndef NO_HMAP
    U_TEST_MODULE_USE(hmap);
#endif
#ifndef NO_LIST
    U_TEST_MODULE_USE(list);
#endif
#ifndef NO_ARRAY
    U_TEST_MODULE_USE(array);
#endif
#ifndef NO_NET
    U_TEST_MODULE_USE(uri);
#endif

    return u_test_run(argc, argv);
}
