#include <stdlib.h>
#include <unistd.h>
#include <u/libu.h>
#include "bst.h"

#define NELEMS  11

int facility = LOG_LOCAL0;

static int intcmp (const void *i, const void *j);
static void print_string (u_bst_node_t *node, void *dummy);
static void print_int (u_bst_node_t *node, void *dummy);
static int sort_random (int howmany);
static int randomized_push (int howmany);
static int search (void);
static int intkeys_balance (void);

int main (void)
{
    srandom((unsigned long) getpid()); 

    getchar();
    con_err_if (intkeys_balance());
    con_err_if (sort_random(NELEMS));
    con_err_if (search());
    con_err_if (randomized_push(NELEMS));

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

static int randomized_push (int howmany)
{
    int i;
    u_bst_t *bst = NULL;

    con_err_if (u_bst_new(U_BST_OPT_RANDOMIZED, &bst));

    /* see if a sequential insert sequence maps into a fairly balanced bst */
    for (i = 0; i < howmany; i++)
    {
        char key[128];

        (void) u_snprintf(key, sizeof key, "%d", i);
        con_err_if (u_bst_push(bst, key, NULL));
    }

    (void) u_bst_foreach(bst, print_string, NULL);

    //u_bst_free(bst);

    return 0;
err:
    u_bst_free(bst);
    return ~0;
}

static int sort_random (int howmany)
{
    int i;
    u_bst_t *bst = NULL;

    /* always push new nodes to the top */
    con_err_if (u_bst_new(U_BST_OPT_PUSH_TOP, &bst));

    for (i = 0; i < howmany; i++)
    {
        char key[128];

        (void) u_snprintf(key, sizeof key, "%12.12d", random());
        con_err_if (u_bst_push(bst, key, NULL));
    }

    u_con("number of nodes in BST: %zu", u_bst_count(bst));

    (void) u_bst_foreach(bst, print_string, NULL);

    u_bst_free(bst);

    return 0;
err:
    u_bst_free(bst);
    return ~0;
}

static void print_string (u_bst_node_t *node, void *dummy)
{
    u_unused_args(dummy);

    u_con("[SORT] key: %s (weight: %zu)", 
            (const char *) u_bst_node_key(node), u_bst_node_count(node));

    return;
}

static void print_int (u_bst_node_t *node, void *dummy)
{
    u_unused_args(dummy);

    u_con("[SORT] key: %d (weight: %zu)",
            *((const int *) u_bst_node_key(node)), u_bst_node_count(node));

    return;
}

static int intcmp (const void *i, const void *j)
{
    return *((const int *) i) - *((const int *) j);
}

static int intkeys_balance (void)
{
    int key;
    size_t i;
    u_bst_t *bst = NULL;
    u_bst_node_t *node;

    con_err_if (u_bst_new(U_BST_OPT_NONE, &bst));
    con_err_if (u_bst_set_cmp(bst, intcmp));
    con_err_if (u_bst_set_keyattr(bst, U_BST_TYPE_OPAQUE, sizeof(int)));

    for (i = 0; i < NELEMS; i++)
    {
        key = (int) random();
        con_err_if (u_bst_push(bst, (const void *) &key, NULL));
    }

    (void) u_bst_foreach(bst, print_int, NULL);
    
    /* search 4th and 5th smallest key */
    for (i = 3; i < 5; i++)
    {
        con_err_if ((node = u_bst_find_nth(bst, i)) == NULL);
        u_con("%zu-th key is %d", i + 1, *((const int *) u_bst_node_key(node)));
    }

    /* delete last inserted key */
    u_con("deleting %d", key);
    con_err_if (u_bst_delete(bst, (const void *) &key));

    /* try again (should fail) */
    con_err_if (!u_bst_delete(bst, (const void *) &key));

    (void) u_bst_foreach(bst, print_int, NULL);

    u_con("balance!");
    con_err_if (u_bst_balance(bst));

    (void) u_bst_foreach(bst, print_int, NULL);

    u_bst_free(bst);

    return 0;
err:
    u_bst_free(bst);
    return ~0;
}
