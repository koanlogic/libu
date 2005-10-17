#include "conf.h"

#ifndef HAVE_UNLINK

int unlink(const char *pathname)
{
    #if OS_WIN
    return DeleteFile(pathname) ? 0 /* success */: -1 /* failure */;
    #else
    #error "unimplemented"
    #endif
}

#else
#include <unistd.h>
int unlink(const char *);
#endif

