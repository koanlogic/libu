/* 
 * Copyright (c) 2005-2010 by KoanLogic s.r.l. - All rights reserved.  
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

static size_t sizeof_type[U_ARRAY_TYPE_MAX + 1] =
{
    0,                              /* U_ARRAY_TYPE_UNSET  = 0          */
#ifdef HAVE_BOOL
    sizeof(bool),                   /* U_ARRAY_TYPE_BOOL                */
#endif  /* HAVE_BOOL */
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
    sizeof(float),                  /* U_ARRAY_TYPE_FLOAT               */
    sizeof(double),                 /* U_ARRAY_TYPE_DOUBLE              */
#ifdef HAVE_LONG_DOUBLE
    sizeof(long double),            /* U_ARRAY_TYPE_LONG_DOUBLE         */
#endif  /* HAVE_LONG_DOUBLE */
#ifdef HAVE_FLOAT_COMPLEX
    sizeof(float complex),          /* U_ARRAY_TYPE_FLOAT_COMPLEX       */
#endif  /* HAVE_FLOAT_COMPLEX */
#ifdef HAVE_DOUBLE_COMPLEX
    sizeof(double complex),         /* U_ARRAY_TYPE_DOUBLE_COMPLEX      */
#endif  /* HAVE_DOUBLE_COMPLEX */
#ifdef HAVE_LONG_DOUBLE_COMPLEX
    sizeof(long double complex),    /* U_ARRAY_TYPE_LONG_DOUBLE_COMPLEX */
#endif  /* HAVE_LONG_DOUBLE_COMPLEX */
    sizeof(void *)                  /* U_ARRAY_TYPE_PTR                 */
};

#define MAX_NSLOTS(da)  (SIZE_MAX / sizeof_type[da->type])

/**
 *  \defgroup array Dynamic Arrays
 *  \{
 *      A dynamic array has a type, which is the type of its elements.
 *      The type of the dynamic array is declared when creating a new
 *      array instance via u_array_create() and must be one of the types 
 *      in u_array_type_t.  Available type are the standard C types supported 
 *      by the target platform, plus a generic pointer type for user defined
 *      types. A couple of getter/setter methods is provided for each 
 *      u_array_type_t entry.
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
 *  \return \c 0 on success, \c -1 on error
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
 *  \return \c 0 on success, \c -1 on error
 */
int u_array_resize (u_array_t *da, size_t idx)
{
    void *new_base;
    size_t new_nslots, max_nslots;

    dbg_return_if (da == NULL, -1);

    /* no need to resize, go out */
    dbg_return_if (idx < da->nslots, 0);        

    /* can't go further, go out */
    dbg_return_if (idx >= (max_nslots = MAX_NSLOTS(da)) - 1, 0);   

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
    warn_err_if (idx > da->nslots -1);                                      \
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
U_ARRAY_GETSET_F(_ptr, U_ARRAY_TYPE_PTR, void *)

#ifdef HAVE_BOOL
U_ARRAY_GETSET_F(_bool, U_ARRAY_TYPE_BOOL, bool)
#endif  /* HAVE_BOOL */

#ifdef HAVE_LONG_LONG
U_ARRAY_GETSET_F(_long_long, U_ARRAY_TYPE_LONG_LONG, long long)
U_ARRAY_GETSET_F(_u_long_long, U_ARRAY_TYPE_U_LONG_LONG, unsigned long long)
#endif  /* HAVE_LONG_LONG */

#ifdef HAVE_LONG_DOUBLE
U_ARRAY_GETSET_F(_long_double, U_ARRAY_TYPE_LONG_DOUBLE, long double)
#endif  /* HAVE_LONG_DOUBLE */

#ifdef HAVE_FLOAT_COMPLEX
U_ARRAY_GETSET_F(_float_complex, U_ARRAY_TYPE_FLOAT_COMPLEX, float complex)
#endif  /* HAVE_FLOAT_COMPLEX */

#ifdef HAVE_DOUBLE_COMPLEX
U_ARRAY_GETSET_F(_double_complex, U_ARRAY_TYPE_DOUBLE_COMPLEX, double complex)
#endif  /* HAVE_DOUBLE_COMPLEX */

#ifdef HAVE_LONG_DOUBLE_COMPLEX
U_ARRAY_GETSET_F(_long_double_complex, U_ARRAY_TYPE_LONG_DOUBLE_COMPLEX, long double complex)
#endif  /* HAVE_LONG_DOUBLE_COMPLEX */

