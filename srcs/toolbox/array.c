/* 
 * Copyright (c) 2005-2008 by KoanLogic s.r.l. - All rights reserved.  
 */

#include <u/libu.h>
#include <toolbox/array.h>

struct u_array_s
{
    size_t nslots;
    u_array_type_t type;
    void *base;
};

static size_t sizeof_type[U_ARRAY_TYPE_MAX + 1] =
{
    0,                          /* U_ARRAY_TYPE_UNSET  = 0  */
#ifdef HAVE_BOOL
    sizeof(bool),               /* U_ARRAY_TYPE_BOOL        */
#endif  /* HAVE_BOOL */
    sizeof(char),               /* U_ARRAY_TYPE_CHAR        */
    sizeof(unsigned char),      /* U_ARRAY_TYPE_UCHAR       */
    sizeof(short),              /* U_ARRAY_TYPE_SHORT       */
    sizeof(unsigned short),     /* U_ARRAY_TYPE_USHORT      */
    sizeof(int),                /* U_ARRAY_TYPE_INT         */
    sizeof(unsigned int),       /* U_ARRAY_TYPE_UINT        */
    sizeof(long),               /* U_ARRAY_TYPE_LONG        */
    sizeof(unsigned long),      /* U_ARRAY_TYPE_ULONG       */
#ifdef HAVE_LONG_LONG
    sizeof(long long),          /* U_ARRAY_TYPE_LONGLONG    */
    sizeof(unsigned long long), /* U_ARRAY_TYPE_ULONGLONG   */
#endif  /* HAVE_LONG_LONG */
    sizeof(float),              /* U_ARRAY_TYPE_FLOAT       */
    sizeof(double),             /* U_ARRAY_TYPE_DOUBLE      */
#ifdef HAVE_LONG_DOUBLE
    sizeof(long double),        /* U_ARRAY_TYPE_LONGDOUBLE  */
#endif  /* HAVE_LONG_DOUBLE */
    sizeof(void *)              /* U_ARRAY_TYPE_CUSTOM      */
};

/**
 *  \defgroup array Dynamic Arrays
 *  \{
 */

/**
 *  \brief  Create a new array object
 *
 *  \param t        the type of the elements in this array, i.e. one of 
 *                  the standard C types (which have 1:1 mapping with 
 *                  \c U_ARRAY_TYPE_*'s) or one which has been user defined 
 *                  (select \c U_ARRAY_TYPE_CUSTOM in this case)
 *  \param nslots   the initial number of slots to be created  (set it to \c 0 
 *                  if you want the default)
 *  \param pda      the newly created array object as a result argument
 *
 *  \return \c 0 on success, \c -1 on error
 */
int u_array_create (u_array_type_t t, size_t nslots, u_array_t **pda)
{
    u_array_t *da = NULL;

    dbg_return_if (pda == NULL, -1);
    dbg_return_if (!U_ARRAY_TYPE_IS_VALID(t), -1);

    da = u_zalloc(sizeof(u_array_t));
    warn_err_sif (da == NULL);

    da->type = t;
    da->nslots = nslots ? nslots : U_ARRAY_NSLOTS_DFL;

    da->base = u_calloc(da->nslots, sizeof_type[da->type]);
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
 *  \param idx  the index that needs to be accomodated
 *
 *  \return \c 0 on success, \c -1 on error
 */
int u_array_resize (u_array_t *da, size_t idx)
{
    void *new_base;
    size_t new_nslots;

    dbg_return_if (da == NULL, -1);

    dbg_return_if (idx < da->nslots, 0);    /* no need to resize */

    new_nslots = idx + 1 + U_ARRAY_RESIZE_PAD; /* XXX check overflow */
    
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
int u_array_set_##pfx (u_array_t *da, size_t idx, ctype v, ctype *pold)     \
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
int u_array_get_##pfx (u_array_t *da, size_t idx, ctype *pv)                \
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

U_ARRAY_GETSET_F(char, U_ARRAY_TYPE_CHAR, char)
U_ARRAY_GETSET_F(uchar, U_ARRAY_TYPE_UCHAR, unsigned char)
U_ARRAY_GETSET_F(short, U_ARRAY_TYPE_SHORT, short)
U_ARRAY_GETSET_F(ushort, U_ARRAY_TYPE_USHORT, unsigned short)
U_ARRAY_GETSET_F(int, U_ARRAY_TYPE_INT, int)
U_ARRAY_GETSET_F(uint, U_ARRAY_TYPE_UINT, unsigned int)
U_ARRAY_GETSET_F(long, U_ARRAY_TYPE_LONG, long)
U_ARRAY_GETSET_F(ulong, U_ARRAY_TYPE_ULONG, unsigned long)
U_ARRAY_GETSET_F(float, U_ARRAY_TYPE_FLOAT, float)
U_ARRAY_GETSET_F(double, U_ARRAY_TYPE_DOUBLE, double)
U_ARRAY_GETSET_F(custom, U_ARRAY_TYPE_CUSTOM, void *)

#ifdef HAVE_BOOL
U_ARRAY_GETSET_F(bool, U_ARRAY_TYPE_BOOL, bool)
#endif  /* HAVE_BOOL */

#ifdef HAVE_LONG_LONG
U_ARRAY_GETSET_F(longlong, U_ARRAY_TYPE_LONGLONG, long long)
U_ARRAY_GETSET_F(ulonglong, U_ARRAY_TYPE_ULONGLONG, unsigned long long)
#endif  /* HAVE_LONG_LONG */

#ifdef HAVE_LONG_DOUBLE
U_ARRAY_GETSET_F(longdouble, U_ARRAY_TYPE_LONGDOUBLE, long double)
#endif  /* HAVE_LONG_DOUBLE */

