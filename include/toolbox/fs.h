/*
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.
 */

#ifndef _U_FS_H_
#define _U_FS_H_

#include <u/libu_conf.h>

#ifdef __cplusplus
extern "C" {
#endif

int u_move(const char *src, const char *dst);
int u_copy(const char *src, const char *dst);
int u_remove(const char *file);

#ifdef __cplusplus
}
#endif

#endif /* !_U_FS_H_ */
