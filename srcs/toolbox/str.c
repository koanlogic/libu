/* 
 * Copyright (c) 2005-2007 by KoanLogic s.r.l. - All rights reserved.  
 */

static const char rcsid[] =
    "$Id: str.c,v 1.2 2007/02/12 08:32:27 tho Exp $";

#include <stdlib.h>
#include <errno.h>

#include <toolbox/str.h>
#include <toolbox/misc.h>
#include <toolbox/carpal.h>
#include <toolbox/memory.h>

/**
 *  \defgroup string String
 *  \{
 */

/* null strings will be bound to the null char* */
static char null_cstr[1] = { 0 };
static char* null = null_cstr;

/* internal string struct */
struct u_string_s
{
    char *data;
    size_t data_sz, data_len, shift_cnt;
};

/**
 * \brief  Remove leading and trailing blanks
 *
 * Remove leading and trailing blanks from the given string
 *
 * \param s     string object
 *
 * \return \c 0 on success, not zero on failure
 */
int u_string_trim(u_string_t *s)
{
    if(s->data_len)
    {
        u_trim(s->data);

        s->data_len = strlen(s->data);
    }

    return 0;
}

/**
 * \brief  Set the length of a string (shortening it)
 * 
 *
 * \param s     string object
 * \param len   on success \a s will be \a len chars long
 *
 * \return \c 0 on success, not zero on failure
 */
int u_string_set_length(u_string_t *s, size_t len)
{
    dbg_err_if(len > s->data_len);

    if(len < s->data_len)
    {
        s->data_len = len;
        s->data[len] = 0;
    }

    return 0;
err:
    return ~0;
}

/**
 * \brief  Return the string length
 *
 * Return the length of the given string.
 *
 * \param s     string object
 *
 * \return the string length
 */
inline size_t u_string_len(u_string_t *s)
{
    return s->data_len;
}

/**
 * \brief  Return the string value
 *
 * Return the const char* value of the given string object. Such const char*
 * value cannot be modified, realloc'd or free'd.
 *
 * \param s     string object
 *
 * \return the string value or NULL if the string is empty
 */
inline const char *u_string_c(u_string_t *s)
{
    return s->data;
}

/**
 * \brief  Copy the value of a string to another
 *
 * Copy \a src string to \a dst string. 
 *
 * \param dst   destination string
 * \param src   source string
 *
 * \return \c 0 on success, not zero on failure
 */
inline int u_string_copy(u_string_t *dst, u_string_t *src)
{
    u_string_clear(dst);
    return u_string_append(dst, src->data, src->data_len);
}

/**
 * \brief  Clear a string
 *
 * Totally erase the content of the given string.
 *
 * \param s     string object
 *
 * \return \c 0 on success, not zero on failure
 */
int u_string_clear(u_string_t *s)
{
    /* clear the string but not deallocate the buffer */
    if(s->data_sz)
    {
        s->data[0] = 0;
        s->data_len = 0;
    }

    return 0;
}

/**
 * \brief  Create a new string
 *
 * Create a new string object and save its pointer to \a *ps.
 *
 * If \a buf is not NULL (and \a len > 0) the string will be initialized with
 * the content of \a buf.
 *
 * \param buf   initial string value
 * \param len   length of \a buf
 * \param ps    on success will get the new string object
 *
 * \return \c 0 on success, not zero on failure
 */
int u_string_create(const char *buf, size_t len, u_string_t **ps)
{
    u_string_t *s = NULL;

    s = u_zalloc(sizeof(u_string_t));
    dbg_err_sif(s == NULL);

    s->data = null;

    if(buf)
        dbg_err_if(u_string_append(s, buf, len));

    *ps = s;

    return 0;
err:
    return ~0;
}


/**
 * \brief  Free a string
 *
 * Release all resources and free the given string object.
 *
 * \param s     string object
 *
 * \return \c 0 on success, not zero on failure
 */
int u_string_free(u_string_t *s)
{
    if(s)
    {
        if(s->data_sz)
            U_FREE(s->data);
        U_FREE(s);
    }
    return 0;
}


/**
 * \brief  Set the value of a string
 *
 * Set the value of \a s to \a buf.
 *
 * \param s     string object
 * \param buf   the value that will be copied to \a s
 * \param len   length of \a buf
 *
 * \return \c 0 on success, not zero on failure
 */
int u_string_set(u_string_t *s, const char *buf, size_t len)
{
    u_string_clear(s);
    return u_string_append(s, buf, len);
}

/**
 * \brief  Enlarge the underlying memory block of the given buffer
 *
 * Enlarge the buffer data block to (at least) \a size bytes.
 *
 * \param s     string object
 * \param size  requested size
 *
 * \return \c 0 on success, not zero on failure
 */
int u_string_reserve(u_string_t *s, size_t size)
{
    char *nbuf;

    dbg_err_if(s == NULL);

    if(size <= s->data_sz)
        return 0; /* nothing to do */
   
    /* size plus 1 char to store a '\0' */
    nbuf = u_realloc( ((s->data == null) ? NULL : s->data), size+1);
    dbg_err_if(nbuf == NULL);

    /* zero terminate it */
    nbuf[size] = 0;

    s->data = (void*)nbuf;
    s->data_sz = size;

    return 0;
err:
    return ~0;
}

/**
 * \brief  Append a char* to a string
 *
 * Append a char* value to the given string. 
 *
 * \param s     string object
 * \param buf   the value that will be appended to \a s
 * \param len   length of \a buf
 *
 * \return \c 0 on success, not zero on failure
 */
int u_string_append(u_string_t *s, const char *buf, size_t len)
{
    size_t nsz, min;

    if(!len)
        return 0; /* nothing to do */

    /* if there's not enough space on s->data alloc a bigger buffer */
    if(s->data_len + len + 1 > s->data_sz)
    {
        min = s->data_len + len + 1; /* min required buffer length */
        nsz = s->data_sz;
        while(nsz <= min)
            nsz += (BLOCK_SIZE << s->shift_cnt++);

        dbg_err_if(u_string_reserve(s, nsz));
    }

    /* append this chunk to the data buffer */
    strncpy(s->data + s->data_len, buf, len);
    s->data_len += len;
    s->data[s->data_len] = 0;
    
    return 0;
err:
    return ~0;
}

/**
 * \brief  Set the string from sprintf-style arguments
 *
 * Set a string from the sprintf-style arguments
 *
 * \param s     string object
 * \param fmt   printf-style format
 * \param ...   variable list of arguments
 *
 * \return \c 0 on success, not zero on failure
 */
int u_string_sprintf(u_string_t *s, const char *fmt, ...)
{
    va_list ap;
    size_t sz, avail;

    dbg_return_if(s == NULL, ~0);
    dbg_return_if(fmt == NULL, ~0);

    u_string_clear(s);

again:
    va_start(ap, fmt); 

    avail = s->data_sz - s->data_len; /* avail may be zero */

    /* write to the internal buffer of the string */
    dbg_err_if(( sz = vsnprintf(s->data + s->data_len, avail, fmt, ap)) <= 0);

    if(sz >= avail)
    {
        /* enlarge the buffer (make it at least 128 bytes bigger) */
        dbg_err_if(u_string_reserve(s, s->data_sz + s->data_len + 2*sz + 1));

        /* zero-term the buffer (vsnprintf has removed the last \0!) */
        s->data[s->data_len] = 0;

        va_end(ap);

        /* try again with a bigger buffer */
        goto again;
    }

    /* update string length */
    s->data_len += sz; 
    s->data[s->data_len] = 0;

    va_end(ap);

    return 0;
err:
    va_end(ap);
    return ~0; /* error */
}

/**
 *      \}
 */
