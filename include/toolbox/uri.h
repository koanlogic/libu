/*
 * Copyright (c) 2005-2008 by KoanLogic s.r.l. - All rights reserved.
 */

#ifndef _U_URI_H_
#define _U_URI_H_

#include <u/libu_conf.h>

#ifdef __cplusplus
extern "C" {
#endif

struct u_uri_s
{
    char *scheme;
    char *user;
    char *pwd;
    char *host;
    int port;
    char *path;
};

typedef struct u_uri_s u_uri_t;

int u_uri_parse (const char *s, u_uri_t **pu);
void u_uri_free (u_uri_t *uri);

#ifdef __cplusplus
}
#endif

#endif /* !_U_URI_H_ */ 
