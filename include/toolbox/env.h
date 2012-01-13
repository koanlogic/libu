/*
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.
 */

#ifndef _U_ENV_H_
#define _U_ENV_H_

#include <u/libu_conf.h>

#ifdef __cplusplus
extern "C" {
#endif

int u_env_init (const char *prefix, const char *cfile);
const char *u_env_var (const char *name);

#ifdef __cplusplus
}
#endif

#endif /* !_U_ENV_H_ */
