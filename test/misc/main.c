#include <u/libu.h>
#include "test.h"

int facility = LOG_LOCAL0;

int main(int argc, char **argv)
{
    IMPORT_TEST_MODULE(misc);
    IMPORT_TEST_MODULE(hmap);

    return run_tests(argc, argv);
}
