/* 
 * Copyright (c) 2005-2008 by KoanLogic s.r.l. - All rights reserved.  
 */

#ifndef _U_MISC_H_
#define _U_MISC_H_

#include <u/libu_conf.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>


#ifdef __cplusplus
extern "C" {
#endif

struct sockaddr; /* forward declaration */

/* define U_PATH_MAX */
#ifndef PATH_MAX
#define U_PATH_MAX 4096
#else
#define U_PATH_MAX PATH_MAX
#endif

/* define U_NAME_MAX */
#ifdef NAME_MAX
#define U_NAME_MAX NAME_MAX
#else
#ifdef FILENAME_MAX
#define U_NAME_MAX FILENAME_MAX
#else
#define U_NAME_MAX 512
#endif 
#endif

/* define U_FILENAME_MAX (path + name) */
#define U_FILENAME_MAX (U_PATH_MAX + U_NAME_MAX)

#define U_ONCE if (({ static int __x = 0; int __y; __y = __x; __x = 1; !__y;}))
#define U_SSTRCPY(to, from) u_sstrncpy((to), (from), sizeof(to) - 1)
#define U_FREE(ptr) do {if (ptr) { u_free(ptr); ptr = NULL; }} while (0)
#define U_FREEF(ptr, func) do {if (ptr) { func(ptr); ptr = NULL; }} while (0)
#define U_CLOSE(fd) do {if (fd != -1) { close(fd); fd = -1; }} while (0)
#define U_FCLOSE(fp) do {if (fp) { fclose(fp); fp = NULL; }} while (0)
#define U_PCLOSE(pp) do {if (pp) { pclose(pp); pp = NULL; }} while (0)
#define U_MIN(a, b) ((a) < (b) ? (a) : (b))
#define U_MAX(a, b) ((a) > (b) ? (a) : (b))
#define u_unused_args(...) u_use_unused_args(NULL, __VA_ARGS__)

/* I/O */
typedef ssize_t (*iof_t) (int, void *, size_t);

int u_io (iof_t f, int sd, void *buf, size_t l, ssize_t *n, int *iseof);
int u_isnl (int c);
void u_trim (char *s);
int u_isblank (int c);
int u_isblank_str (const char *ln);
char *u_strndup (const char *s, size_t len);
char *u_strdup (const char *s);
int u_savepid (const char *pf);
char *u_sstrncpy (char *dst, const char *src, size_t size);
void* u_memdup (const void *src, size_t size);
int u_tokenize (char *wlist, const char *delim, char **tokv, size_t tokv_sz);
int u_snprintf (char *str, size_t size, const char *fmt, ...);
int u_path_snprintf (char *str, size_t size, char sep, const char *fmt, ...);
void u_use_unused_args (char *dummy, ...);
int u_data_is_bin (char *data, size_t sz);
int u_data_dump (char *data, size_t sz, const char *file);
int u_load_file (const char *path, size_t sz_max, char **pbuf, size_t *psz);
ssize_t u_read(int fd, void *buf, size_t size);
ssize_t u_write(int fd, void *buf, size_t size);
int u_accept(int s, struct sockaddr *addr, int *addrlen);
int u_sleep(unsigned int secs);


#ifdef __cplusplus
}
#endif

#endif /* !_U_MISC_H_ */
