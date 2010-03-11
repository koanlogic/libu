#include <stdlib.h>
#include <unistd.h>
#include <u/libu.h>
#include "bst.h"

#define NELEMS  10


int facility = LOG_LOCAL0;

static void printer (u_bst_node_t *node, void *dummy);
static int sort_random (int howmany);
static int search (void);

int main (void)
{
    con_err_if (sort_random(NELEMS));
    con_err_if (search());

    return 0;
err:
    return 1;
}

/* TODO randomize the push ! */
static int search (void)
{
    char c;
    u_bst_t *bst = NULL;
    u_bst_node_t *node;
    enum { A = 33, B = 125, C = (B - A) };
    
    con_err_if (u_bst_new(U_BST_OPT_NONE, &bst));

    /* printable chars except '~' */
    for (c = B; c >= A; c--)
    {
        char key[2];

//        (void) u_snprintf(key, sizeof key, "%c", ((c % C + 11) % C) + A);
        (void) u_snprintf(key, sizeof key, "%c", c);
        con_err_if (u_bst_push(bst, key, NULL));
    }

    for (c = A; c <= B; c++)
    {
        char key[2];

        (void) u_snprintf(key, sizeof key, "%c", c);
        con_err_ifm ((node = u_bst_search(bst, key)) == NULL, 
                "key \'%s\' not found", key);
        con_err_if (strcmp((const char *) u_bst_node_key(node), key));
    }

    /* search for '~' that wasn't pushed */
    con_err_if (u_bst_search(bst, "~") != NULL);

    u_bst_free(bst);

    return 0;
err:
    u_bst_free(bst);
    return ~0;
}

static int sort_random (int howmany)
{
    int i;
    u_bst_t *bst = NULL;

    srandom((unsigned long) getpid()); 

    /* push new nodes to the top */
    con_err_if (u_bst_new(U_BST_OPT_PUSH_TOP, &bst));

    for (i = 0; i < howmany; i++)
    {
        char key[128];

        (void) u_snprintf(key, sizeof key, "%12.12d", random());
        con_err_if (u_bst_push(bst, key, NULL));
    }

    u_con("number of nodes in BST: %zu", u_bst_count(bst));

    (void) u_bst_foreach(bst, printer, NULL);

    u_bst_free(bst);

    return 0;
err:
    u_bst_free(bst);
    return ~0;
}

static void printer (u_bst_node_t *node, void *dummy)
{
    u_unused_args(dummy);

    if (node)
        u_con("[SORT] key: %s", (const char *) u_bst_node_key(node));
    return;
}
