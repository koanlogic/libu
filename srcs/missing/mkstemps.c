/*
 * Copyright (c) 1987, 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by the University of
 *  California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <u/missing/mkstemps.h>

#ifdef OS_WIN
#include <windows.h>
#define __mkdir(dir, perm) CreateDirectory(dir, NULL)
#define lstat(path, buf) stat(path, buf)
#else
#define __mkdir(dir, perm) mkdir(dir, perm)
#endif

#ifndef HAVE_MKSTEMPS

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

static int _gettemp(char *, int *, int, int);
static unsigned int __time_seed (void);

static const unsigned char padchar[] =
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

int mkstemps(char *path, int slen)
{
    int fd;

    return (_gettemp(path, &fd, 0, slen) ? fd : -1);
}

static int _gettemp(char *path, int *doopen, int domkdir, int slen)
{
    char *start, *trv, *suffp;
    char *pad;
    struct stat sbuf;
    int rval;
    int __rand;

    srand(__time_seed());

    if (doopen != NULL && domkdir) {
        errno = EINVAL;
        return (0);
    }

    for (trv = path; *trv != '\0'; ++trv)
        ;
    trv -= slen;
    suffp = trv;
    --trv;
    if (trv < path) {
        errno = EINVAL;
        return (0);
    }

    /* Fill space with random characters */
    while (trv >= path && *trv == 'X') {
        __rand = rand() % (sizeof(padchar) - 1);
        *trv-- = padchar[__rand];
    }
    start = trv + 1;

    /*
     * check the target directory.
     */
    if (doopen != NULL || domkdir) {
        for (; trv > path; --trv) {
            if (*trv == '/') {
                *trv = '\0';
                rval = stat(path, &sbuf);
                *trv = '/';
                if (rval != 0)
                    return (0);
                if (!S_ISDIR(sbuf.st_mode)) {
                    errno = ENOTDIR;
                    return (0);
                }
                break;
            }
        }
    }

    for (;;) {
        if (doopen) {
            if ((*doopen = open(path, O_CREAT|O_EXCL|O_RDWR, 0600)) >= 0)
                return (1);
            if (errno != EEXIST)
                return (0);
        } else if (domkdir) {
            if (__mkdir(path, 0700) == 0)
                return (1);
            if (errno != EEXIST)
                return (0);
        } else if (lstat(path, &sbuf))
            return (errno == ENOENT);

        /* If we have a collision, cycle through the space of filenames */
        for (trv = start;;) {
            if (*trv == '\0' || trv == suffp)
                return (0);
            pad = strchr((const char *) padchar, *trv);
            if (pad == NULL || *++pad == '\0')
                *trv++ = padchar[0];
            else {
                *trv++ = *pad;
                break;
            }
        }
    }
    /*NOTREACHED*/
}

static unsigned int __time_seed (void)
{
    time_t now;
    unsigned char *p;
    unsigned seed = 0;
    size_t i;

    now = time(NULL);
    p = (unsigned char *) &now;

    for (i = 0; i < sizeof now; i++)
        seed = seed * (UCHAR_MAX + 2U) + p[i];

    return seed;
}

#else   /* HAVE_MKSTEMPS */
int mkstemps (char *template, int suffixlen);
#endif  /* !HAVE_MKSTEMPS */
