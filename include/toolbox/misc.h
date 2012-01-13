/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.  
 */

#ifndef _U_MISC_H_
#define _U_MISC_H_

#include <u/libu.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \addtogroup misc
 *  \{
 */

/** \brief  Maximum path length */
#ifndef PATH_MAX
  #define U_PATH_MAX 4096
#else
  #define U_PATH_MAX PATH_MAX
#endif

/** \brief  Maximum file name length */
#ifdef NAME_MAX
  #define U_NAME_MAX NAME_MAX
#else
  #ifdef FILENAME_MAX
    #define U_NAME_MAX FILENAME_MAX
  #else
    #define U_NAME_MAX 512
  #endif 
#endif

/** \brief  Maximum length for a fully qualified file name */
#define U_FILENAME_MAX (U_PATH_MAX + U_NAME_MAX)

#define U_CLOSE(fd) do {if (fd != -1) { close(fd); fd = -1; }} while (0)
#define U_FCLOSE(fp) do {if (fp) { fclose(fp); fp = NULL; }} while (0)
#define U_FREE(ptr) do { u_free(ptr); ptr = NULL; } while (0)
#define U_FREEF(ptr, func) do {if (ptr) { func(ptr); ptr = NULL; }} while (0)
#define U_MAX(a, b) ((a) > (b) ? (a) : (b))
#define U_MIN(a, b) ((a) < (b) ? (a) : (b))
#define U_ONCE if (({ static int __x = 0; int __y; __y = __x; __x = 1; !__y;}))
#define U_PCLOSE(pp) do {if (pp) { pclose(pp); pp = NULL; }} while (0)
#define U_SSTRCPY(to, from) u_sstrncpy((to), (from), sizeof(to) - 1)
#define u_unused_args(...) u_use_unused_args(NULL, __VA_ARGS__)

#define u_timersub(t1, t0, delta)                           \
    do {                                                    \
        (delta)->tv_sec = (t1)->tv_sec - (t0)->tv_sec;      \
        (delta)->tv_usec = (t1)->tv_usec - (t0)->tv_usec;   \
        if ((delta)->tv_usec < 0)                           \
        {                                                   \
            (delta)->tv_sec--;                              \
            (delta)->tv_usec += 1000000;                    \
        }                                                   \
     } while (0)

/** \brief  Prototype for an I/O driver function used by ::u_io, e.g. \c read(2)
 *          or \c write(2) */
typedef ssize_t (*iof_t) (int, void *, size_t);

char *u_strdup (const char *s);
char *u_strndup (const char *s, size_t len);
int u_atol (const char *nptr, long *pl);
int u_atoi (const char *nptr, int *pi);
int u_atof (const char *nptr, double *pd);
#ifdef HAVE_STRTOUMAX
#include <inttypes.h>
int u_atoumax (const char *nptr, uintmax_t *pumax);
#endif  /* HAVE_STRTOUMAX */
int u_data_dump (char *data, size_t sz, const char *file);
int u_data_is_bin (char *data, size_t sz);
int u_io (iof_t f, int sd, void *buf, size_t l, ssize_t *n, int *iseof);
int u_isblank (int c);
int u_isblank_str (const char *ln);
int u_isnl (int c);
int u_load_file (const char *path, size_t sz_max, char **pbuf, size_t *psz);
int u_path_snprintf (char *str, size_t size, char sep, const char *fmt, ...);
int u_savepid (const char *pf);
int u_sleep (unsigned int secs);
int u_snprintf (char *str, size_t size, const char *fmt, ...);
int u_strlcat (char *dst, const char *src, size_t size);
int u_strlcpy (char *dst, const char *src, size_t size);
int u_strtok (const char *s, const char *delim, char ***ptv, size_t *pnelems);
ssize_t u_read (int fd, void *buf, size_t size);
ssize_t u_write (int fd, void *buf, size_t size);
void u_strtok_cleanup (char **tv, size_t nelems);
void u_trim (char *s);
void u_use_unused_args (char *dummy, ...);
void *u_memdup (const void *src, size_t size);

/* depreacated interfaces */
int u_tokenize (char *wlist, const char *delim, char **tokv, size_t tokv_sz) \
    __LIBU_DEPRECATED;
char *u_sstrncpy (char *dst, const char *src, size_t size) \
    __LIBU_DEPRECATED;

/**
 *  \}
 */

#ifdef __cplusplus
}
#endif

#endif /* !_U_MISC_H_ */
