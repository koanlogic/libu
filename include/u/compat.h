/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.  
 */

#ifndef _LIBU_COMPAT_H_
#define _LIBU_COMPAT_H_

#include <u/libu_conf.h>

#ifdef LIBU_1X_COMPAT

/* enable compatibility layer with the 1.X branch */ 

/* carpal module */
#define con(...)    u_con(__VA_ARGS__)
#define dbg(...)    u_dbg(__VA_ARGS__)
#define info(...)   u_info(__VA_ARGS__)
#define notice(...) u_notice(__VA_ARGS__)
#define warn(...)   u_warn(__VA_ARGS__)
#define err(...)    u_err(__VA_ARGS__)
#define crit(...)   u_crit(__VA_ARGS__)
#define alert(...)  u_alert(__VA_ARGS__)
#define emerg(...)  u_emerg(__VA_ARGS__)

/* uri module */
#define u_uri_parse(s, pu)  u_uri_crumble(s, 0, pu)

/* net module */
#define u_net_sock(uri, mode)   u_net_sd(uri, mode, 0)

#endif  /* LIBU_1X_COMPAT */

#endif  /* !_LIBU_COMPAT_H_ */
