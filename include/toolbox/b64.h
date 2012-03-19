/*
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.
 */

#ifndef _U_B64_H_
#define _U_B64_H_

#include <u/libu_conf.h>

#ifdef __cplusplus
extern "C" {
#endif

int u_b64_encode (const uint8_t *in, size_t in_sz, char *out, size_t out_sz);
int u_b64_decode (const char *in, size_t in_sz, uint8_t *out, size_t *out_sz);

#ifdef __cplusplus
}
#endif

#endif /* !_U_B64_H_ */
