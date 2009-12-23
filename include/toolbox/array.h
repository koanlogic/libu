/* 
 * Copyright (c) 2005-2010 by KoanLogic s.r.l. - All rights reserved.  
 */

#ifndef _U_ARRAY_H_
#define _U_ARRAY_H_

#include <u/libu_conf.h>
#ifdef HAVE_BOOL
#include <stdbool.h>
#endif
#ifdef HAVE_COMPLEX
#include <complex.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \addtogroup array
 *  \{
 */

/** \brief  default number of slots on array creation (can be changed at
 *          compile time via "-DU_ARRAY_NSLOTS_DFL=nnn" flag)  */
#ifndef U_ARRAY_NSLOTS_DFL
#define U_ARRAY_NSLOTS_DFL 512
#endif

/** \brief  right-pad when doing dynamic resize (can be changed at compile 
 *          time via "-DU_ARRAY_RESIZE_PAD=nnn" flag) */
#ifndef U_ARRAY_RESIZE_PAD
#define U_ARRAY_RESIZE_PAD 100
#endif

/** \brief Available dynamic array types, i.e. the standard C types supported 
 *         by the target platform, plus an any type pointer for user defined 
 *         types. */
typedef enum {
        U_ARRAY_TYPE_UNSET = 0,             /**< no type */
#ifdef HAVE_BOOL
        U_ARRAY_TYPE_BOOL,                  /**< \c bool */
#endif  /* HAVE_BOOL */
        U_ARRAY_TYPE_CHAR,                  /**< \c char */
        U_ARRAY_TYPE_U_CHAR,                /**< <tt>unsigned char</tt> */
        U_ARRAY_TYPE_SHORT,                 /**< <tt>short int</tt> */
        U_ARRAY_TYPE_U_SHORT,               /**< <tt>unsigned short int</tt> */
        U_ARRAY_TYPE_INT,                   /**< \c int */
        U_ARRAY_TYPE_U_INT,                 /**< <tt>unsigned int</tt> */
        U_ARRAY_TYPE_LONG,                  /**< <tt>long int</tt> */
        U_ARRAY_TYPE_U_LONG,                /**< <tt>unsigned long int</tt> */
#ifdef HAVE_LONG_LONG
        U_ARRAY_TYPE_LONG_LONG,             /**< <tt>long long</tt> */
        U_ARRAY_TYPE_U_LONG_LONG,           /**< <tt>unsigned long long</tt> */
#endif  /* HAVE_LONG_LONG */
        U_ARRAY_TYPE_FLOAT,                 /**< \c float */
        U_ARRAY_TYPE_DOUBLE,                /**< \c double */
#ifdef HAVE_LONG_DOUBLE
        U_ARRAY_TYPE_LONG_DOUBLE,           /**< <tt>long double</tt> */
#endif  /* HAVE_LONG_DOUBLE */
#ifdef HAVE_FLOAT_COMPLEX
        U_ARRAY_TYPE_FLOAT_COMPLEX,         /**< <tt>float complex</tt> */
#endif  /* HAVE_FLOAT_COMPLEX */
#ifdef HAVE_DOUBLE_COMPLEX
        U_ARRAY_TYPE_DOUBLE_COMPLEX,        /**< <tt>double complex</tt> */
#endif  /* HAVE_DOUBLE_COMPLEX */
#ifdef HAVE_LONG_DOUBLE_COMPLEX
        U_ARRAY_TYPE_LONG_DOUBLE_COMPLEX,   /**< <tt>long double complex</tt> */
#endif  /* HAVE_LONG_DOUBLE_COMPLEX */
        U_ARRAY_TYPE_PTR,                   /**< generic pointer */
        U_ARRAY_TYPE_MAX = U_ARRAY_TYPE_PTR
} u_array_type_t;
         
#define U_ARRAY_TYPE_IS_VALID(t) \
    (t > U_ARRAY_TYPE_UNSET && t <= U_ARRAY_TYPE_MAX)

struct u_array_s;
typedef struct u_array_s u_array_t;

/* [cd]tor */
int u_array_create (u_array_type_t t, size_t nslots, u_array_t **pda);
void u_array_free (u_array_t *da);

/* dyn resize */
int u_array_resize (u_array_t *da, size_t req_idx);

/** \brief  Put value \p v into the array \p da at index \p idx.
 *          In case \p pold is not \c NULL, the overridden value (if any)
 *          is copied out.  \b NOTE: an identical interface -- aside from the 
 *          type of \p v and \p pold parameters -- is given for any supported 
 *          types.
 *
 *  \param  da      An already instantiated dynamic array
 *  \param  idx     The index at which the element shall be placed
 *                  Transparent resize is handled in case the index were out
 *                  of actual array bounds
 *  \param  v       The value that must be copied into the array
 *  \param  pold    If not \c NULL, it is the address at which the overridden
 *                  element (if any) shall be copied
 *
 *  \return \c 0 on success \c -1 on error */
int u_array_set_char (u_array_t *da, size_t idx, char v, char *pold);
int u_array_set_u_char (u_array_t *da, size_t idx, unsigned char v, 
        unsigned char *pold);
int u_array_set_short (u_array_t *da, size_t idx, short v, short *pold);
int u_array_set_u_short (u_array_t *da, size_t idx, unsigned short v, 
        unsigned short *pold);
int u_array_set_int (u_array_t *da, size_t idx, int v, int *pold);
int u_array_set_u_int (u_array_t *da, size_t idx, unsigned int v, 
        unsigned int *pold);
int u_array_set_long (u_array_t *da, size_t idx, long v, long *pold);
int u_array_set_u_long (u_array_t *da, size_t idx, unsigned long v, 
        unsigned long *pold);
int u_array_set_float (u_array_t *da, size_t idx, float v, float *pold);
int u_array_set_double (u_array_t *da, size_t idx, double v, double *pold);
int u_array_set_ptr (u_array_t *da, size_t idx, void *v, void **pold);

/** \brief  Get the element of \c char array \p da at index \p idx and save it 
 *          at \p *pv.  \b NOTE: an identical interface -- aside from the 
 *          type of \p pv parameter -- is given for the supported types.
 *
 *  \param  da      An already instantiated dynamic array
 *  \param  idx     The index at which the element shall be retrieved
 *  \param  pv      The address at which the retrieved value is copied
 *
 *  \return \c 0 on success \c -1 on error */
int u_array_get_char (u_array_t *da, size_t idx, char *pv);
int u_array_get_u_char (u_array_t *da, size_t idx, unsigned char *pv);
int u_array_get_short (u_array_t *da, size_t idx, short *pv);
int u_array_get_u_short (u_array_t *da, size_t idx, unsigned short *pv);
int u_array_get_int (u_array_t *da, size_t idx, int *pv);
int u_array_get_u_int (u_array_t *da, size_t idx, unsigned int *pv);
int u_array_get_long (u_array_t *da, size_t idx, long *pv);
int u_array_get_u_long (u_array_t *da, size_t idx, unsigned long *pv);
int u_array_get_float (u_array_t *da, size_t idx, float *pv);
int u_array_get_double (u_array_t *da, size_t idx, double *pv);
int u_array_get_ptr (u_array_t *da, size_t idx, void **pv);

#ifdef HAVE_BOOL
/* set */
int u_array_set_bool (u_array_t *da, size_t idx, bool v, bool *pold);
/* get */
int u_array_get_bool (u_array_t *da, size_t idx, bool *pv);
#endif  /* HAVE_BOOL */

#ifdef HAVE_LONG_LONG
/* set */
int u_array_set_long_long (u_array_t *da, size_t idx, long long v, 
        long long *pold);
int u_array_set_u_long_long (u_array_t *da, size_t idx, unsigned long long v, 
        unsigned long long *pold);
/* get */
int u_array_get_long_long (u_array_t *da, size_t idx, long long *pv);
int u_array_get_u_long_long (u_array_t *da, size_t idx, unsigned long long *pv);
#endif  /* HAVE_LONG_LONG */

#ifdef HAVE_LONG_DOUBLE
/* set */
int u_array_set_long_double (u_array_t *da, size_t idx, long double v, 
        long double *pold);
/* get */
int u_array_get_long_double (u_array_t *da, size_t idx, long double *pv);
#endif  /* HAVE_LONG_DOUBLE */

#ifdef HAVE_FLOAT_COMPLEX
/* set */
int u_array_set_float_complex (u_array_t *da, size_t idx, float complex v, 
        float complex *pold);
/* get */
int u_array_get_float_complex (u_array_t *da, size_t idx, float complex *pv);
#endif  /* HAVE_FLOAT_COMPLEX */

#ifdef HAVE_DOUBLE_COMPLEX
/* set */
int u_array_set_double_complex (u_array_t *da, size_t idx, double complex v, 
        double complex *pold);
/* get */
int u_array_get_double_complex (u_array_t *da, size_t idx, double complex *pv);
#endif  /* HAVE_DOUBLE_COMPLEX */

#ifdef HAVE_LONG_DOUBLE_COMPLEX
/* set */
int u_array_set_long_double_complex (u_array_t *da, size_t idx, 
        long double complex v, long double complex *pold);
/* get */
int u_array_get_long_double_complex (u_array_t *da, size_t idx, 
        long double complex *pv);
#endif  /* HAVE_LONG_DOUBLE_COMPLEX */

/**
 *  \}
 */

#ifdef __cplusplus
}
#endif

#endif /* !_U_ARRAY_H_ */
