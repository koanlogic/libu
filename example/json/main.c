#include <u/libu.h>
#include "json.h"

int facility = LOG_LOCAL0;

int main (int ac, char *av[])
{
    json_lex_t *jl = NULL;
    json_obj_t *jo = NULL;

    con_err_ifm (ac != 2, "%s <...>", av[0]);

    con_err_if (json_lex_new(av[1], &jl));
    con_err_if (json_lex(jl, &jo));
    json_lex_free(jl), jl = NULL;

    /* Print out what has been parsed. */
    json_obj_print(jo);
    json_obj_free(jo);

    return EXIT_SUCCESS;
err:
    if (jl) 
        u_con("%s", json_lex_geterr(jl));

    json_lex_free(jl);
    json_obj_free(jo);

    return EXIT_FAILURE;
}
