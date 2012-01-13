/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.  
 */
#ifndef _LIBU_VA_H_
#define _LIBU_VA_H_
#include <u/libu_conf.h>

#if defined(va_copy)
   /* C99 va_copy */
   #define u_va_copy(a, b)  va_copy(a, b)
#elif defined(__va_copy)
   /* GNU libc va_copy replacement */
   #define u_va_copy(a, b)  __va_copy(a, b)
#else
  #error "va_copy is not defined"
#endif

#endif
