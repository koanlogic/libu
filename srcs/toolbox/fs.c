/*
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <u/libu_conf.h>
#include <toolbox/fs.h>
#include <toolbox/carpal.h>
#include <toolbox/misc.h>

/**
    \defgroup fs File system
    \{
        The \ref fs module provides a small set of interfaces to do basic
        file juggling tasks: move, copy an remove.
 */

/** 
 *  \brief  Copy a file
 * 
 *  Copy the file whose path name is \p src to \p dst
 * 
 *  \param  src source file name
 *  \param  dst destination file name
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_copy (const char *src, const char *dst)
{
    FILE *sfp = NULL, *dfp = NULL;
    size_t c;
    char buf[4096];

    dbg_return_if (src == NULL, ~0);
    dbg_return_if (dst == NULL, ~0);

    sfp = fopen(src, "rb");
    dbg_err_sifm(sfp == NULL, "unable to open %s for reading", src);

    dfp = fopen(dst, "wb+");
    dbg_err_sifm(dfp == NULL, "unable to open %s for writing", dst);

    while((c = fread(buf, 1, sizeof(buf), sfp)) > 0)
    {
        dbg_err_sifm(fwrite(buf, 1, c, dfp) == 0, "error writing to %s", dst);
    }

    dbg_err_sif(fclose(sfp)); sfp = NULL;

    dbg_err_sifm(fclose(dfp), "error flushing %s", dst); dfp = NULL;

    return 0;
err:
    if(sfp)
        fclose(sfp);
    if(dfp)
        fclose(dfp);
    return ~0;
}

/**
 *  \brief  Move a file
 * 
 *  Move the file whose path name is \p src to \p dst
 * 
 *  \param  src source file name
 *  \param  dst destination file name
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_move(const char *src, const char *dst)
{
    int rc;

    dbg_return_if (src == NULL, ~0);
    dbg_return_if (dst == NULL, ~0);

#ifdef HAVE_LINK
    dbg_err_sif((rc = link(src, dst)) < 0 && errno != EXDEV);

    if(rc && errno == EXDEV)
        dbg_err_if(u_copy(src, dst)); /* deep copy */
#else
    u_unused_args(rc);

    dbg_err_if(u_copy(src, dst));
#endif

    dbg_if(u_remove(src) < 0);

    return 0;
err:
    return ~0;
}

/**
 *  \brief  Remove a file
 * 
 *  Unlink the file whose path name is \p file
 * 
 *  \param  file    the file to remove
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_remove(const char *file)
{
    dbg_return_if (file == NULL, ~0);

    dbg_err_sif(unlink(file) < 0);

    return 0;
err:
    return ~0;
}

/**
 *  \}
 */
