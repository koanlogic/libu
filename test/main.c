#include <u/libu.h>

int facility = LOG_LOCAL0;

int main(int argc, char **argv)
{
    U_TEST_MODULE_USE(misc);
    U_TEST_MODULE_USE(hmap);

    return u_test_run(argc, argv);
}
