#include <u/missing/tempnam.h>

#ifndef HAVE_TEMPNAM

char *tempnam (const char *dir, const char *pfx)
{
    /* TODO impl */
    return NULL;
}

#else   /* HAVE_TEMPNAM */
char *tempnam (const char *dir, const char *pfx);
#endif  /* !HAVE_TEMPNAM */
