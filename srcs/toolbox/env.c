/*
 * Copyright (c) 2005-2010 by KoanLogic s.r.l. - All rights reserved.
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <toolbox/env.h>
#include <toolbox/carpal.h>
#include <toolbox/misc.h>

/**
 *  \defgroup env Environment
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
    struct stat sb;
    char line[BUFSZ], pcmd[BUFSZ], *val;
    FILE *pi = NULL;

    dbg_return_if (cfile == NULL || prefix == NULL, ~0);

    /* if 'cfile' does not exist bail out */
    dbg_err_sifm (stat(cfile, &sb) == -1, "%s", cfile);

    snprintf(pcmd, BUFSZ, ". %s 2>/dev/null && env", cfile);

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
