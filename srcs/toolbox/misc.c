/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.  
 */

#include <u/libu_conf.h>
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
#include <sys/types.h>

#include <toolbox/misc.h>
#include <toolbox/carpal.h>
#include <toolbox/memory.h>

/**
    \defgroup misc Miscellaneous
    \{
        
        

 */

/** 
 *  \brief  Tell if the supplied char is a space or tab
 *
 *  Tell if the supplied char is a space or tab 
 *
 *  \param  c   the character to be tested
 *
 *  \retval 1   if \p c is whitespace or tab character
 *  \retval 0   if \p c is neither a whitespace nor tab character
 */
inline int u_isblank (int c)
{
    return c == ' ' || c == '\t';
}

/** 
 *  \brief  Tell if the supplied string is blank
 *  
 *  Tell if the supplied string is composed of tabs and/or spaces only
 *
 *  \param  s   a NUL-terminated string that is is being queried
 *
 *  \retval 1   if \p s is blank
 *  \retval 0   if \p s contains non-blank characters 
 */
inline int u_isblank_str (const char *s)
{
    for ( ; *s; ++s)
    {
        if (!u_isblank(*s))
            return 0;
    }

    return 1;
}

/** 
 *  \brief  Tell if the supplied char is a new-line character
 *
 *  Tell if the supplied char is a new-line character, i.e. CR (\c \\r) or
 *  LF (\c \\n)
 *
 *  \param  c   the character to be tested
 *
 *  \retval 1   if \p c is CR or LF
 *  \retval 0   if \p c is neither CR nor LF
 */
inline int u_isnl (int c)
{
    return c == '\n' || c == '\r';
}

/** 
 *  \brief  Tell if the supplied buffer contains non-ASCII bytes 
 *
 *  Tell if the supplied buffer \p data contains non-ASCII bytes in the first
 *  \p sz bytes 
 *
 *  \param  data    the data buffer that shall be examined
 *  \param  sz      the number of bytes that will be taken into account
 *
 *  \retval 1   if there is at least one non-ASCII character
 *  \retval 0   if it \p data contains only ASCII characters
 */
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
 *  \brief Removes leading and trailing blanks (spaces and tabs) from a string
 *
 *  Remove leading and trailing blanks (spaces and tabs) from the supplied 
 *  string \p s
 *
 *  \param  s   a NUL-terminated string
 *
 *  return nothing 
 */
void u_trim (char *s)
{
    char *p;

    nop_return_if (s == NULL, );

    /* trim trailing blanks */
    p = s + strlen(s) - 1;
    while (s < p && u_isblank(*p))
        --p;
    p[1] = '\0';

    /* trim leading blanks */
    p = s;
    while (*p && u_isblank(*p))
        ++p;

    if (p > s)
        memmove(s, p, 1 + strlen(p));

    return;
}

/** 
 *  \brief  Make a copy of a portion of a string
 *
 *  Return a NUL-terminated string which duplicates the first \p len characters
 *  of the supplied string \p s.  The returned pointer may subsequently be used
 *  as an argument to ::u_free.
 *
 *  \param  s       a NUL-terminated string
 *  \param  len     the length of the substring which has to be copied
 *
 *  \return the duplicated substring, or \c NULL on error (i.e. insufficient
 *          memory or invalid \p len)
 */
char *u_strndup (const char *s, size_t len)
{
    char *cp;

    /* check bounds */
    dbg_return_if (len > strlen(s), NULL);

    /* make room for the substring + \0 */
    dbg_return_sif ((cp = u_malloc(len + 1)) == NULL, NULL);

    memcpy(cp, s, len);
    cp[len] = '\0';

    return cp;
}

/** 
 *  \brief  Allocate sufficient memory for a copy of the supplied string, copy 
 *          it, and return a reference to the copy
 *
 *  Allocate sufficient memory for a copy of the supplied string \p s, copy it,
 *  and return its reference. The returned pointer may subsequently be used
 *  as an argument to ::u_free.
 *
 *  \param  s   a NUL-terminated string
 *
 *  \return the duplicated string or \c NULL in case no sufficient memory is
 *          available
 */
char *u_strdup (const char *s)
{
    return u_strndup(s, strlen(s));
}

/** 
 *  \brief  Wrapper to strlcpy(3) 
 * 
 *  Wrapper to strlcpy(3) that will check whether \p src is too big to fit 
 *  \p dst.  In case of overflow, at least \p size bytes will be copied anyway
 *  from \p src to \p dst.
 * 
 *  \param  dst     buffer of at least \p size bytes where bytes from \p src 
 *                  will be copied
 *  \param  src     NUL-terminated string that is (possibly) copied to \p dst
 *  \param  size    full size of the destination buffer \p dst
 *  
 *  \retval  0  copy is ok
 *  \retval ~0  copy would overflow the destination buffer
 */
inline int u_strlcpy(char *dst, const char *src, size_t size)
{
    return (strlcpy(dst, src, size) >= size ? ~0 : 0);
}

/** 
 *  \brief  Wrapper to strlcat(3) 
 * 
 *  Wrapper to strlcat(3) that will check whether \p src is too big to fit 
 *  \p dst.  In any case at least \p size bytes of \p src will be concatenated
 *  to \p dst. 
 *
 *  \param  dst     NUL-terminated string to which \p src will be concatenated
 *  \param  src     NUL-terminated string that is (possibly) contatenated to 
 *                  \p dst
 *  \param  size    full size of the destination buffer \p dst
 *  
 *  \retval  0  string concatenation is ok
 *  \retval ~0  string concatenation would overflow the destination buffer
 */
inline int u_strlcat(char *dst, const char *src, size_t size)
{
    return (strlcat(dst, src, size) >= size ? ~0 : 0);
}

/**
 *  \brief   Break a string into pieces separated by a given set of characters
 *
 *  Tokenize the \p delim separated string \p s and place its \p *pnelems 
 *  pieces into the string array \p *ptv.  The \p *ptv result argument contains
 *  all the pieces at increasing indices [0..\p *pnelems-1] in the order 
 *  they've been found in string \p s.  At index \p *pnelems - i.e. 
 *  \p *ptv[\p *pnelems] - the pointer to a (clobbered) duplicate of \p s is 
 *  stored, which must be free'd along with the string array result \p *ptv 
 *  once the caller is done with it:
 * 
 *  \code
 *      size_t nelems, i;
 *      char **tv = NULL;
 *          
 *      // break string into pieces
 *      dbg_err_if (u_strtok("white spaces separated s", " ", &tv, &nelems));
 *
 *      // use found tokens
 *      for (i = 0; i < nelems; i++)
 *          con("%s", tv[i]);
 *
 *      // free memory
 *      u_strtok_cleanup(tv, nelems);
 *  \endcode
 *
 *  The aforementioned disposal must be carried out every time the function 
 *  returns successfully, even if the number of found tokens is zero (i.e. \p s
 *  contains separator chars only, or is an empty string).
 *
 *  \param  s       the string that shall be broken up
 *  \param  delim   the set of allowed delimiters as a string
 *  \param  ptv     the pieces as a result string array
 *  \param  pnelems the number of tokens actually separated
 *
 *  \retval  0  on success
 *  \retval ~0  on error
 */ 
int u_strtok (const char *s, const char *delim, char ***ptv, size_t *pnelems)
{
    enum { TV_NSLOTS_DFLT = 10, TV_NSLOTS_INCR = 10 };
    char **ap, **tv = NULL, **ntv = NULL;
    size_t nelems, tv_slots = TV_NSLOTS_DFLT;
    char *p, *sdup = NULL;

    dbg_return_if (s == NULL, ~0);
    dbg_return_if (delim == NULL, ~0);
    dbg_return_if (ptv == NULL, ~0);
    dbg_return_if (pnelems == NULL, ~0);
    
    sdup = u_strdup(s);
    dbg_err_if (sdup == NULL);

    p = sdup;
    ap = tv;
    nelems = 0;

expand:
    ntv = u_realloc(tv, tv_slots * sizeof(char *));
    dbg_err_sif (ntv == NULL);

    /* recalc 'ap' in case realloc has changed the base pointer */
    if (ntv != tv)
        ap = ntv + (ap - tv);

    tv = ntv;

    for (; (*ap = strsep(&p, delim)) != NULL; )
    {
        /* skip empty field */
        if (**ap == '\0')
            continue;

        ++nelems;

        /* when we reach the last slot, request new slots */
        if (++ap == &tv[tv_slots - 1])
        {
            tv_slots += TV_NSLOTS_INCR;
            goto expand;
        }
    }

    /* save the dup'd string on the last slot */
    *ap = sdup;

    /* copy out */
    *ptv = tv;
    *pnelems = nelems;

    return 0;
err:
    U_FREE(sdup);
    U_FREE(tv);
    return ~0;
}

/**
 *  \brief  Cleanup the strings vector created by ::u_strtok
 *
 *  Cleanup the strings vector created by ::u_strtok
 *
 *  \param   tv     the strings vector created by a previous call to ::u_strtok
 *  \param   nelems number of elements in \p tv (as returned by ::u_strtok)
 *
 *  \return nothing
 */
void u_strtok_cleanup (char **tv, size_t nelems)
{
    if (tv)
    {
        U_FREE(tv[nelems]);
        u_free(tv);
    }

    return;
}

/**
 *  \brief  snprintf(3) wrapper 
 *
 *  snprintf(3) wrapper
 *
 *  \param  str     destination buffer
 *  \param  size    size of \p str
 *  \param  fmt     printf(3) format string
 *
 *  \retval 0   on success
 *  \retval ~0  on failure, i.e. if an output error has occurred, or the
 *              destination buffer would have been overflowed
 */
int u_snprintf (char *str, size_t size, const char *fmt, ...)
{
    int wr;
    va_list ap;

    va_start(ap, fmt);
    wr = vsnprintf(str, size, fmt, ap);
    va_end(ap);

    dbg_err_sif (wr < 0);               /* output error */
    dbg_err_if (size <= (size_t) wr);   /* overflow */

    /* was: "dbg_err_if (wr >= (int) size);" bad cast ? */

    return 0;
err:
    return ~0;
}

/**
 *  \brief  snprintf(3)-like function that handles path name building
 *
 *  Call snprintf(3) with the provided arguments and remove consecutive
 *  path separators from the resulting string
 *
 *  \param buf  destination buffer
 *  \param sz   size of \p str
 *  \param sep  path separator to use (\c / or \c \)
 *  \param fmt  printf(3) format string
 *
 *  \retval 0   on success
 *  \retval ~0  on failure
 */
int u_path_snprintf (char *buf, size_t sz, char sep, const char *fmt, ...)
{
    va_list ap;
    int wr, i, len;

    va_start(ap, fmt);
    wr = vsnprintf(buf, sz, fmt, ap);
    va_end(ap);

    dbg_err_sif (wr < 0);
    dbg_err_if (sz <= (size_t) wr);

    /* remove multiple consecutive '/' */
    for (len = i = strlen(buf); i > 0; --i)
    {
        if (buf[i] == sep && buf[i - 1] == sep)
        {
            memmove(buf + i, buf + i + 1, len - i);
            len--;
        }
    }

    return 0;
err:
    return ~0;
}

/** 
 *  \brief  Save the process id of the calling process to file
 *
 *  Save the process id of the calling process to the supplied file \p pf
 *
 *  \param  pf  path of the file (be it fully qualified or relative to the 
 *              current working directory) where the pid will be saved
 *
 *  \retval  0  on success
 *  \retval ~0  on error
 */
int u_savepid (const char *pf)
{
    FILE *pfp = NULL;

    dbg_return_if (pf == NULL, ~0);

    dbg_err_sif ((pfp = fopen(pf, "w")) == NULL);
    dbg_err_sif (fprintf(pfp, "%ld\n", (long) getpid()) == 0);
    fclose(pfp);

    return 0;
err:
    U_FCLOSE(pfp);
    return ~0;
}

/** 
 *  \brief  Make a duplicate of a memory block
 *
 *  Duplicate the memory block starting at \c src of size \c size
 *
 *  \param  src     reference to the block that must be copied
 *  \param  size    number of bytes that must be copied 
 *
 *  \return the reference to the duplicated block of memory on success, 
 *          \c NULL on error.
 */
void *u_memdup (const void *src, size_t size)
{
    void *p;

    dbg_return_sif ((p = u_malloc(size)) == NULL, NULL);

    memcpy(p, src, size);

    return p;
}

/**
 *  \brief  Consume an arbitrary number of variable names
 *
 *  Consume an arbitrary number of variable names: it is used to silent the
 *  GNU C compiler when invoked with \c -Wunused, and you know that a given
 *  variable won't be never used.
 *
 *  \param  dummy   the first of a possibly long list of meaningless parameters
 *
 *  \return nothing
 *
 */ 
inline void u_use_unused_args (char *dummy, ...)
{
    dummy = 0;
    return;
}


/** 
 *  \brief  Save some data in memory to file
 *
 *  Save some memory starting from \p data for \p sz bytes to the supplied
 *  \p file
 *
 *  \param  data    pointer to the data to be saved
 *  \param  sz      size in bytes of \p data
 *  \param  file    the path of the file where data will be saved
 *
 *  \retval  0  on success
 *  \retval ~0  on error
 */
int u_data_dump (char *data, size_t sz, const char *file)
{
    FILE *fp = NULL;

    dbg_err_sif ((fp = fopen(file, "w")) == NULL); 
    dbg_err_sif (fwrite(data, sz, 1, fp) < 1);
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
 *  \retval  0  on success
 *  \retval ~0  on error
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
    
    dbg_err_sifm ((fp = fopen(path, "r")) == NULL, "%s", path);
    dbg_err_sifm (fstat(fileno(fp), &sb) == -1, "%s", path); 
    sz = sb.st_size;
    dbg_err_ifm (sz_max > 0 && sz > sz_max, 
            "file too big (%zu vs %zu bytes)", sz, sz_max);
    dbg_err_sif ((buf = u_zalloc(sz)) == NULL);
    dbg_err_sifm (fread(buf, sz, 1, fp) != 1, "%s", path);
    
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
 *  \brief  Top level I/O routine 
 *
 *  Try to read/write - atomically - a chunk of \p l bytes from/to the object 
 *  referenced by the descriptor \p sd.  The data chunk is written to/read from
 *  the buffer starting at \p buf.  An I/O driver function \p f is used to 
 *  carry out the job whose interface and behaviour must conform to those of 
 *  \c POSIX.1 \c read(2) or \c write(2).  If \p n is not \c NULL, it will store
 *  the number of bytes actually read/written: this information is significant
 *  only when ::u_io fails.  If \p eof is not \c NULL, it will be set to \c 1 
 *  on an EOF condition.
 *
 *  \param f        the I/O function, i.e. \c read(2) or \c write(2)
 *  \param sd       the file descriptor on which the I/O operation is performed
 *  \param buf      the data chunk to be read or written
 *  \param l        the length in bytes of \p buf
 *  \param n        the number of bytes read/written as a value-result arg
 *  \param eof      true if end-of-file condition
 *   
 *  \retval  0  is returned on success
 *  \retval ~0  is returned if an error other than \c EINTR has occurred, or 
 *              if the requested amount of data could not be entirely 
 *              read/written.
 */
int u_io (iof_t f, int sd, void *buf, size_t l, ssize_t *n, int *eof)
{
#define SET_PPTR(pptr, val) do {if ((pptr)) *(pptr) = (val);} while (0);
    ssize_t nret;
    size_t nleft = l;
    char *p = buf;

    SET_PPTR(n, 0);
    SET_PPTR(eof, 0);

    while (nleft > 0) 
    {
        if ((nret = (f) (sd, p, nleft)) == -1)
        {
            if (errno == EINTR)
                continue;
            else
            {
                dbg_strerror(errno);
                goto end;
            }
        }

        /* test EOF */
        if (nret == 0)
        {
            SET_PPTR(eof, 1);
            goto end;
        }
        
        nleft -= nret;
        p += nret;
    }

end:
    SET_PPTR(n, l - nleft);
    return nleft ? ~0 : 0;
#undef SET_PPTR
}

/**
 *  \brief  An ::u_io wrapper that uses \c read(2) as I/O driver
 *
 *  An ::u_io wrapper that uses \c read(2) as I/O driver: its behaviour
 *  is identical to \c read(2) except it automatically restarts when 
 *  interrupted
 *
 *  \param  fd      an open file descriptor
 *  \param  buf     buffer to read into
 *  \param  size    size of \p buf in bytes
 *
 *  \return on success the number of read bytes (that will always 
 *          be \p size except that on EOF) is returned; on failure return \c -1
 */
ssize_t u_read (int fd, void *buf, size_t size)
{
    int eof = 0;
    ssize_t nw = 0;

    if (u_io((iof_t) read, fd, buf, size, &nw, &eof))
    {
        if (eof)
            return nw; /* may be zero */
        else
            return -1;
    }

    return nw;
}

/**
 *  \brief  An ::u_io wrapper that uses \c write(2) as I/O driver
 *
 *  An ::u_io wrapper that uses \c write(2) as I/O driver: its behaviour
 *  is identical to \c write(2) except it automatically restarts when 
 *  interrupted
 *
 *  \param  fd      an open file descriptor
 *  \param  buf     buffer to write
 *  \param  size    size of \p buf in bytes
 *
 *  \retval size    on success
 *  \retval -1      on error
 */ 
ssize_t u_write (int fd, void *buf, size_t size)
{
    if (u_io((iof_t) write, fd, buf, size, NULL, NULL))
        return -1;

    return size;
}

/**
 *  \brief  sleep(3) wrapper that handles EINTR
 *
 *  sleep(3) wrapper that automatically restarts when interrupted by a signal
 *
 *  \param  secs    number of seconds to sleep
 *
 *  \retval  0  on success
 *  \retval -1  on failure
 */ 
int u_sleep(unsigned int secs)
{
#ifdef OS_WIN
    Sleep(secs * 1000);
#else
    int sleep_for, c;

    for (sleep_for = secs; sleep_for > 0; sleep_for = c)
    {
        if ((c = sleep(sleep_for)) == 0)
            break;
        else if (errno != EINTR)
            return -1; /* should never happen */
    }
#endif
    return 0;
}

/** 
 *  \brief  Convert string to \c long \c int representation
 *
 *  Try to convert the string \p nptr into a long integer at \p pl, handling 
 *  all \c strtol(3) exceptions (overflow or underflow, invalid representation)
 *  in a simple go/no-go way.
 *
 *  \param  nptr    NUL-terminated string (possibly) representing an integer
 *                  value (base 10)
 *  \param  pl      pointer to a \c long \c int variable that contains the 
 *                  result of a successful conversion
 *  
 *  \retval 0   on success
 *  \retval ~0  on error
 */
int u_atol (const char *nptr, long *pl)
{
    char *endptr;
    long tmp, saved_errno = errno;

    dbg_return_if (nptr == NULL, ~0);
    dbg_return_if (pl == NULL, ~0);
 
    tmp = strtol(nptr, &endptr, 10);
    
    dbg_err_sif (tmp == 0 && errno == EINVAL);
    dbg_err_sif ((tmp == LONG_MIN || tmp == LONG_MAX) && errno == ERANGE);

    /* check if no valid digit string was supplied
     * glibc does not handle this as an explicit error (would return
     * 0 with errno unset) */
    dbg_err_ifm (nptr == endptr, "invalid base10 string: %s", nptr);

    *pl = tmp;
    
    errno = saved_errno;
    return 0;
err:
    errno = saved_errno;
    return ~0;
}

/** 
 *  \brief  Convert string to \c int representation
 *
 *  Try to convert the string \p nptr into an integer at \p pi, handling all 
 *  \c strtol(3) exceptions (overflow or underflow, invalid representation)
 *  in a simple go/no-go way.
 *
 *  \param  nptr    NUL-terminated string (possibly) representing an integer
 *                  value (base 10)
 *  \param  pi      pointer to an integer variable that contains the result
 *                  of a successful conversion
 *  
 *  \retval 0   on success
 *  \retval ~0  on error
 */
int u_atoi (const char *nptr, int *pi)
{
    long int tmp;

    dbg_err_if (u_atol(nptr, &tmp));

    /* check overflows/underflows when int bits are less than long bits */
#if (INT_MAX < LONG_MAX) 
    dbg_err_if (tmp > INT_MAX);
#endif
#if (INT_MIN > LONG_MIN)
    dbg_err_if (tmp < INT_MIN);
#endif
        
    *pi = (int) tmp;
    
    return 0;
err:
    return ~0;
}

#ifdef HAVE_STRTOUMAX
/** 
 *  \brief  Convert string to \c u_intmax_t representation
 *
 *  Try to convert the string \p nptr into a maximum unsigned integer at 
 *  \p pumax, handling all \c strtoumax(3) exceptions (overflow or underflow, 
 *  invalid representation) in a simple go/no-go way.
 *
 *  \param  nptr    NUL-terminated string (possibly) representing an unsigned
 *                  integer value (base 10)
 *  \param  pumax   pointer to an \c u_intmax_t variable that contains the 
 *                  result of a successful conversion
 *  
 *  \retval 0   on success
 *  \retval ~0  on error
 */
int u_atoumax (const char *nptr, uintmax_t *pumax)
{
    uintmax_t tmp; 
    int saved_errno = errno;

    dbg_return_if (nptr == NULL, ~0);
    dbg_return_if (pumax == NULL, ~0);
 
    tmp = strtoumax(nptr, (char **) NULL, 10);
    
    dbg_err_if (tmp == 0 && errno == EINVAL);
    dbg_err_if (tmp == UINTMAX_MAX && errno == ERANGE);
        
    *pumax = tmp;
    
    errno = saved_errno;
    return 0;
err:
    errno = saved_errno;
    return ~0;
}
#endif  /* HAVE_STRTOUMAX */

/** 
 *  \brief  Convert string to \c double representation
 *
 *  Try to convert the string \p nptr into a double precision FP number at 
 *  \p pl, handling all \c strtod(3) exceptions (overflow or underflow, invalid
 *  representation) in a simple go/no-go way.
 *  Beware that if your platform does not implements the strtod(3) interface,
 *  this function falls back using plain old atof(3) which does have severe 
 *  limitations as of error handling and thread safety.
 *
 *  \param  nptr    NUL-terminated string (possibly) representing an floating
 *                  point number (see strtod(3) for formatting details)
 *  \param  pd      pointer to a \c double variable that contains the result of
 *                  a successful conversion
 *  
 *  \retval 0   on success  (always if \c HAVE_STRTOD is not defined)
 *  \retval ~0  on error
 */
int u_atof (const char *nptr, double *pd)
{
#ifdef HAVE_STRTOD
    double tmp;
    char *endptr;
    int saved_errno = errno;

    dbg_return_if (nptr == NULL, ~0);
    dbg_return_if (pd == NULL, ~0);

    /* Reset errno. */
    errno = 0;  
 
    /* Try conversion. */
    tmp = strtod(nptr, &endptr);
    
    /* No conversion performed */
    dbg_err_if (tmp == 0 && nptr == endptr);

    /* Overflow/underflow conditions.
     * 'tmp' can be 0 or [+-]HUGE_VAL, but we're not interested in 
     * distinguishing. */
    dbg_err_sif (errno == ERANGE);

    *pd = tmp;
 
    errno = saved_errno;
    return 0;
err:
    errno = saved_errno;
    return ~0;
#else
    /* Fall back to old plain atof(3). */
    dbg_return_if (nptr == NULL, ~0);
    dbg_return_if (pd == NULL, ~0);

    *pd = atof(nptr);

    return 0;
#endif  /* HAVE_STRTOD */
}

/**
 *  \brief  tokenize the supplied \p wlist string (<b>DEPRECATED</b>, use 
 *          ::u_strtok instead)
 *
 *  Tokenize the \p delim separated string \p wlist and place its
 *  pieces (at most \p tokv_sz - 1) into \p tokv.
 *
 *  \note <b>DEPRECATED</b>, use ::u_strtok instead
 *
 *  \param wlist    list of strings possibily separated by chars in \p delim 
 *  \param delim    set of token separators
 *  \param tokv     pre-allocated string array
 *  \param tokv_sz  number of cells in \p tokv array
 *
 *  \retval  0  on success
 *  \retval ~0  on error
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
 *  \brief  Safe string copy (<b>DEPRECATED</b>, use ::u_strlcpy instead)
 *
 *  Safe string copy which NUL-terminates the destination string \a dst before
 *  copying the source string \a src for no more than \a size bytes
 *
 *  \note <b>DEPRECATED</b>, use ::u_strlcpy instead
 *
 *  \param  dst     buffer of at least \p size bytes where \p src will be copied
 *  \param  src     NUL-terminated string that is (possibly) copied to \p dst
 *  \param  size    full size of the destination buffer \p dst
 *
 *  \return a pointer to the destination string \a dst
 */
char *u_sstrncpy (char *dst, const char *src, size_t size)
{
    dst[size] = '\0';
    return strncpy(dst, src, size);
}

/**
 *      \}
 */
