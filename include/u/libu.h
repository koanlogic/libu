/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.  
 */

#ifndef _U_LIBU_H_
#define _U_LIBU_H_

#include <u/libu_conf.h>

#include <stdio.h>

#ifdef HAVE_STDLIB
  #include <stdlib.h>
#endif

/* GNUC __attribute__((deprecated)) (gcc 3.1 and later) */
#if defined(__GNUC__) && ((__GNUC__ >= 4) || \
        ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1)))
  #define __LIBU_DEPRECATED __attribute__((deprecated))
#else
  #define __LIBU_DEPRECATED
#endif

#include <u/missing.h>
#include <u/toolbox.h>
#include <u/compat.h>

#endif /* !_U_LIBU_H_ */
