/*
 * Copyright (c) 2005, KoanLogic s.r.l. - All rights reserved.
 */

#ifndef _U_ENV_H_
#define _U_ENV_H_
#include "libu_conf.h"

int u_env_init (const char *prefix, const char *cfile);
const char *u_env_var (const char *name);

#endif /* !_U_ENV_H_ */
