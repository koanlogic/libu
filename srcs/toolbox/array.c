/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.  
 */

#include <stdint.h>
#include <u/libu.h>
#include <toolbox/array.h>

/* for C89 implementations */
#ifndef SIZE_MAX
#define SIZE_MAX ((size_t) -1)
#endif

struct u_array_s
{
    size_t nslots;
    u_array_type_t type;
    void *base;
};

#define U_ARRAY_TYPE_IS_VALID(t) \
    (t > U_ARRAY_TYPE_UNSET && t <= U_ARRAY_TYPE_MAX)

static size_t sizeof_type[U_ARRAY_TYPE_MAX + 1] =
{
    0,                              /* U_ARRAY_TYPE_UNSET  = 0          */
#ifdef HAVE__BOOL
    sizeof(_Bool),                  /* U_ARRAY_TYPE_BOOL                */
#endif  /* HAVE__BOOL */
    sizeof(char),                   /* U_ARRAY_TYPE_CHAR                */
    sizeof(unsigned char),          /* U_ARRAY_TYPE_U_CHAR              */
    sizeof(short),                  /* U_ARRAY_TYPE_SHORT               */
    sizeof(unsigned short),         /* U_ARRAY_TYPE_U_SHORT             */
    sizeof(int),                    /* U_ARRAY_TYPE_INT                 */
    sizeof(unsigned int),           /* U_ARRAY_TYPE_U_INT               */
    sizeof(long),                   /* U_ARRAY_TYPE_LONG                */
    sizeof(unsigned long),          /* U_ARRAY_TYPE_U_LONG              */
#ifdef HAVE_LONG_LONG
    sizeof(long long),              /* U_ARRAY_TYPE_LONG_LONG           */
    sizeof(unsigned long long),     /* U_ARRAY_TYPE_U_LONG_LONG         */
#endif  /* HAVE_LONG_LONG */
#ifdef HAVE_INTMAX_T
    sizeof(intmax_t),               /* U_ARRAY_TYPE_INTMAX              */
    sizeof(uintmax_t),              /* U_ARRAY_TYPE_U_INTMAX            */
#endif  /* HAVE_INTMAX_T */
    sizeof(float),                  /* U_ARRAY_TYPE_FLOAT               */
    sizeof(double),                 /* U_ARRAY_TYPE_DOUBLE              */
#ifdef HAVE_LONG_DOUBLE
    sizeof(long double),            /* U_ARRAY_TYPE_LONG_DOUBLE         */
#endif  /* HAVE_LONG_DOUBLE */
#ifdef HAVE_FLOAT__COMPLEX
    sizeof(float _Complex),         /* U_ARRAY_TYPE_FLOAT_COMPLEX       */
#endif  /* HAVE_FLOAT__COMPLEX */
#ifdef HAVE_DOUBLE__COMPLEX
    sizeof(double _Complex),        /* U_ARRAY_TYPE_DOUBLE_COMPLEX      */
#endif  /* HAVE_DOUBLE__COMPLEX */
#ifdef HAVE_LONG_DOUBLE__COMPLEX
    sizeof(long double _Complex),   /* U_ARRAY_TYPE_LONG_DOUBLE_COMPLEX */
#endif  /* HAVE_LONG_DOUBLE__COMPLEX */
    sizeof(void *)                  /* U_ARRAY_TYPE_PTR                 */
};

#define MAX_NSLOTS(da)  (SIZE_MAX / sizeof_type[da->type])

/**
    \defgroup array Dynamic Arrays
    \{
        A dynamic array has a type, which is the type of its elements.
        The type of the dynamic array is declared when a new array instance 
        is created via ::u_array_create and must be one of the types 
        in ::u_array_type_t.  Available types are the standard C types 
        supported by the target platform, plus a generic pointer type for 
        user defined types.  A couple of getter/setter methods is provided for 
        each ::u_array_type_t entry, e.g. see ::u_array_get_char and 
        ::u_array_set_char.

        The following is some toy code showing basic operations (create,
        set, get, destroy) using double precision complex numbers (C99):
    \code          
    u_array_t *a = NULL;
    size_t idx;
    long double _Complex c0, c1;

    // create an array to store double precision complex numbers
    // array resize is handled transparently
    con_err_if (u_array_create(U_ARRAY_TYPE_LONG_DOUBLE_COMPLEX, 0, &a));

    // insert values from 0+0i to 10+10i at increasing indexes,
    // also check that what has been inserted matches what we get back
    for (idx = 0; idx < 10; idx++)
    {
        c0 = idx + idx * _Complex_I; 
        con_err_if (u_array_set_long_double_complex(a, idx, c0, NULL));
        con_err_if (u_array_get_long_double_complex(a, idx, &c1));
        con_err_if (creal(c0) != creal(c1) || cimag(c0) != cimag(c1));
    }

    // now overwrite previously set values with new ones
    for (idx = 0; idx < 10; idx++)
    {
        long double _Complex c2;

        c0 = (idx + 10) + (idx + 10) * _Complex_I;
        con_err_if (u_array_set_long_double_complex(a, idx, c0, &c2));
        u_con("overwrite %lf + %lfi at %zu with %lf + %lfi", 
                creal(c2), cimag(c2), idx, creal(c0), cimag(c0)); 
        con_err_if (u_array_get_long_double_complex(a, idx, &c1));
        con_err_if (creal(c0) != creal(c1) || cimag(c0) != cimag(c1));
    }

    // ok, enough stress, dispose it :)
    u_array_free(a);
    \endcode

        \note The getter/setter interfaces for generic pointers are different
              from all the other, see ::u_array_get_ptr and ::u_array_set_ptr
              for details.

 */

/**
 *  \brief  Create a new array object
 *
 *  \param t        the type of the elements in this array, i.e. one of 
 *                  the standard C types (which have 1:1 mapping with 
 *                  \c U_ARRAY_TYPE_*'s) or a pointer type (select 
 *                  \c U_ARRAY_TYPE_PTR in this case)
 *  \param nslots   the initial number of slots to be created  (set it to \c 0 
 *                  if you want the default)
 *  \param pda      the newly created array object as a result argument
 *
 *  \retval  0  on success 
 *  \retval -1  on error
 */
int u_array_create (u_array_type_t t, size_t nslots, u_array_t **pda)
{
    u_array_t *da = NULL;
    size_t max_nslots;

    dbg_return_if (pda == NULL, -1);
    dbg_return_if (!U_ARRAY_TYPE_IS_VALID(t), -1);

    da = u_zalloc(sizeof(u_array_t));
    warn_err_sif (da == NULL);

    da->type = t;

    if (nslots == 0)
        da->nslots = U_ARRAY_NSLOTS_DFL;
    else if (nslots > (max_nslots = MAX_NSLOTS(da)))
        da->nslots = max_nslots;
    else
        da->nslots = nslots;

    da->base = u_zalloc(da->nslots * sizeof_type[da->type]);
    warn_err_sif (da->base == NULL);

    *pda = da;

    return 0;
err:
    u_array_free(da);
    return -1;
}

/**
 *  \brief  Free the array object: the array does not own the pointers in it,
 *          the client must free them explicitly
 *
 *  \param da   the array object that has to be disposed
 *
 *  \return nothing 
 */
void u_array_free (u_array_t *da)
{
    nop_return_if (da == NULL, );

    U_FREE(da->base);
    u_free(da);

    return;
}

/**
 *  \brief  Grow the array so that the supplied index can be accomodated
 *
 *  \param da   the array object
 *  \param idx  the index that needs to be accomodated/reached
 *
 *  \retval  0  on success
 *  \retval -1  on error
 */
int u_array_resize (u_array_t *da, size_t idx)
{
    void *new_base;
    size_t new_nslots, max_nslots;

    dbg_return_if (da == NULL, -1);

    /* no need to resize, go out */
    dbg_return_if (idx < da->nslots, 0);

    /* can't go further, go out */
    dbg_return_if (idx >= (max_nslots = MAX_NSLOTS(da)) - 1, -1);

    /* decide how many new slots are needed */
    new_nslots = ((max_nslots - U_ARRAY_RESIZE_PAD - 1) >= idx) ?
        idx + U_ARRAY_RESIZE_PAD + 1 : max_nslots;
 
    /* try to realloc the array base pointer with the new size */
    new_base = u_realloc(da->base, new_nslots * sizeof_type[da->type]);
    warn_err_sif (new_base == NULL);

    da->base = new_base;
    da->nslots = new_nslots;

    return 0;
err:
    return -1;
}

/**
 *  \}
 */

#define U_ARRAY_GETSET_F(pfx, dtype, ctype)    \
int u_array_set##pfx (u_array_t *da, size_t idx, ctype v, ctype *pold)      \
{                                                                           \
    ctype *ep;                                                              \
                                                                            \
    dbg_return_if (da == NULL, -1);                                         \
                                                                            \
    /* be strict over what we're assigning */                               \
    dbg_return_if (da->type != dtype, -1);                                  \
                                                                            \
    /* silently handle resize in case an element is set past the            \
     * actual index range */                                                \
    if (idx > da->nslots - 1)                                               \
        warn_err_if (u_array_resize(da, idx));                              \
                                                                            \
    /* get the selected slot */                                             \
    ep = (ctype *) da->base + idx;                                          \
                                                                            \
    /* if requested copy-out the old value before overriding it */          \
    if (pold)                                                               \
        *pold = *ep;                                                        \
                                                                            \
    /* assign */                                                            \
    *ep = v;                                                                \
                                                                            \
    return 0;                                                               \
err:                                                                        \
    return -1;                                                              \
}                                                                           \
                                                                            \
int u_array_get##pfx (u_array_t *da, size_t idx, ctype *pv)                 \
{                                                                           \
    ctype *ep;                                                              \
                                                                            \
    dbg_return_if (da == NULL, -1);                                         \
    dbg_return_if (pv == NULL, -1);                                         \
    /* be strict over what we're returning */                               \
    dbg_return_if (da->type != dtype, -1);                                  \
                                                                            \
    /* check overflow */                                                    \
    warn_err_if (idx > da->nslots - 1);                                     \
                                                                            \
    ep = (ctype *) da->base + idx;                                          \
                                                                            \
    *pv = *ep;                                                              \
                                                                            \
    return 0;                                                               \
err:                                                                        \
    return -1;                                                              \
}

U_ARRAY_GETSET_F(_char, U_ARRAY_TYPE_CHAR, char)
U_ARRAY_GETSET_F(_u_char, U_ARRAY_TYPE_U_CHAR, unsigned char)
U_ARRAY_GETSET_F(_short, U_ARRAY_TYPE_SHORT, short)
U_ARRAY_GETSET_F(_u_short, U_ARRAY_TYPE_U_SHORT, unsigned short)
U_ARRAY_GETSET_F(_int, U_ARRAY_TYPE_INT, int)
U_ARRAY_GETSET_F(_u_int, U_ARRAY_TYPE_U_INT, unsigned int)
U_ARRAY_GETSET_F(_long, U_ARRAY_TYPE_LONG, long)
U_ARRAY_GETSET_F(_u_long, U_ARRAY_TYPE_U_LONG, unsigned long)
U_ARRAY_GETSET_F(_float, U_ARRAY_TYPE_FLOAT, float)
U_ARRAY_GETSET_F(_double, U_ARRAY_TYPE_DOUBLE, double)

#ifdef HAVE__BOOL
U_ARRAY_GETSET_F(_bool, U_ARRAY_TYPE_BOOL, _Bool)
#endif  /* HAVE__BOOL */

#ifdef HAVE_LONG_LONG
U_ARRAY_GETSET_F(_long_long, U_ARRAY_TYPE_LONG_LONG, long long)
U_ARRAY_GETSET_F(_u_long_long, U_ARRAY_TYPE_U_LONG_LONG, unsigned long long)
#endif  /* HAVE_LONG_LONG */

#ifdef HAVE_INTMAX_T
U_ARRAY_GETSET_F(_intmax, U_ARRAY_TYPE_INTMAX, intmax_t)
U_ARRAY_GETSET_F(_u_intmax, U_ARRAY_TYPE_U_INTMAX, uintmax_t)
#endif  /* HAVE_INTMAX_T */

#ifdef HAVE_LONG_DOUBLE
U_ARRAY_GETSET_F(_long_double, U_ARRAY_TYPE_LONG_DOUBLE, long double)
#endif  /* HAVE_LONG_DOUBLE */

#ifdef HAVE_FLOAT__COMPLEX
U_ARRAY_GETSET_F(_float_complex, U_ARRAY_TYPE_FLOAT_COMPLEX, float _Complex)
#endif  /* HAVE_FLOAT__COMPLEX */

#ifdef HAVE_DOUBLE__COMPLEX
U_ARRAY_GETSET_F(_double_complex, U_ARRAY_TYPE_DOUBLE_COMPLEX, double _Complex)
#endif  /* HAVE_DOUBLE__COMPLEX */

#ifdef HAVE_LONG_DOUBLE__COMPLEX
U_ARRAY_GETSET_F(_long_double_complex, U_ARRAY_TYPE_LONG_DOUBLE_COMPLEX, long double _Complex)
#endif  /* HAVE_LONG_DOUBLE__COMPLEX */

/**
 *  \brief Dynamic array setter interface for generic pointer values.
 *
 *  Try to put some generic pointer \p v at position \p idx into the dyamic
 *  array \p da.  If supplied (i.e. non-NULL) the result argument \p *prc will
 *  hold the return code of the whole set operation.  
 *  We could not preserve the same interface as all the other standard types
 *  because the \c void** out parameter would be easily abused through an 
 *  incorrect cast.  The following sample should illustrate how to correctly
 *  use the interface:
 *  
 *  \code
 *      int rc = 0;
 *      size_t i = 231; // some random index
 *      my_t *old_v, *v = NULL; // the value that we are going to store
 *
 *      // make room for your new object and fill it some way
 *      dbg_err_sif ((v = u_zalloc(sizeof(my_t))) == NULL);
 *      v->somefield = somevalue;
 *      
 *      // set the new value in the array at some conveninent index, 
 *      // and dispose the old one in case it was set
 *      old_v = u_array_set_ptr(da, i, v, &rc);
 *      dbg_err_if (rc == -1);
 *      if (old_v)
 *          my_free(old_v);
 *      ...
 *  \endcode
 *
 *  \param  da      An already instantiated dynamic array 
 *  \param  idx     The index at which the element \p v shall be stored
 *  \param  v       The value that must be stored
 *  \param  prc     If non-NULL this result argument will store the return
 *                  code of the whole operation, i.e. \c 0 if successfull,
 *                  \c -1 if something went wrong
 *
 *  \return the old value stored at index \p idx (can be \c NULL)
 */ 
void *u_array_set_ptr (u_array_t *da, size_t idx, void *v, int *prc)
{
    void **ep, *old_ep;

    dbg_err_if (da == NULL);
    dbg_err_if (da->type != U_ARRAY_TYPE_PTR);

    if (idx > da->nslots - 1)
        dbg_err_if (u_array_resize (da, idx));

    ep = (void **) da->base + idx;

    /* save old value (could be NULL) */
    old_ep = *ep;

    /* overwrite with the supplied value */
    *ep = v;

    if (prc)
        *prc = 0;

    return old_ep;
err:
    if (prc)
        *prc = -1;
    return NULL;
}

/**
 *  \brief Dynamic array getter interface for generic pointer values.
 *
 *  Try to retrieve the pointer at position \p idx from the dyamic array \p da.
 *  If supplied (i.e. non-NULL) the result argument \p *prc will hold the 
 *  return code of the whole get operation.  
 *  We could not preserve the same interface as all the other standard types
 *  because the \c void** out parameter would be easily abused through an 
 *  incorrect cast.  The following sample should illustrate how to correctly 
 *  use the interface:
 *  
 *  \code
 *      int rc = 0;
 *      size_t i = 231; // some random index
 *      my_t *v = NULL; // the value that we are going to retrieve
 *
 *      // get the new value from the array at some conveninent index, 
 *      v = u_array_get_ptr(da, i, &rc);
 *      dbg_err_if (rc == -1);
 *
 *      // do something with 'v'
 *      ...
 *  \endcode
 *
 *  \param  da      An already instantiated dynamic array 
 *  \param  idx     The index at which the element \p v shall be stored
 *  \param  prc     If non-NULL this result argument will store the return
 *                  code of the whole operation, i.e. \c 0 if successfull,
 *                  \c -1 if something went wrong
 *
 *  \return the value stored at index \p idx (note that it can be \c NULL and
 *          therefore it can't be used to distinguish a failed operation.
 */ 
void *u_array_get_ptr (u_array_t *da, size_t idx, int *prc)
{
    dbg_err_if (da == NULL);
    dbg_err_if (da->type != U_ARRAY_TYPE_PTR);

    warn_err_if (idx > da->nslots - 1);

    if (prc)
        *prc = 0;

    return *((void **) da->base + idx);
err:
    if (prc)
        *prc = -1;
    return NULL;
}
