/* 
 * Copyright (c) 2005-2010 by KoanLogic s.r.l. - All rights reserved.  
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
    char *data;
    size_t size, len;
};

/**
 *  \defgroup buf Buf
 *  \{
 */

/**
 * \brief  Enlarge the underlying memory block of the given buffer
 *
 * Enlarge the buffer data block to (at least) \a size bytes.
 *
 * \param ubuf  buffer object
 * \param size  requested size
 *
 * \return \c 0 on success, not zero on failure
 */
int u_buf_reserve(u_buf_t *ubuf, size_t size)
{
    char *nbuf;

    dbg_err_if(ubuf == NULL);

    if(size <= ubuf->size)
        return 0; /* nothing to do */
   
    /* size plus 1 char to store a '\0' */
    nbuf = u_realloc(ubuf->data, size+1);
    dbg_err_if(nbuf == NULL);

    /* buffer data will always be zero-terminated (but the len field will not
     * count the last '\0') */
    nbuf[size] = 0;

    ubuf->data = (void*)nbuf;
    ubuf->size = size;

    return 0;
err:
    return ~0;
}

/**
 * \brief  Append some data to the buffer
 *
 * Append \a data of size \a size to the given buffer. If needed the buffer
 * will be enlarged.
 *
 * \param ubuf  buffer object
 * \param data  the data block to append
 * \param size  size of \a data
 *
 * \return \c 0 on success, not zero on failure
 */
int u_buf_append(u_buf_t *ubuf, const void *data, size_t size)
{
    dbg_err_if(ubuf == NULL);
    dbg_err_if(data == NULL);
    dbg_err_if(size == 0);

    if(ubuf->size - ubuf->len < size)
    {   /* buffer too small, need to resize */
        dbg_err_if(u_buf_reserve(ubuf, ubuf->size + ubuf->len + 2*size));
    }
   
    memcpy(ubuf->data + ubuf->len, data, size);
    ubuf->len += size;

    /* zero term the buffer so it can be used (when applicable) as a string */
    ubuf->data[ubuf->len] = 0;

    return 0;
err:
    return ~0;
}

/**
 * \brief  Fill a buffer object with the content of a file
 *
 * Open \a filename and copy its whole content into the given buffer.
 *
 * \param ubuf     buffer object
 * \param filename the source filename
 *
 * \return \c 0 on success, not zero on failure
 */
int u_buf_load(u_buf_t *ubuf, const char *filename)
{
    struct stat st;
    FILE *fp = NULL;

    dbg_err_if(ubuf == NULL);
    dbg_err_if(filename == NULL);

    dbg_err_if(stat(filename, &st));

    /* clear the current data */
    dbg_err_if(u_buf_clear(ubuf));

    /* be sure to have a big enough buffer */
    dbg_err_if(u_buf_reserve(ubuf, st.st_size));

    dbg_err_sif((fp = fopen(filename, "r")) == NULL);

    /* fill the buffer with the whole file content */
    dbg_err_if(fread(ubuf->data, st.st_size, 1, fp) == 0);
    ubuf->len = st.st_size;

    fclose(fp);

    return 0;
err:
    if(fp)
        fclose(fp);
    return ~0;
}

/**
 * \brief  Release buffer's underlying memory block without freeing it
 *
 * Release the underlying memory block of the given buffer without 
 * calling free() on it. The caller must free the buffer later on (probably
 * after using it somwhow). 
 *
 * Use u_buf_ptr() to get the pointer of the memory block, u_buf_size() to
 * get its size and u_buf_len() to get its length.
 *
 * \param ubuf     buffer object
 *
 * \return \c 0 on success, not zero on failure
 */
int u_buf_detach(u_buf_t *ubuf)
{
    dbg_err_if(ubuf == NULL);

    ubuf->data = NULL;
    ubuf->size = 0;
    ubuf->len = 0;

    return 0;
err:
    return ~0;
}

/**
 * \brief  Return the size of memory block allocated by the buffer
 *
 * Return the size of memory block allocated by the buffer. 
 *
 * \param ubuf     buffer object
 *
 * \return the data buffer length
 */
size_t u_buf_size(u_buf_t *ubuf)
{
    dbg_err_if(ubuf == NULL);

    return ubuf->size;
err:
    return 0;
}

/**
 * \brief  Return the length of the buffer
 *
 * Return the length of data store in the given buffer.
 *
 * \param ubuf     buffer object
 *
 * \return the data buffer length
 */
size_t u_buf_len(u_buf_t *ubuf)
{
    dbg_err_if(ubuf == NULL);

    return ubuf->len;
err:
    return 0;
}

/**
 * \brief  Clear a buffer
 *
 * Totally erase the content of the given buffer. The memory allocated by
 * the buffer will not be released until u_buf_free() is called.
 *
 * \param ubuf     buffer object
 *
 * \return \c 0 on success, not zero on failure
 */
int u_buf_clear(u_buf_t *ubuf)
{
    dbg_err_if(ubuf == NULL);
    
    ubuf->len = 0;

    return 0;
err:
    return ~0;
}

/**
 * \brief  Set the value of a buffer
 *
 * Set the value of \a ubuf to \a data. If needed the buffer object will alloc
 * more memory to store the \a data value.
 *
 * \param ubuf  buffer object
 * \param data  the value that will be copied into the buffer
 * \param size  size of \a data
 *
 * \return \c 0 on success, not zero on failure
 */
int u_buf_set(u_buf_t *ubuf, const void *data, size_t size)
{
    dbg_err_if(ubuf == NULL);
    dbg_err_if(data == NULL);
    dbg_err_if(size == 0);

    dbg_err_if(u_buf_clear(ubuf));

    dbg_err_if(u_buf_append(ubuf, data, size));

    return 0;
err:
    return ~0;
}

/**
 * \brief  Return a pointer to the buffer internal momory block
 *
 * Return a void* pointer to the memory block allocated by the buffer object.
 *
 * \param ubuf     buffer object
 *
 * \return \c 0 on success, not zero on failure
 */
void* u_buf_ptr(u_buf_t *ubuf)
{
    dbg_err_if(ubuf == NULL);
    
    return ubuf->data;
err:
    return NULL;
}

/**
 * \brief  Free a buffer
 *
 * Release all resources and free the given buffer object.
 *
 * \param ubuf     buffer object
 *
 * \return \c 0 on success, not zero on failure
 */
int u_buf_free(u_buf_t *ubuf)
{
    dbg_err_if(ubuf == NULL);

    if(ubuf->data)
        u_free(ubuf->data);

    u_free(ubuf);

    return 0;
err:
    return ~0;
}

/**
 * \brief  Append a string to the given buffer
 *
 * Create a string from the printf-style arguments and append it to the 
 * given u_buf_t object.
 *
 * The length of the appended string (NOT including the ending '\\0') will 
 * be added to the current length of the buffer (u_buf_len).
 *
 * \param ubuf  buffer object
 * \param fmt   printf-style format
 * \param ...   variable list of arguments
 *
 * \return \c 0 on success, not zero on failure
 */
int u_buf_printf(u_buf_t *ubuf, const char *fmt, ...)
{
    va_list ap;
    size_t sz, avail;

    dbg_return_if(ubuf == NULL, ~0);
    dbg_return_if(fmt == NULL, ~0);

again:
    va_start(ap, fmt); 

    avail = ubuf->size - ubuf->len; /* avail may be zero */

    /* write to the internal buffer of ubuf */
    dbg_err_if(( sz = vsnprintf(ubuf->data + ubuf->len, avail, fmt, ap)) <= 0);

    if(sz >= avail)
    {
        /* enlarge the buffer (make it at least 128 bytes bigger) */
        dbg_err_if(u_buf_reserve(ubuf, ubuf->len + U_MAX(128, sz + 1)));

        /* zero-term the buffer (vsnprintf has removed the last \0!) */
        ubuf->data[ubuf->len] = 0;

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
    return ~0; /* error */
}

/**
 * \brief  Create a new buffer
 *
 * Create a new buffer object and save its pointer to \a *ps.
 *
 * \param pubuf    on success will get the new buffer object
 *
 * \return \c 0 on success, not zero on failure
 */
int u_buf_create(u_buf_t **pubuf)
{
    u_buf_t *ubuf = NULL;

    dbg_err_if(pubuf == NULL);

    ubuf = (u_buf_t*)u_zalloc(sizeof(u_buf_t));
    dbg_err_if(ubuf == NULL);

    *pubuf = ubuf;

    return 0;
err:
    return ~0;
}

/**
 *      \}
 */

