#include <stdlib.h>
#include <u/libu.h>

int facility = LOG_LOCAL0;

static void usage(void)
{
    u_con("usage: uconfig [-s] FILE");
    exit(1);
}

static int cmp_by_key(u_config_t **pa, u_config_t **pb)
{
    u_config_t *a = *pa, *b = *pb;
    const char *va, *vb;

    if((va = u_config_get_key(a)) == NULL)
        va = "";
    
    if((vb = u_config_get_key(b)) == NULL)
        vb = "";

    return strcmp(va, vb);
}

int main(int argc, char **argv)
{
    u_config_t *c = NULL;
    const char *filename;
    int sort = 0;

    con_err_ifm(argc < 2, "usage: uconfig [-s] FILE");

    if(argv[1][0] == '-') 
    {
        filename = argv[2];
        if(strcmp(argv[1], "-s"))
            usage();
        sort++;
    } else
        filename = argv[1];

    con_err_if(u_config_load_from_file(filename, &c));

    if(sort)
        u_config_sort_children(c, cmp_by_key);

    u_config_print(c, 0);

    u_config_free(c);

    return 0;
err:
    return 1;
}
