/* $Id: main.c,v 1.2 2006/12/22 14:23:54 stewy Exp $ */

#include <u/libu.h>

int facility = LOG_LOCAL0;

static int example_dynamic(void);
static int example_static(void);

int main()
{
    con_err_if (example_static());
    con_err_if (example_dynamic());

    return 0;
err:    
    return ~0;
}

static int example_static()
{
    u_hmap_opts_t *opts = NULL;
    u_hmap_t *hmap = NULL;
    int fibonacci[] = { 0, 1, 1, 2, 3, 5, 8, 13, 21 },
        *x;

    con("example_static()");

    /* initialise options and hmap */
    con_err_if (u_hmap_opts_new(&opts));
    opts->f_free = NULL;                    /* static allocation */

    con_err_if (u_hmap_new(opts, &hmap));

    /* insert some sample elements */
    con_err_if (u_hmap_put(hmap, "first", (void *) &fibonacci[0]));
    con_err_if (u_hmap_put(hmap, "fifth", (void *) &fibonacci[4])); 
    con_err_if (u_hmap_put(hmap, "last", (void *) 
                &fibonacci[(sizeof(fibonacci)/sizeof(int))-1]));

    /* retrieve and print values to console */
    con_err_if (u_hmap_get(hmap, "last", (void **) &x)); 
    con("hmap['last'] = %d", *x);
    con_err_if (u_hmap_get(hmap, "fifth", (void **) &x)); 
    con("hmap['fifth'] = %d", *x); 
    con_err_if (u_hmap_get(hmap, "first", (void **) &x)); 
    con("hmap['first'] = %d", *x);

    /* remove an element and replace it - will use default free() function */
    con_err_if (u_hmap_del(hmap, "fifth")); 

    /* check that it has been deleted */
    con_err_if (u_hmap_get(hmap, "fifth", (void **) &x) == 0); 

    /* replace with a new element and print it */
    con_err_if (u_hmap_put(hmap, "first", (void **) &x));
    con_err_if (u_hmap_get(hmap, "last", (void **) &x)); 
    con("hmap['last'] = %d", *x);

    /* free hmap (options are freed automatically) */
    u_hmap_free(hmap);
    
    return 0;
err:
    return ~0;
}

static int example_dynamic()
{
    u_hmap_opts_t *opts = NULL;
    u_hmap_t *hmap = NULL;
    char *s = NULL;

    con("example_dynamic()");

    /* initialise options and hmap */
    con_err_if (u_hmap_opts_new(&opts));
    con_err_if (u_hmap_new(opts, &hmap));

    /* insert some sample elements */
    con_err_if (u_hmap_put(hmap, "english", (void *) strdup("Hello world!")));
    con_err_if (u_hmap_put(hmap, "italian", (void *) strdup("Ciao mondo!")));
    con_err_if (u_hmap_put(hmap, "german", (void *) strdup("Hallo Welt!")));

    /* retrieve and print values to console */
    con_err_if (u_hmap_get(hmap, "italian", (void **) &s)); 
    con("hmap['italian'] = %s", s);
    con_err_if (u_hmap_get(hmap, "german", (void **) &s)); 
    con("hmap['german'] = %s", s); 
    con_err_if (u_hmap_get(hmap, "english", (void **) &s)); 
    con("hmap['english'] = %s", s);

    /* remove an element and replace it - will use default free() function */
    con_err_if (u_hmap_del(hmap, "german")); 

    /* check that it has been deleted */
    con_err_if (u_hmap_get(hmap, "german", (void **) &s) == 0); 

    /* replace with a new element and print it */
    con_err_if (u_hmap_put(hmap, "german", (void *) strdup("Auf Wiedersehen!")));
    con_err_if (u_hmap_get(hmap, "german", (void **) &s)); 
    con("hmap['german'] = %s", s);

    /* free hmap (options are freed automatically) */
    u_hmap_free(hmap);
    
    return 0;
err:
    return ~0;
}
