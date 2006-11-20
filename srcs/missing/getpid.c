#include <u/libu_conf.h>
#ifndef HAVE_GETPID

pid_t getpid()
{
    #ifdef OS_WIN
    return GetCurrentProcessId();
    #else
    #error "unimplemented"
    #endif
}
#else
#include <sys/types.h>
#include <unistd.h>
pid_t getpid(void);
#endif 
