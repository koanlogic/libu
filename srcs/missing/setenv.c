#ifndef HAVE_SETENV
#include <windows.h>

int setenv (const char *name, const char *value, int overwrite)
{
    return SetEnvironmentVariableA(name, value) == 0 ? -1 : 0;
}
#else
int setenv (const char *name, const char *value, int overwrite);
#endif
