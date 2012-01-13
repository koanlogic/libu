/*
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.
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
    \defgroup env Environment
    \{
        The basic idea behind the \ref env module is to load configuration 
        values (i.e. values that a program needs at run-time) from a shell 
        script which exports some variables in a given namespace (let's say
        \c PFX_).  

        We let the shell act in the background - using just some \c env(1) and 
        \c pipe(2) tricks to redirect and capture the variables - which in turn
        gives us a great deal of power and flexibility in just a couple of C 
        lines. 

        The following is a very simple configuration file in which three 
        variables are set and \c export'ed to the environment:
    \code
    SET_PFX_VAR1=""

    [ ! -z "$SET_PFX_VAR1" ] && export PFX_VAR1="tip"
    export PFX_VAR2="tap"
    export PFX_VAR3=$((10 * 100 * 1000))
    \endcode
        Things worth noting are:  
        - the conditional behaviour by which single or sets of variables
          can be pulled in/out;
        - bash does the math to eval \c PFX_VAR3;
        - bash can spawn external utilities;
        - it is possible to do complex substitutions;
        - every information one can get through the shell 
          (e.g. <code>cat /proc/something</code>) is available;
        - aggregation of other configuration modules is trivial via the 
          \c source (aka \c ".") shell builtin(1).

        The C code needed to access the former configuration file is:
    \code
    size_t i;
    const char *v, *vp[] = { "PFX_VAR1", "PFX_VAR2", "PFX_VAR3" };

    // load, parse, eval, etc. variables with PFX_ prefix from my.conf 
    // into the process environment
    dbg_err_if (u_env_init("PFX_", "./my.conf"));

    // access PFX_VAR{1,2,3} variables and print their values
    for (i = 0; i < 3; ++i)
    {
        v = u_env_var(vp[i]);

        con("%s = %s", vp[i], v ? v : "UNSET");
    }
    \endcode

        \note   Configuration files handled by the \ref env module are flat.
                If you need tree-like structured configuration files use the
                \ref config module instead.
 */

/**
 *  \brief  Load configuration environment
 *
 *  \param  prefix  variables namespace
 *  \param  cfile   configuration file
 *
 *  Load all configuration variables in the given \p prefix namespace from
 *  the configuration file \p cfile into the environment of the calling process.
 *  Complex parameter substitution, conditional evaluations, arithmetics, etc. 
 *  is done by the shell: the caller has a simple name=value view of the 
 *  configuration file.
 *
 *  \retval  0  on success 
 *  \retval ~0  on failure 
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

    dbg_err_if (u_snprintf(pcmd, BUFSZ, ". %s 2>/dev/null && env", cfile));

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
 *  \brief  Get a configuration variable value 
 * 
 *  Return a configuration variable by its name \p name
 * 
 *  \param  name    the name of the variable to get 
 *
 *  \return  the value of the requested variable or \c NULL if the variable
 *           is not defined
 */
const char *u_env_var (const char *name)
{
    return getenv(name);
}

/**
 *  \}
 */
