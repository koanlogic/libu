/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.  
 */

#include <stdlib.h>
#include <errno.h>
#include <toolbox/str.h>
#include <toolbox/misc.h>
#include <toolbox/carpal.h>
#include <toolbox/memory.h>

enum { BLOCK_SIZE = 64 };

/* null strings will be bound to the null char* */
static char null_cstr[1] = { 0 };
static char *null = null_cstr;

/* internal string struct */
struct u_string_s
{
    char *data;
    size_t data_sz;     /* total malloc'c bytes starting from .data */
    size_t data_len;    /* strlen(3) of the NUL-terminated string */
    size_t shift_cnt;   /* (internal) used by the realloc algorithm */
};

static int u_string_do_vprintf (u_string_t *, int, const char *, va_list);

/**
    \defgroup string String
    \{
        The \ref string module provides a string type and the set of associated
        primitives that can be used instead of (or in combination with) the 
        classic NUL-terminated C strings.
        The convenience in using ::u_string_t objects in your code is that
        you can forget the buffer length constraints (with their implied 
        checks) and string explicit termination that come with C char arrays, 
        since any needed buffer resizing and consistency is handled by the 
        \ref string machinery transparently.

        Anyway, a bunch of C code lines is better than any further word:
    \code
    u_string_t *s = NULL;

    // create and initialize a new string object 's'
    dbg_err_if (u_string_create("pi", strlen("pi"), &s));

    // append the value of pi to 's'
    dbg_err_if (u_string_aprintf(s, " = %.30f", M_PI));

    // print it out (should give: "pi = 3.141592653589793115997963468544")
    u_con("%s", u_string_c(s));

    // clear the string
    dbg_err_if (u_string_clear(s));

    // go again with other stuff
    dbg_err_if (u_string_set(s, "2^(1/2)", strlen("2^(1/2)")));

    // append the value of 2^1/2 to 's'
    dbg_err_if (u_string_aprintf(s, " = %.30f", M_SQRT2));

    // print it out (should give: "2^(1/2) = 1.414213562373095145474621858739")
    u_con("%s", u_string_c(s));

    // when your done, dispose it
    u_string_free(s);
    \endcode
*/

/**
 *  \brief  Detach the NUL-terminated C string associated to the supplied 
 *          ::u_string_t object
 *
 *  Detach the NUL-terminated C string embedded into the supplied ::u_string_t 
 *  object.  The original string object is then free'd and can't be accessed
 *  anymore.
 *
 *  \param  s   an ::u_string_t object
 *
 *  \retval a NUL-terminated C string
 */
char *u_string_detach_cstr (u_string_t *s)
{
    char *tmp;

    dbg_return_if (s == NULL, NULL);

    tmp = s->data;

    u_free(s);

    return tmp;
}

/**
 *  \brief  Remove leading and trailing blanks
 *
 *  Remove leading and trailing blanks from the given string
 *
 *  \param  s   an ::u_string_t object
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_string_trim (u_string_t *s)
{
    dbg_return_if (s == NULL, ~0);

    if (s->data_len > 0)
    {
        u_trim(s->data);
        s->data_len = strlen(s->data);
    }

    return 0;
}

/**
 *  \brief  Set the length of a string (shortening it)
 *
 *  Set the length of the supplied string \p s to \p len
 *
 *  \param  s   an ::u_string_t object
 *  \param  len new (shorter) length of \p s
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_string_set_length (u_string_t *s, size_t len)
{
    dbg_return_if (s == NULL, ~0);
    dbg_return_if (len > s->data_len, ~0);

    if (len < s->data_len)
    {
        s->data_len = len;
        s->data[len] = '\0';
    }

    /* if (len == s->data_len) do nothing */ 

    return 0;
}

/**
 *  \brief  Return the string length
 *
 *  Return the length of the given string \p s
 *
 *  \param  s   an ::u_string_t object
 *
 *  \return the string length
 */
inline size_t u_string_len (u_string_t *s)
{
    dbg_return_if (s == NULL, 0);

    return s->data_len;
}

/**
 *  \brief  Return the string value
 *
 *  Return the NUL-terminated C string handled by the given ::u_string_t 
 *  object \p s.  Such const value cannot be modified, realloc'd nor free'd.
 *
 *  \param  s   an ::u_string_t object
 *
 *  \return the string value or \c NULL if the string is empty
 */
inline const char *u_string_c (u_string_t *s)
{
    dbg_return_if (s == NULL, NULL);

    return s->data;
}

/**
 *  \brief  Copy the value of a string to another
 *
 *  Copy \p src string to \p dst string.  Both \p src and \p dst must be
 *  already TODO...
 *
 *  \param  dst destination ::u_string_t object
 *  \param  src source ::u_string_t object
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
inline int u_string_copy (u_string_t *dst, u_string_t *src)
{
    dbg_return_if (dst == NULL, ~0);
    dbg_return_if (src == NULL, ~0);

    (void) u_string_clear(dst);

    return u_string_append(dst, src->data, src->data_len);
}

/**
 *  \brief  Clear a string
 *
 *  Erase the content of the given ::u_string_t object \p s
 *
 *  \param  s   an ::u_string_t object
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_string_clear (u_string_t *s)
{
    dbg_return_if (s == NULL, ~0);

    /* clear the string but do not free the buffer */
    if (s->data_sz)
    {
        s->data[0] = '\0';
        s->data_len = 0;
    }

    return 0;
}

/**
 *  \brief  Create a new string
 *
 *  Create a new ::u_string_t object and save its reference to \p *ps.
 *  If \p buf is not \c NULL (and <code>len > 0</code>), the string will be 
 *  initialized with the content of \p buf
 *
 *  \param  buf initial string value, or \c NULL
 *  \param  len length of \p buf
 *  \param  ps  result argument that will get the newly created ::u_string_t
 *              object on success
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_string_create (const char *buf, size_t len, u_string_t **ps)
{
    u_string_t *s = NULL;

    dbg_return_if (ps == NULL, ~0);

    dbg_err_sif ((s = u_malloc(sizeof(u_string_t))) == NULL);

    s->data_sz = 0;
    s->data_len = 0;
    s->shift_cnt = 0;
    s->data = null;

    if (buf)
        dbg_err_if (u_string_append(s, buf, len));

    *ps = s;

    return 0;
err:
    if (s)
        u_string_free(s);
    return ~0;
}

/**
 *  \brief  Free a string
 *
 *  Release all resources allocated to the supplied ::u_string_t object \p s 
 *
 *  \param  s   the ::u_string_t object that will be destroyed
 *
 *  \retval  0  always
 */
int u_string_free (u_string_t *s)
{
    if (s)
    {
        if (s->data_sz)
            U_FREE(s->data);
        u_free(s);
    }

    return 0;
}

/**
 *  \brief  Set the value of a string
 *
 *  Set the value of the supplied \p s to \p buf
 *
 *  \param  s   an ::u_string_t object
 *  \param  buf the value that will be assigned to \p s
 *  \param  len length of \p buf
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_string_set (u_string_t *s, const char *buf, size_t len)
{
    dbg_return_if (s == NULL, ~0);

    (void) u_string_clear(s);

    return u_string_append(s, buf, len);
}

/**
 *  \brief  Enlarge the underlying memory block of the given buffer
 *
 *  Enlarge the buffer data block of the supplied ::u_string_t object \p s 
 *  to (at least) \p size bytes.
 *
 *  \param  s       an ::u_string_t object
 *  \param  size    the requested new size
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_string_reserve (u_string_t *s, size_t size)
{
    char *nbuf = NULL;

    dbg_return_if (s == NULL, ~0);
    nop_return_if (size <= s->data_sz, 0);
   
    /* realloc requested size plus 1 char to store the terminating '\0' */
    nbuf = u_realloc(((s->data == null) ? NULL : s->data), size + 1);
    dbg_err_sif (nbuf == NULL);

    /* zero terminate it */
    nbuf[size] = '\0';

    s->data = (void *) nbuf;
    s->data_sz = size;

    return 0;
err:
    return ~0;
}

/**
 *  \brief  Set or append a printf-style formatted string to the given string
 *
 *  Set or append (depending on \p clear value) the printf-style format string 
 *  \p fmt to the given ::u_string_t object \p s
 *
 *  \param  s     an ::u_string_t object
 *  \param  clear \c 1 to set, \c 0 to append
 *  \param  fmt   printf-style format
 *  \param  ...   variable list of arguments that feed \p fmt
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_string_do_printf (u_string_t *s, int clear, const char *fmt, ...)
{
    int rc;
    va_list ap;

    va_start(ap, fmt);
    rc = u_string_do_vprintf(s, clear, fmt, ap);
    va_end(ap);

    return rc;
}

/**
 *  \brief  Append a NUL-terminated string to a string object
 *
 *  Append the NUL-terminated string \p buf to the ::u_string_t object \p s
 *
 *  \param  s   an ::u_string_t object
 *  \param  buf NUL-terminated string that will be appended to \p s
 *  \param  len length of \p buf
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_string_append (u_string_t *s, const char *buf, size_t len)
{
    size_t nsz, min;

    dbg_return_if (s == NULL, ~0);
    dbg_return_if (buf == NULL, ~0);
    nop_return_if (len == 0, 0);    /* nothing to do */

    /* if there's not enough space on s->data alloc a bigger buffer */
    if (s->data_len + len + 1 > s->data_sz)
    {
        min = s->data_len + len + 1; /* min required buffer length */
        nsz = s->data_sz;
        while (nsz <= min)
            nsz += (BLOCK_SIZE << s->shift_cnt++);

        dbg_err_if (u_string_reserve(s, nsz));
    }

    /* append this chunk to the data buffer */
    strncpy(s->data + s->data_len, buf, len);
    s->data_len += len;
    s->data[s->data_len] = '\0';
    
    return 0;
err:
    return ~0;
}

/**
 *  \brief  Set the string from printf-style arguments
 *
 *  Set an ::u_string_t object \p s from the printf-style arguments \p fmt
 *
 *  \param  s   an ::u_string_t object
 *  \param  fmt printf-style format
 *  \param  ... variable list of arguments that feed \p fmt
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_string_sprintf (u_string_t *s, const char *fmt, ...)
{
    int rc;
    va_list ap;

    va_start(ap, fmt);
    rc = u_string_do_vprintf(s, 1, fmt, ap);
    va_end(ap);

    return rc;
}

/**
 *  \brief  Append to the string the supplied printf-style arguments
 *
 *  Append the format string \p fmt to the supplied ::u_string_t object \p s 
 *
 *  \param  s   an ::u_string_t object
 *  \param  fmt printf-style format
 *  \param  ... variable list of arguments that feed \p fmt
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_string_aprintf (u_string_t *s, const char *fmt, ...)
{
    int rc;
    va_list ap;

    va_start(ap, fmt);
    rc = u_string_do_vprintf(s, 0, fmt, ap);
    va_end(ap);

    return rc;
}

/**
 *      \}
 */

static int u_string_do_vprintf (u_string_t *s, int clear, const char *fmt, 
        va_list ap)
{
    size_t sz, avail;
    va_list apcpy;

    dbg_return_if (s == NULL, ~0);
    dbg_return_if (fmt == NULL, ~0);

    if (clear)
        (void) u_string_clear(s);

again:
    avail = s->data_sz - s->data_len; /* avail may be zero */

    /* copy ap to avoid using it twice when realloc'ing */
    u_va_copy(apcpy, ap); 

    /* write to the internal buffer of the string.
     * if the return value is greater than or equal to the size argument, 
     * the string was too short and some of the printed characters were 
     * discarded; in that case realloc a bigger buffer and retry */
    sz = vsnprintf(s->data + s->data_len, avail, fmt, apcpy);

    va_end(apcpy);

    if (sz >= avail)
    {
        dbg_err_if (u_string_reserve(s, s->data_sz + s->data_len + 2 * sz + 1));

        /* trunc the buffer again since vsnprintf could have overwritten '\0' */
        s->data[s->data_len] = '\0';

        goto again;
    }

    /* update string length */
    s->data_len += sz; 

    return 0;
err:
    return ~0;
}
