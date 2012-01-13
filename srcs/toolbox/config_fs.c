/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.  
 */

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#include <toolbox/carpal.h>
#include <toolbox/config.h>
#include <toolbox/misc.h>
#include <toolbox/memory.h>
#include <toolbox/str.h>

static int drv_fs_open (const char *path, void **parg)
{
    int fd = -1;
    FILE *file = NULL;

    dbg_return_if (path == NULL, ~0);
    dbg_return_if (parg == NULL, ~0);

    /* open the file */
again:
    fd = open(path, O_RDONLY);
    if(fd == -1 && (errno == EINTR))
        goto again; /* interrupted */

    crit_err_sif(fd < 0);

    /* stdio will handle buffering */
    file = fdopen(fd, "r");
    dbg_err_if(file == NULL);

    *parg = file;

    return 0;
err:
    if(file)
        fclose(file);
    if(fd >= 0)
        close(fd);
    return ~0;
}

static int drv_fs_close (void *arg)
{
    int rc;
    FILE *file = (FILE *) arg;

    dbg_return_if (file == NULL, ~0);

    if ((rc = fclose(file)) != 0)
        dbg_strerror(errno);

    return (rc == 0) ? 0 : ~0;
}

static char *drv_fs_gets (void *arg, char *buf, size_t size)
{
    FILE *file = (FILE *) arg;

    dbg_return_if (file == NULL, NULL);
    dbg_return_if (buf == NULL, NULL);

    return fgets(buf, size, file);
}

/* pre-set driver for filesystem access */
u_config_driver_t u_config_drv_fs = 
{ 
    drv_fs_open,
    drv_fs_close,
    drv_fs_gets,
    NULL    /* no resolver */
};
