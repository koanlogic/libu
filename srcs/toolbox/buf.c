/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.  
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <u/libu.h>
#include <toolbox/buf.h>

struct u_buf_s
{
    char *data;     /* the memory buffer */
    size_t size;    /* bytes malloc'd for .data */
    size_t len;     /* bytes actually in use (always <= .size) */
};

/**
    \defgroup buf Buffer
    \{
        The \ref buf module can be thought as the dual of the \ref string
        module for generic data buffers.  The exposed primitives provide
        hooks for the typical life-cycle of a data buffer: ex-nihil creation 
        or loading from external source, manipulation, possible data printing,
        raw data extraction and/or dump to non-volatile storage, and disposal.
 */

/**
 *  \brief  Enlarge the memory allocated to a given buffer
 *
 *  Enlarge the buffer data block to (at least) \p size bytes.
 *
 *  \param  ubuf    an already allocated ::u_buf_t object
 *  \param  size    the requested size in bytes
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_buf_reserve (u_buf_t *ubuf, size_t size)
{
    char *nbuf;

    dbg_err_if (ubuf == NULL);

    nop_return_if (size <= ubuf->size, 0);  /* nothing to do */
   
    /* size plus 1 char to store a '\0' */
    dbg_err_sif ((nbuf = u_realloc(ubuf->data, size + 1)) == NULL);

    /* buffer data will always be zero-terminated (but the len field will not
     * count the last '\0') */
    nbuf[size] = '\0';

    ubuf->data = nbuf;
    ubuf->size = size;

    return 0;
err:
    return ~0;
}

/**
 *  \brief  Append data to the buffer
 *
 *  Append \p size bytes of \p data to the given buffer. If needed the buffer
 *  will be transparently enlarged.
 *
 *  \param  ubuf  an already allocated ::u_buf_t object
 *  \param  data  a reference to the data block that will be appended
 *  \param  size  the number of bytes to append
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_buf_append (u_buf_t *ubuf, const void *data, size_t size)
{
    dbg_return_if (ubuf == NULL, ~0);
    dbg_return_if (data == NULL, ~0);
    dbg_return_if (size == 0, ~0);

    if (ubuf->size - ubuf->len < size)
    {
        /* buffer too small, resize generously */
        dbg_err_if (u_buf_reserve(ubuf, ubuf->size + ubuf->len + 2 * size));
    }
 
    memcpy(ubuf->data + ubuf->len, data, size);
    ubuf->len += size;

    /* zero term the buffer so it can be used (when applicable) as a string */
    ubuf->data[ubuf->len] = '\0';

    return 0;
err:
    return ~0;
}

/**
 *  \brief  Fill a buffer object with the content of a file
 *
 *  Open \p filename and copy its whole content into the given buffer \p ubuf
 *
 *  \param  ubuf        an already allocated ::u_buf_t object
 *  \param  filename    path of the source file
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_buf_load (u_buf_t *ubuf, const char *filename)
{
    struct stat st;
    FILE *fp = NULL;

    dbg_return_if (ubuf == NULL, ~0);
    dbg_return_if (filename == NULL, ~0);

    dbg_err_sif (stat(filename, &st) == -1);

    /* clear the current data */
    dbg_err_if (u_buf_clear(ubuf));

    /* be sure to have a big enough buffer */
    dbg_err_if (u_buf_reserve(ubuf, st.st_size));

    dbg_err_sifm ((fp = fopen(filename, "r")) == NULL, "%s", filename);

    /* fill the buffer with the whole file content */
    dbg_err_if (fread(ubuf->data, st.st_size, 1, fp) != 1);
    ubuf->len = st.st_size;

    (void) fclose(fp);

    return 0;
err:
    U_FCLOSE(fp);
    return ~0;
}

/**
 *  \brief  Save buffer to file
 *
 *  Save the ::u_buf_t object \p ubuf to \p filename
 *
 *  \param  ubuf        an already allocated ::u_buf_t object
 *  \param  filename    path of the destination file
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_buf_save (u_buf_t *ubuf, const char *filename)
{
    dbg_return_if (ubuf == NULL, ~0);
    dbg_return_if (filename == NULL, ~0);

    return u_data_dump(u_buf_ptr(ubuf), u_buf_len(ubuf), filename);
}

/**
 *  \brief  Give up ownership of the ::u_buf_t data block
 *
 *  Give up ownership of the data block of the supplied ::u_buf_t object 
 *  \p ubuf.  Note that ::u_free won't be called on it, so the caller must 
 *  ::u_free the buffer later on.  Before calling ::u_buf_detach, use 
 *  ::u_buf_ptr to get the pointer of the data block, and possibly ::u_buf_size
 *  and ::u_buf_len to get its total size and actually used length.
 *
 *  \param  ubuf    a previously allocated ::u_buf_t object
 *
 *  \retval  0  on success
 *  \retval ~0  on failure (i.e. \c NULL \p ubuf supplied)
 */
int u_buf_detach (u_buf_t *ubuf)
{
    dbg_return_if (ubuf == NULL, ~0);

    ubuf->data = NULL;
    ubuf->size = 0;
    ubuf->len = 0;

    return 0;
}

/**
 *  \brief  Return the total size of the data block allocated by the buffer
 *
 *  Return the total size of data block allocated by the supplied ::u_buf_t 
 *  object \p ubuf. 
 *
 *  \param  ubuf    a previously allocated ::u_buf_t object
 *
 *  \return the length in bytes of the data block (which could even be \c 0 
 *          in case \p ubuf is detached), or \c -1 if a \c NULL ::u_buf_t
 *          reference was supplied.
 */
ssize_t u_buf_size (u_buf_t *ubuf)
{
    dbg_return_if (ubuf == NULL, -1);

    return ubuf->size;
}

/**
 *  \brief  Return the number of used bytes in the supplied buffer
 *
 *  Return the number of bytes actually used in the supplied ::u_buf_t object 
 *  \p ubuf
 *
 *  \param  ubuf    a previously allocated ::u_buf_t object
 *
 *  \return the number of \p used bytes or \c -1 if the ::u_buf_t reference
 *          is invalid
 */
ssize_t u_buf_len (u_buf_t *ubuf)
{
    dbg_return_if (ubuf == NULL, -1);

    return ubuf->len;
}

/**
 *  \brief  Clear a buffer
 *
 *  Reset the internal \p used bytes indicator.  Note that the memory allocated
 *  by the buffer object won't be ::u_free'd until ::u_buf_free is called.
 *
 *  \param  ubuf    a previously allocated ::u_buf_t object
 *
 *  \retval  0  on success
 *  \retval ~0  on failure (i.e. \c NULL \p ubuf supplied)
 */
int u_buf_clear (u_buf_t *ubuf)
{
    dbg_return_if (ubuf == NULL, ~0);
    
    ubuf->len = 0;

    return 0;
}

/**
 *  \brief  Set the value of a buffer
 *
 *  Explicitly set the value of \a ubuf to \a data.  If needed the buffer
 *  object will call ::u_buf_append to enlarge the storage needed to copy-in
 *  the \a data value.
 *
 *  \param  ubuf    a previously allocated ::u_buf_t object
 *  \param  data    a reference to the memory block that will be copied into
 *                  the buffer
 *  \param  size    size of \a data in bytes
 *
 *  \retval  0  on success
 *  \retval ~0  on error
 */
int u_buf_set (u_buf_t *ubuf, const void *data, size_t size)
{
    dbg_return_if (ubuf == NULL, ~0);
    dbg_return_if (data == NULL, ~0);
    dbg_return_if (size == 0, ~0);

    dbg_err_if (u_buf_clear(ubuf));
    dbg_err_if (u_buf_append(ubuf, data, size));

    return 0;
err:
    return ~0;
}

/**
 *  \brief  Return the pointer to the internal data block
 *
 *  Return a generic pointer to the data block allocated by the ::u_buf_t 
 *  object \p ubuf.
 *
 *  \param  ubuf    a previously allocated buffer object
 *
 *  \return the pointer to the internal data block of \c NULL if an invalid
 *          ::u_buf_t reference is supplied
 */
void *u_buf_ptr (u_buf_t *ubuf)
{
    dbg_return_if (ubuf == NULL, NULL);
    
    return ubuf->data;
}

/**
 *  \brief  Removes bytes from the tail of the buffer
 *
 *  Set the new length of the buffer; \p newlen must be less or equal to the
 *  current buffer length.
 *
 *  \param  ubuf    a previously allocated buffer object
 *  \param  newlen     new length of the buffer
 *
 *  \return 
 *  \retval  0  on success
 *  \retval ~0  on error
 */
int u_buf_shrink(u_buf_t *ubuf, size_t newlen)
{
    dbg_err_if (ubuf == NULL);
    dbg_err_if (newlen > ubuf->len);

    ubuf->len = newlen;

    return 0;
err:
    return ~0;
}

/**
 *  \brief  Free a buffer
 *
 *  Release all resources and free the given buffer object.
 *
 *  \param  ubuf    buffer object
 *
 *  \retval  0  on success
 *  \retval ~0  on error
 */
int u_buf_free (u_buf_t *ubuf)
{
    dbg_return_if (ubuf == NULL, ~0);

    U_FREE(ubuf->data);
    u_free(ubuf);

    return 0;
}

/**
 *  \brief  Append a string to the given buffer
 *
 *  Create a NUL-terminated string from the \c printf(3) like arguments and 
 *  append it to the given ::u_buf_t object.  The length of the appended string
 *  (NOT including the ending \c \\0) will be added to the current length of 
 *  the buffer (::u_buf_len).
 *
 *  \param  ubuf    a previously allocated ::u_buf_t object
 *  \param  fmt     printf(3) format string
 *  \param  ...     variable list of arguments used by the \p fmt
 *
 *  \retval  0  on success
 *  \retval ~0  on error
 */
int u_buf_printf (u_buf_t *ubuf, const char *fmt, ...)
{
    va_list ap;
    size_t sz, avail;

    dbg_return_if (ubuf == NULL, ~0);
    dbg_return_if (fmt == NULL, ~0);

again:
    va_start(ap, fmt); 

    avail = ubuf->size - ubuf->len; /* avail may be zero */

    /* write to the internal buffer of ubuf */
    dbg_err_if ((sz = vsnprintf(ubuf->data + ubuf->len, avail, fmt, ap)) <= 0);

    if (sz >= avail)
    {
        /* enlarge the buffer (make it at least 128 bytes bigger) */
        dbg_err_if (u_buf_reserve(ubuf, ubuf->len + U_MIN(128, sz + 1)));

        /* zero-term the buffer (vsnprintf has removed the last \0!) */
        ubuf->data[ubuf->len] = '\0';

        va_end(ap);

        /* try again with a bigger buffer */
        goto again;
    }

    /* update data length (don't include the '\0' in the size count) */
    ubuf->len += sz; 

    va_end(ap);

    return 0;
err:
    va_end(ap);
    return ~0;
}


/**
 *  \brief  Create a new buffer
 *
 *  Create a new ::u_buf_t object and save its reference to \p *ps.
 *
 *  \param  pubuf   result argument: on success it will get the new ::u_buf_t
 *                  object
 *
 *  \retval  0  on success
 *  \retval ~0  on error
 */
int u_buf_create (u_buf_t **pubuf)
{
    u_buf_t *ubuf = NULL;

    dbg_return_if (pubuf == NULL, ~0);

    dbg_err_sif ((ubuf = u_zalloc(sizeof(u_buf_t))) == NULL);

    ubuf->len = 0;
    ubuf->size = 0;
    ubuf->data = NULL;

    *pubuf = ubuf;

    return 0;
err:
    return ~0;
}

/**
 *      \}
 */
