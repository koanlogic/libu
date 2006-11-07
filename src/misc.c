/* 
 * Copyright (c) 2005, 2006 by KoanLogic s.r.l. - All rights reserved.  
 */

static const char rcsid[] =
    "$Id: misc.c,v 1.25 2006/11/07 11:40:32 tat Exp $";

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

/** \brief  Safe string copy, see also the U_SSTRNCPY define 
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
 * path separators from the resulting string.
 *
 * \param buf       destination buffer
 * \param sz        size of \p str
 * \param sep       path separator to use ('/' or '\')
 * \param fmt       snprintf format string
 *
 *   Returns \c 0 on success, not-zero on error.
 */
int u_path_snprintf(char *buf, size_t sz, char sep, const char *fmt, ...)
{
    va_list ap;
    int wr, i, len;

    va_start(ap, fmt);

    wr = vsnprintf(buf, sz, fmt, ap);

    va_end(ap);

    dbg_err_if(wr < 0 || wr >= (int)sz);

    /* remove multiple consecutive '/' */
    for(len = i = strlen(buf); i > 0; --i)
        if(buf[i] == sep && buf[i-1] == sep)
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

int u_data_dump (char *data, size_t sz, const char *file)
{
    FILE *fp = NULL;

    dbg_err_if ((fp = fopen(file, "w")) == NULL); 
    dbg_err_if (fwrite(data, sz, 1, fp) < 1);
    fclose(fp);

    return 0;
err:
    U_FCLOSE(fp); 
    return ~0;
}

/**
 *  \brief  Load file into memory from the supplied FS entry 
 *
 *  \param  path    the path name of the file 
 *  \param  sz_max  if >0 impose this size limit (otherwise unlimited)
 *  \param  pbuf    the pointer to the memory location holding the file's 
 *                  contents as a value-result argument
 *  \param  psz     size of the loaded file as a v-r argument
 *
 *  \return \c 0 on success, \c ~0 on error
 */ 
int u_load_file (const char *path, size_t sz_max, char **pbuf, size_t *psz)
{   
    FILE *fp = NULL;
    char *buf = NULL;
    size_t sz; 
    struct stat sb;
    
    dbg_return_if (path == NULL, ~0);
    dbg_return_if (pbuf == NULL, ~0);
    dbg_return_if (psz == NULL, ~0);
    
    warn_err_sif ((fp = fopen(path, "r")) == NULL);
    warn_err_sif (fstat(fileno(fp), &sb) == -1); 
    sz = sb.st_size;
    warn_err_ifm (sz_max > 0 && sz > sz_max, "file too big");
    warn_err_sif ((buf = u_zalloc(sz)) == NULL);
    warn_err_sif (fread(buf, sz, 1, fp) != 1);
    
    U_FCLOSE(fp);
    
    *pbuf = buf;
    *psz = sz;
    
    return 0;
err:
    U_FCLOSE(fp);
    U_FREE(buf);
    return ~0;
}

/**
 *  \brief  Return, in the given buffer, a string describing the error code
 *
 *  Return in 'msg' a string describing the error code. Works equally with 
 *  POSIX-style C libs and with glibc (that use a different prototype for 
 *  strerror_r).
 *
 *  If strerror_r doesn't exist in the system strerror() is used instead.
 *
 *  \param  en      the error code
 *  \param  msg     the buffer that will get the error message
 *  \param  sz      size of buf
 *
 *  \return \c 0 on success, \c ~0 on error
 */ 
int u_strerror_r(int en, char *msg, size_t sz)
{

#ifdef HAVE_STRERROR_R
    enum { BUFSZ = 256 };
    char buf[BUFSZ] = { 0 };
    int rc;

    /* assume POSIX prototype */
    rc = (int)strerror_r(en, buf, BUFSZ);

    if(rc == 0)
    {    /* posix version, success */
        strncpy(msg, buf, sz);
    } else if(rc < 1024) {
         /* posix version, failed (returns -1 or an error number) */
         goto err;
    } else {
        /* glibc char*-returning version, always succeeds */
        strncpy(msg, (char*)rc, sz);
    }
#else
    /* there's not strerror_r, use strerror() instead */
    char *p;

    dbg_err_if((p = strerror(en)) == NULL);

    strncpy(msg, p, sz);
#endif

    msg[sz - 1] = 0;

    return 0;
err:
    return ~0;
}

/**
 *      \}
 */
