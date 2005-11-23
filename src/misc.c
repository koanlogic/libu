/* 
 * Copyright (c) 2005, KoanLogic s.r.l. - All rights reserved.  
 */

static const char rcsid[] =
    "$Id: misc.c,v 1.13 2005/11/23 20:15:59 tat Exp $";

#include "libu_conf.h"
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
#include <u/carpal.h>
#include <u/memory.h>

/**
 *  \defgroup misc Miscellaneous
 *  \{
 */

/** \brief Returns \c 0 if \p c is neither a space or a tab, not-zero otherwise.
 */
inline int u_isblank(int c)
{
    return c == ' ' || c == '\t';
}

/** \brief Removes leading and trailing blanks (spaces and tabs) from \p s
 */
void u_trim(char *s)
{
    char *p;

    if(!s)
        return;

    /* trim trailing blanks */
    p = s + strlen(s) -1;
    while(s < p && u_isblank(*p))
        --p;
    p[1] = 0;

    /* trim leading blanks */
    p = s;
    while(*p && u_isblank(*p))
        ++p;

    if(p > s)
        memmove(s, p, 1 + strlen(p));
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
void* u_memdup(const void *src, size_t size)
{
    void *p;

    p = u_malloc(size);
    if(p)
        memcpy(p, src, size);
    return p;
}

/**
 * \brief   tokenize the supplied \p wlist string
 *
 * Tokenize the \p delim separated string \p wlist and place its
 * pieces (at most \p tokv_sz - 1) into \p tokv.
 *
 * \param wlist     list of strings possibily separated by chars in \p delim 
 * \param delim     set of token separators
 * \param tokv      pre-allocated string array
 * \param tokv_sz   number of cells in \p tokv array
 *
 */
int u_tokenize (char *wlist, const char *delim, char **tokv, size_t tokv_sz)
{
    char **ap;

    dbg_return_if (wlist == NULL, ~0);
    dbg_return_if (delim == NULL, ~0);
    dbg_return_if (tokv == NULL, ~0);
    dbg_return_if (tokv_sz == 0, ~0);

    ap = tokv;

    for ( ; (*ap = strsep(&wlist, delim)) != NULL; )
    {
        /* skip empty field */
        if (**ap == '\0')
            continue;

        /* check bounds */
        if (++ap >= &tokv[tokv_sz - 1])
            break;
    }

    /* put an explicit stopper to tokv */
    *ap = NULL;

    return 0;
}

/**
 * \brief   snprintf-like function that returns 0 on success and ~0 on error
 *
 * snprintf-like function that returns 0 on success and ~0 on error
 *
 * \param str       destination buffer
 * \param size      size of \p str
 * \param fmt       snprintf format string
 *
 *   Returns \c 0 on success, not-zero on error.
 */
int u_snprintf(char *str, size_t size, const char *fmt, ...)
{
    va_list ap;
    int wr;

    va_start(ap, fmt);

    wr = vsnprintf(str, size, fmt, ap);

    va_end(ap);

    dbg_err_if(wr < 0 || wr >= (int)size);

    return 0;
err:
    return ~0;
}

/**
 * \brief   snprintf-like function that handle path separator issues
 *
 * Calls snprintf with the provided arguments and remove consecutive
 * U_PATH_SEPARATOR from the resulting string.
 *
 * \param buf       destination buffer
 * \param sz        size of \p str
 * \param fmt       snprintf format string
 *
 *   Returns \c 0 on success, not-zero on error.
 */
int u_path_snprintf(char *buf, size_t sz, const char *fmt, ...)
{
    va_list ap;
    int wr, i, len;

    va_start(ap, fmt);

    wr = vsnprintf(buf, sz, fmt, ap);

    va_end(ap);

    dbg_err_if(wr < 0 || wr >= (int)sz);

    /* remove multiple consecutive '/' */
    for(len = i = strlen(buf); i > 0; --i)
        if(buf[i] == (char) U_PATH_SEPARATOR &&
           buf[i-1] == (char) U_PATH_SEPARATOR)
            memmove(buf + i, buf + i + 1, len--);

    return 0;
err:
    return ~0;
}

inline void u_use_unused_args(char *dummy, ...)
{
    dummy = 0;
    return;
}

/** \brief  Return \c 1 if the supplied buffer \p data has non-ascii bytes */
int u_data_is_bin (char *data, size_t sz)
{
    size_t i;

    for (i = 0; i < sz; i++)
    {
        if (!isascii(data[i]))
            return 1;
    }

    return 0;
}

/**
 *      \}
 */
