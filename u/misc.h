/* 
 * Copyright (c) 2005, KoanLogic s.r.l. - All rights reserved.  
 */

#ifndef _U_MISC_H_
#define _U_MISC_H_

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define U_ONCE if (({ static int __x = 0; int __y; __y = __x; __x = 1; !__y;}))
#define U_SSTRCPY(to, from) u_sstrncpy((to), (from), sizeof(to) - 1)
#define U_FREE(ptr) do {if (ptr) { free(ptr); ptr = NULL; }} while (0)
#define U_CLOSE(fd) do {if (fd != -1) { close(fd); fd = -1; }} while (0)
#define U_FCLOSE(fp) do {if (fp) { fclose(fp); fp = NULL; }} while (0)
#define U_PCLOSE(pp) do {if (pp) { pclose(pp); pp = NULL; }} while (0)

int u_isnl(int c);
void u_trim(char *s);
int u_isblank(int c);
int u_isblank_str(const char *ln);
char *u_strndup(const char *s, size_t len);
char *u_strdup(const char *s);
int u_savepid (const char *pf);
char *u_sstrncpy (char *dst, const char *src, size_t size);
void* u_memdup(void *src, size_t size);
int u_tokenize (char *wlist, const char *delim, char **tokv, size_t tokv_sz);

#endif /* !_U_MISC_H_ */
