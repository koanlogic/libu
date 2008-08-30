/* 
 * Copyright (c) 2005-2008 by KoanLogic s.r.l. - All rights reserved.  
 */

#ifndef _U_ARRAY_H_
#define _U_ARRAY_H_

#include <u/libu_conf.h>
#ifdef HAVE_BOOL
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef U_ARRAY_NSLOTS_DFL
#define U_ARRAY_NSLOTS_DFL 512
#endif

#ifndef U_ARRAY_RESIZE_PAD
#define U_ARRAY_RESIZE_PAD 100
#endif

typedef enum {
        U_ARRAY_TYPE_UNSET = 0,
#ifdef HAVE_BOOL
        U_ARRAY_TYPE_BOOL,
#endif  /* HAVE_BOOL */
        U_ARRAY_TYPE_CHAR,
        U_ARRAY_TYPE_UCHAR,
        U_ARRAY_TYPE_SHORT,
        U_ARRAY_TYPE_USHORT,
        U_ARRAY_TYPE_INT,
        U_ARRAY_TYPE_UINT,
        U_ARRAY_TYPE_LONG,
        U_ARRAY_TYPE_ULONG,
#ifdef HAVE_LONG_LONG
        U_ARRAY_TYPE_LONGLONG,
        U_ARRAY_TYPE_ULONGLONG,
#endif  /* HAVE_LONG_LONG */
        U_ARRAY_TYPE_FLOAT,
        U_ARRAY_TYPE_DOUBLE,
#ifdef HAVE_LONG_DOUBLE
        U_ARRAY_TYPE_LONGDOUBLE,
#endif  /* HAVE_LONG_DOUBLE */
        U_ARRAY_TYPE_CUSTOM,
        U_ARRAY_TYPE_MAX = U_ARRAY_TYPE_CUSTOM
} u_array_type_t;
         
#define U_ARRAY_TYPE_IS_VALID(t) \
    (t > U_ARRAY_TYPE_UNSET && t <= U_ARRAY_TYPE_CUSTOM)

struct u_array_s;
typedef struct u_array_s u_array_t;

/* [cd]tor */
int u_array_create (u_array_type_t t, size_t nslots, u_array_t **pda);
void u_array_free (u_array_t *da);

/* dyn resize */
int u_array_resize (u_array_t *da, size_t req_idx);

/* set */
int u_array_set_char (u_array_t *da, size_t idx, char v, char *pold);
int u_array_set_uchar (u_array_t *da, size_t idx, unsigned char v, 
        unsigned char *pold);
int u_array_set_short (u_array_t *da, size_t idx, short v, short *pold);
int u_array_set_ushort (u_array_t *da, size_t idx, unsigned short v, 
        unsigned short *pold);
int u_array_set_int (u_array_t *da, size_t idx, int v, int *pold);
int u_array_set_uint (u_array_t *da, size_t idx, unsigned int v, 
        unsigned int *pold);
int u_array_set_long (u_array_t *da, size_t idx, long v, long *pold);
int u_array_set_ulong (u_array_t *da, size_t idx, unsigned long v, 
        unsigned long *pold);
int u_array_set_float (u_array_t *da, size_t idx, float v, float *pold);
int u_array_set_double (u_array_t *da, size_t idx, double v, double *pold);
int u_array_set_custom (u_array_t *da, size_t idx, void *v, void **pold);

/* get */
int u_array_get_char (u_array_t *da, size_t idx, char *pv);
int u_array_get_uchar (u_array_t *da, size_t idx, unsigned char *pv);
int u_array_get_short (u_array_t *da, size_t idx, short *pv);
int u_array_get_ushort (u_array_t *da, size_t idx, unsigned short *pv);
int u_array_get_int (u_array_t *da, size_t idx, int *pv);
int u_array_get_uint (u_array_t *da, size_t idx, unsigned int *pv);
int u_array_get_long (u_array_t *da, size_t idx, long *pv);
int u_array_get_ulong (u_array_t *da, size_t idx, unsigned long *pv);
int u_array_get_float (u_array_t *da, size_t idx, float *pv);
int u_array_get_double (u_array_t *da, size_t idx, double *pv);
int u_array_get_custom (u_array_t *da, size_t idx, void **pv);

#ifdef HAVE_BOOL
/* set */
int u_array_set_bool (u_array_t *da, size_t idx, bool v, bool *pold);
/* get */
int u_array_get_bool (u_array_t *da, size_t idx, bool *pv);
#endif  /* HAVE_BOOL */

#ifdef HAVE_LONG_LONG
/* set */
int u_array_set_longlong (u_array_t *da, size_t idx, long long v, 
        long long *pold);
int u_array_set_ulonglong (u_array_t *da, size_t idx, unsigned long long v, 
        unsigned long long *pold);
/* get */
int u_array_get_longlong (u_array_t *da, size_t idx, long long *pv);
int u_array_get_ulonglong (u_array_t *da, size_t idx, unsigned long long *pv);
#endif  /* HAVE_LONG_LONG */

#ifdef HAVE_LONG_DOUBLE
/* set */
int u_array_set_longdouble (u_array_t *da, size_t idx, long double v, 
        long double *pold);
/* get */
int u_array_get_longdouble (u_array_t *da, size_t idx, long double *pv);
#endif  /* HAVE_LONG_DOUBLE */

#ifdef __cplusplus
}
#endif

#endif /* !_U_ARRAY_H_ */
