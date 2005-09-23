/*
 * Copyright (c) 2005, KoanLogic s.r.l. - All rights reserved.
 */

static const char rcsid[] =
    "$Id: env.c,v 1.1.1.1 2005/09/23 13:04:38 tho Exp $";

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <u/env.h>
#include <u/debug.h>
#include <u/misc.h>

/**
 *  \defgroup env Configuration Through Environment 
 *  \{
 */

/**
 * \brief   Load configuration environment
 *
 * \param   prefix  variables namespace
 * \param   cfile   configuration file
 *
 * Load all configuration variables in the given \p prefix namespace from
 * the configuration file \p cfile into the environment of the calling process.
 * Complex parameter substitution, conditional evaluations, arithmetics, etc. 
 * is done by the shell: the caller has a simple name=value view of the 
 * configuration file.
 *
 * \return 
 * - \c 0 on success
 * - non-zero on failure
 */
int u_env_init (const char *prefix, const char *cfile)
{
    enum { BUFSZ = 1024 };
    char line[BUFSZ], pcmd[BUFSZ], *val;
    FILE *pi = NULL;

    dbg_return_if (cfile == NULL || prefix == NULL, ~0);

    snprintf(pcmd, BUFSZ, ". %s 2>/dev/null && printenv", cfile);

    dbg_err_if ((pi = popen(pcmd, "r")) == NULL);

    while(fgets(line, BUFSZ-1, pi))
    {
        if(strncmp(line, prefix, strlen(prefix)) == 0)
        {
            line[strlen(line)-1] = 0;
            val = strchr(line, '=');
            if(!val)
                continue; /* should never happen... */
            *val++ = 0;
            /* line is the name and val the value */
            dbg_err_if(setenv(line, val, 1));
        }
    }

    pclose(pi);
    return 0;

err:
    U_PCLOSE(pi);
    return ~0;
}

/** 
 * \brief   Get a configuration variable value 
 * 
 * Return a configuration variable
 * 
 * \param   name    the name of the variable to get 
 *
 * \return  the value of the requested variable or \c NULL if the variable
 *          is not defined
 */
const char *u_env_var (const char *name)
{
    return getenv(name);
}

/**
 *      \}
 */
