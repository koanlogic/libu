/* $Id: main.c,v 1.1 2006/12/20 21:56:04 stewy Exp $ */

#include <u/libu.h>

int facility = LOG_LOCAL0;

int main()
{
    u_hmap_opts_t *opts = NULL;
    u_hmap_t *hmap = NULL;
    char *s = NULL;

    /* initialise options and hmap */
    con_err_if (u_hmap_opts_new(&opts));
    con_err_if (u_hmap_new(opts, &hmap));

    /* insert some sample elements */
    con_err_if (u_hmap_put(hmap, "english", (void *) strdup("Hello world!")));
    con_err_if (u_hmap_put(hmap, "italian", (void *) strdup("Ciao mondo!")));
    con_err_if (u_hmap_put(hmap, "german", (void *) strdup("Hallo Welt!")));

    /* retrieve and print values to console */
    con_err_if (u_hmap_get(hmap, "italian", (void **) &s)); 
    con(s);
    con_err_if (u_hmap_get(hmap, "german", (void **) &s)); 
    con(s); 
    con_err_if (u_hmap_get(hmap, "english", (void **) &s)); 
    con(s);

    /* remove an element and replace it - will use default free() function */
    con_err_if (u_hmap_del(hmap, "german")); 

    /* check that it has been deleted */
    con_err_if (u_hmap_get(hmap, "german", (void **) &s) == 0); 

    /* replace with a new element and print it */
    con_err_if (u_hmap_put(hmap, "german", (void *) strdup("Auf Wiedersehen!")));
    con_err_if (u_hmap_get(hmap, "german", (void **) &s)); 
    con(s);

    /* free hmap (options are freed automatically) */
    u_hmap_free(hmap);
    
    return 0;

err:
    return ~0;
}
