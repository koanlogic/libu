/* 
 * Copyright (c) 2005, KoanLogic s.r.l. - All rights reserved.  
 */

static const char rcsid[] =
    "$Id: misc.c,v 1.1 2005/09/23 13:04:38 tho Exp $";

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#include <u/misc.h>
#include <u/debug.h>
#include <u/alloc.h>

/**
 *  \defgroup misc Misc Utility Functions
 *  \{
 */

/** \brief Removes leading and trailing blanks (spaces and tabs) from \p s
 */
void u_trim(char *s)
{
    char *p;

    if(!s)
        return;

    /* trim trailing blanks */
    p = s + strlen(s) -1;
    while(s < p && isblank(*p))
        --p;
    p[1] = 0;

    /* trim leading blanks */
    p = s;
    while(*p && isblank(*p))
        ++p;

    if(p > s)
        memmove(s, p, 1 + strlen(p));
}

/** \brief Returns \c 0 if \p c is neither a space or a tab, not-zero otherwise.
 */
inline int u_isblank(int c)
{
    return c == ' ' || c == '\t';
}

/** \brief Returns \c 1 if \p ln is a blank string i.e. a string formed by 
           ONLY spaces and/or tabs characters.
 */
inline int u_isblank_str(const char *ln)
{
    for(; *ln; ++ln)
        if(!u_isblank(*ln))
            return 0;
    return 1;
}

/** \brief Returns \c 0 if \p c is neither a CR (\\r) or a LF (\\n), 
     not-zero otherwise.
 */
inline int u_isnl(int c)
{
    return c == '\n' || c == '\r';
}

/** \brief Dups the first \p len chars of \p s.
     Returns the dupped zero terminated string or \c NULL on error.
 */
char *u_strndup(const char *s, size_t len)
{
    char *cp;

    if((cp = (char*) u_malloc(len + 1)) == NULL)
        return NULL;
    memcpy(cp, s, len);
    cp[len] = 0;
    return cp;
}

/** \brief Dups the supplied string \p s */
char *u_strdup(const char *s)
{
    return u_strndup(s, strlen(s));
}

/** \brief Save the PID of the calling process to a file named \p pf 
     (that should be a fully qualified path).
     Returns \c 0 on success, not-zero on error.
 */
int u_savepid (const char *pf)
{
    FILE *pfp;

    dbg_return_if ((pfp = fopen(pf, "w")) == NULL, ~0);

    fprintf(pfp, "%ld\n", (long) getpid());
    fclose(pfp);

    return 0;
}

/** \brief  Safe string copy, see also the SSTRNCPY define 
  Safe string copy which null-terminates the destination string \a dst before
  copying the source string \a src for no more than \a size bytes.
  Returns a pointer to the destination string \a dst.
*/
char *u_sstrncpy (char *dst, const char *src, size_t size)
{
    dst[size] = '\0';
    return strncpy(dst, src, size);
}

/** \brief Dups the memory block \c src of size \c size.
     Returns the pointer of the dup'd block on success, \c NULL on error.
 */
void* u_memdup(void *src, size_t size)
{
    void *p;

    p = u_malloc(size);
    if(p)
        memcpy(p, src, size);
    return p;
}

/**
 *      \}
 */
