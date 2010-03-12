#include <toolbox/carpal.h>
#include <toolbox/memory.h>
#include <toolbox/misc.h>

#include "bst.h"

struct u_bst_node_s
{
    void *key, *val;
    size_t nelem;
    struct u_bst_node_s *left, *right;
};

struct u_bst_s
{
    int opts;
    int (*cmp) (const void *, const void *);
    u_bst_type_t keytype, valtype;
    size_t keysize, valsize;
    void (*keyfree) (void *);
    void (*valfree) (void *);
    struct u_bst_node_s *root;
};

static int u_bst_keycmp (const void *a, const void *b);
static int u_bst_keycmp_dbg (const void *a, const void *b);
static void u_bst_genfree (void *p);
static int u_bst_node_new (u_bst_t *bst, const void *key, const void *val, 
        u_bst_node_t **pnode);
static void u_bst_node_free (u_bst_t *bst, u_bst_node_t *node);
static void u_bst_node_do_free (u_bst_t *bst, u_bst_node_t *node);
static int u_bst_assign (void **pdst, const void *src, u_bst_type_t t, 
        size_t sz);
static u_bst_node_t *u_bst_node_search (u_bst_node_t *node, const void *key, 
        int (*cmp)(const void *, const void *));
static void u_bst_node_foreach (u_bst_node_t *node, 
        void (*cb)(u_bst_node_t *, void *), void *cb_args);
static u_bst_node_t *u_bst_node_push_top (u_bst_t *bst, u_bst_node_t *node, 
        const void *key, const void *val);
static u_bst_node_t *u_bst_node_push_bottom (u_bst_t *bst, const void *key, 
        const void *val);
static u_bst_node_t *u_bst_node_push_rand (u_bst_t *bst, u_bst_node_t *node,
        const void *key, const void *val);
static u_bst_node_t *u_bst_node_push (u_bst_t *bst, u_bst_node_t *node, 
        const void *key, const void *val);
static u_bst_node_t *u_bst_node_find_nth (u_bst_node_t *node, size_t n);
static u_bst_node_t *u_bst_node_promote_nth (u_bst_node_t *node, size_t n);
static u_bst_node_t *u_bst_node_join_lr (u_bst_node_t *l, u_bst_node_t *r);
static u_bst_node_t *u_bst_node_delete (u_bst_t *bst, u_bst_node_t *node, 
        const void *key, int *pfound);
static u_bst_node_t *u_bst_node_join (u_bst_node_t *b1, u_bst_node_t *b2);
static u_bst_node_t *u_bst_node_balance (u_bst_node_t *node);


int u_bst_new (int opts, u_bst_t **pbst)
{
    u_bst_t *bst = NULL;

    dbg_return_if (pbst == NULL, ~0);

    warn_return_sif ((bst = u_malloc(sizeof *bst)) == NULL, ~0);

    bst->keytype = U_BST_TYPE_STRING;
    bst->keyfree = u_bst_genfree;
    bst->valtype = U_BST_TYPE_PTR;
    bst->valfree = NULL;
#ifdef BST_DEBUG
    bst->cmp = u_bst_keycmp_dbg;
#else
    bst->cmp = u_bst_keycmp;
#endif  /* BST_DEBUG */
    bst->root = NULL;
    bst->opts = opts;

    /* Seed the PRNG in case we need to handle randomized insertion. */
    if (bst->opts & U_BST_OPT_RANDOMIZED)
        srand((unsigned int) getpid()); 

    *pbst = bst;

    return 0;
}

void u_bst_free (u_bst_t *bst)
{
    if (bst)
    {
        u_bst_node_free(bst, bst->root);
        u_free(bst);
    }

    return;
}

int u_bst_push (u_bst_t *bst, const void *key, const void *val)
{
    dbg_return_if (bst == NULL, ~0);
    dbg_return_if (key == NULL, ~0);

    if (bst->opts & U_BST_OPT_RANDOMIZED)
        bst->root = u_bst_node_push_rand(bst, bst->root, key, val);
    else if (bst->opts & U_BST_OPT_PUSH_TOP)
        bst->root = u_bst_node_push_top(bst, bst->root, key, val);
    else /* The default is bottom insertion. */
        bst->root = u_bst_node_push_bottom(bst, key, val);

    return bst->root ? 0 : ~0;
}

int u_bst_delete (u_bst_t *bst, const void *key)
{
    int found = 0;

    dbg_return_if (bst == NULL, ~0);
    dbg_return_if (key == NULL, ~0);

    bst->root = u_bst_node_delete(bst, bst->root, key, &found);

    return found ? 0 : ~0;
}

u_bst_node_t *u_bst_search (u_bst_t *bst, const void *key)
{
    dbg_return_if (bst == NULL, NULL);
    dbg_return_if (key == NULL, NULL);

    return u_bst_node_search(bst->root, key, bst->cmp);
}

u_bst_node_t *u_bst_find_nth (u_bst_t *bst, size_t n)
{
    dbg_return_if (bst == NULL, NULL);

    return u_bst_node_find_nth(bst->root, n);
}

ssize_t u_bst_count (u_bst_t *bst)
{
    dbg_return_if (bst == NULL, (ssize_t) -1);

    if (bst->root == NULL)
        return 0;

    return bst->root->nelem;
}

int u_bst_foreach (u_bst_t *bst, void (*cb)(u_bst_node_t *, void *), 
        void *cb_args)
{
    dbg_return_if (bst == NULL, ~0);
    dbg_return_if (cb == NULL, ~0);

    u_bst_node_foreach(bst->root, cb, cb_args);

    return 0;
}

int u_bst_balance (u_bst_t *bst)
{
    dbg_return_if (bst == NULL, ~0);

    bst->root = u_bst_node_balance(bst->root);

    return 0;
}

/* return the new parent node (left or right child of the 'pivot', depending 
 * on 'dir'. */
u_bst_node_t *u_bst_rotate (u_bst_node_t *pivot, u_bst_rot_t dir)
{
    u_bst_node_t *newroot;    /* It will become the new parent node. */

    dbg_return_if (pivot == NULL, NULL);

    switch (dir)
    {
        /* Promote the right child. */
        case U_BST_ROT_LEFT:
            newroot = pivot->right;
            pivot->right = newroot->left;
            newroot->left = pivot;
            /* Update child nodes' counters. */
            newroot->nelem = pivot->nelem;
            pivot->nelem -= (newroot->right ? newroot->right->nelem + 1 : 1);
            break;

        /* Promote the left child. */
        case U_BST_ROT_RIGHT:
            newroot = pivot->left;
            pivot->left = newroot->right;
            newroot->right = pivot;
            /* Update child nodes' counters. */
            newroot->nelem = pivot->nelem;
            pivot->nelem -= (newroot->left ? newroot->left->nelem + 1 : 1);
            break;
    }

    return newroot;
}

int u_bst_set_keyattr (u_bst_t *bst, u_bst_type_t kt, size_t ks)
{
    dbg_return_if (bst == NULL, ~0);
    dbg_return_if (kt == U_BST_TYPE_OPAQUE && ks == 0, ~0);

    bst->keytype = kt;
    bst->keysize = ks;

    if (kt == U_BST_TYPE_OPAQUE)
        bst->keyfree = u_bst_genfree;

    return 0;
}

int u_bst_set_valattr (u_bst_t *bst, u_bst_type_t vt, size_t vs)
{
    dbg_return_if (bst == NULL, ~0);
    dbg_return_if (vt == U_BST_TYPE_OPAQUE && vs == 0, ~0);

    bst->valtype = vt;
    bst->valsize = vs;

    if (vt == U_BST_TYPE_OPAQUE)
        bst->valfree = u_bst_genfree;

    return 0;
}

int u_bst_set_cmp (u_bst_t *bst, int (*f)(const void *, const void *))
{
    dbg_return_if (bst == NULL, ~0);
    dbg_return_if (f == NULL, ~0);

    bst->cmp = f;

    return 0;
}

int u_bst_set_keyfree (u_bst_t *bst, void (*f)(void *))
{
    dbg_return_if (bst == NULL, ~0);
    dbg_return_if (f == NULL, ~0);

    bst->keyfree = f;

    return 0;
}

int u_bst_set_valfree (u_bst_t *bst, void (*f)(void *))
{
    dbg_return_if (bst == NULL, ~0);
    dbg_return_if (f == NULL, ~0);

    bst->valfree = f;

    return 0;
}

const void *u_bst_node_key (u_bst_node_t *node)
{
    dbg_return_if (node == NULL, NULL);

    return node->key;
}

const void *u_bst_node_val (u_bst_node_t *node)
{
    dbg_return_if (node == NULL, NULL);

    return node->val;
}

ssize_t u_bst_node_count (u_bst_node_t *node)
{
    dbg_return_if (node == NULL, -1);

    return node->nelem;
}

int u_bst_empty (u_bst_t *bst)
{
    dbg_return_if (bst == NULL, 1); 

    return (bst->root == NULL);
}

static int u_bst_node_new (u_bst_t *bst, const void *key, const void *val, 
        u_bst_node_t **pnode)
{
    u_bst_node_t *node = NULL;

    dbg_return_if (bst == NULL, ~0);
    dbg_return_if (pnode == NULL, ~0);
    dbg_return_if (key == NULL, ~0);

    warn_err_sif ((node = u_zalloc(sizeof *node)) == NULL);

    warn_err_if (u_bst_assign(&node->key, key, bst->keytype, bst->keysize));
    warn_err_if (u_bst_assign(&node->val, val, bst->valtype, bst->valsize));

    node->right = node->left = NULL;
    node->nelem = 1;    /* Count itself. */

    *pnode = node;

    return 0;
err:
    u_bst_node_do_free(bst, node);
    return ~0;
}

static void u_bst_node_free (u_bst_t *bst, u_bst_node_t *node)
{
    if (node == NULL)
        return;

    /* Recursively free all child nodes (post-order). */
    u_bst_node_free(bst, node->left);
    u_bst_node_free(bst, node->right);
    u_bst_node_do_free(bst, node);

    return;
}

static void u_bst_node_do_free (u_bst_t *bst, u_bst_node_t *node)
{
    warn_return_if (bst == NULL, );

    if (node == NULL)
        return;

    if (bst->keyfree && node->key) 
        bst->keyfree(node->key);

    if (bst->valfree && node->val) 
        bst->valfree(node->val);

    u_free(node);

    return;
}

static u_bst_node_t *u_bst_node_search (u_bst_node_t *node, const void *key, 
        int (*cmp)(const void *, const void *))
{
    int rc;

    dbg_return_if (cmp == NULL, NULL);
    dbg_return_if (key == NULL, NULL);

    /* We've reached an external node: the quest ends here. */
    if (node == NULL)
        return NULL;

    /* If keys match, we're done. */
    if ((rc = cmp(key, node->key)) == 0)
        return node;

    /* Searched key is greater: recur into the right subtree. */
    if (rc > 0)
        return u_bst_node_search(node->right, key, cmp);

    /* The searched key is smaller, go left. */
    return u_bst_node_search(node->left, key, cmp);
}

static int u_bst_assign (void **pdst, const void *src, u_bst_type_t t, 
        size_t sz)
{
    dbg_return_if (pdst == NULL, ~0);

    switch (t)
    {
        case U_BST_TYPE_STRING:
            warn_err_sif (src && (*pdst = u_strdup(src)) == NULL);
            break;
        case U_BST_TYPE_OPAQUE:
            if (src)
            {
                warn_err_ifm (sz == 0, "0-len opaque type !");
                warn_err_sif ((*pdst = u_malloc(sz)) == NULL);
                memcpy(*pdst, src, sz);
            }
            break;
        case U_BST_TYPE_PTR:
            memcpy(pdst, &src, sizeof(void **));
            break;
    }

    return 0;
err:
    return ~0;
}

/* Do in-order tree traversal.  Note that this provides a "natural" sort 
 * of BST elements. 
 * TODO: give a way to break the walk ? */
static void u_bst_node_foreach (u_bst_node_t *node, 
        void (*cb)(u_bst_node_t *, void *), void *cb_args)
{
    if (node == NULL)
        return;

    u_bst_node_foreach(node->left, cb, cb_args);
    cb(node, cb_args);
    u_bst_node_foreach(node->right, cb, cb_args);

    return;
}

static u_bst_node_t *u_bst_node_push_rand (u_bst_t *bst, u_bst_node_t *node, 
        const void *key, const void *val)
{
    dbg_return_if (bst == NULL, NULL);
    dbg_return_if (key == NULL, NULL);

    if (node == NULL)
    {
        warn_err_if (u_bst_node_new(bst, key, val, &node));
        return node;
    }

    /* The new node is inserted on the top with 1/nelem+1 probability. */
    if ((size_t) rand() < RAND_MAX / (node->nelem + 1))
        return u_bst_node_push_top(bst, node, key, val);

    if (bst->cmp(key, node->key) < 0)
        node->left = u_bst_node_push(bst, node->left, key, val);
    else
        node->right = u_bst_node_push(bst, node->right, key, val);

    node->nelem += 1;

    return node;
err:
    return NULL;
}

static u_bst_node_t *u_bst_node_push (u_bst_t *bst, u_bst_node_t *node, 
        const void *key, const void *val)
{
    dbg_return_if (bst == NULL, NULL);
    dbg_return_if (key == NULL, NULL);

    if (node == NULL)
    {
        warn_err_if (u_bst_node_new(bst, key, val, &node));
        return node;
    }

    if (bst->cmp(key, node->key) < 0)
        node->left = u_bst_node_push(bst, node->left, key, val);
    else
        node->right = u_bst_node_push(bst, node->right, key, val);

    node->nelem += 1;

    return node;
err:
    return NULL;
}
 
static u_bst_node_t *u_bst_node_push_bottom (u_bst_t *bst, const void *key, 
        const void *val)
{
    u_bst_node_t *parent, *node;

    dbg_return_if (bst == NULL, NULL);
    dbg_return_if (key == NULL, NULL);

    /* Empty BST, handle special case. */
    if (u_bst_empty(bst))
    {
        warn_err_if (u_bst_node_new(bst, key, val, &bst->root));
        return bst->root;
    }

    /* Find a way through the non-empty BST until we reach an external node. */
    for (node = bst->root;
            node != NULL;
            node = (bst->cmp(key, node->key) < 0) ? node->left : node->right)
    {
        /* Save parent reference.  It is needed later, on loop exhaustion, to 
         * fix left/child links of the node to which we attach. */
        parent = node;

        /* Increment children counter of each node while we traverse. */
        node->nelem += 1;
    }

    /* Create a new node with the supplied (key, val). */
    warn_err_if (u_bst_node_new(bst, key, val, &node));

    /* Choose the subtree to which it belongs. */
    if (bst->cmp(key, parent->key) < 0)
        parent->left = node;
    else
        parent->right = node;

    return bst->root;
err:
    return NULL;
}

static u_bst_node_t *u_bst_node_push_top (u_bst_t *bst, u_bst_node_t *node, 
        const void *key, const void *val)
{
    u_bst_node_t *newnode = NULL;

    dbg_return_if (bst == NULL, NULL);
    dbg_return_if (key == NULL, NULL);

    /* External node reached: create the node. */
    if (node == NULL)
    {
        warn_err_if (u_bst_node_new(bst, key, val, &newnode));
        return newnode;
    }

    /* Update child counter of the traversed node. */
    node->nelem += 1;

    /* Let the created node bubble up through subsequent rotations. */
    if (bst->cmp(key, node->key) < 0)
    {
        node->left = u_bst_node_push_top(bst, node->left, key, val);
        node = u_bst_rotate(node, U_BST_ROT_RIGHT);
    }
    else
    {
        node->right = u_bst_node_push_top(bst, node->right, key, val);
        node = u_bst_rotate(node, U_BST_ROT_LEFT);
    }
    
    return node;
err:
    return NULL;
}

static u_bst_node_t *u_bst_node_promote_nth (u_bst_node_t *node, size_t n)
{
    size_t t;
    
    dbg_return_if (node == NULL, NULL);

    t = (node->left == NULL) ? 0 : node->left->nelem;

    if (t > n)
    {
        node->left = u_bst_node_promote_nth(node->left, n);
        node = u_bst_rotate(node, U_BST_ROT_RIGHT);
    }

    if (t < n)
    {
        node->right = u_bst_node_promote_nth(node->right, n - (t + 1));
        node = u_bst_rotate(node, U_BST_ROT_LEFT);
    }

    return node;
}

static u_bst_node_t *u_bst_node_join_lr (u_bst_node_t *l, u_bst_node_t *r)
{
    if (r == NULL)
        return l;

    /* Make the smallest node in the right subtree the new BST root. */
    r = u_bst_node_promote_nth(r, 0);

    /* Let the left subtree become the left child of the new root. */
    r->left = l;

    return r;
}

static u_bst_node_t *u_bst_node_delete (u_bst_t *bst, u_bst_node_t *node, 
        const void *key, int *pfound)
{
    int rc;
    u_bst_node_t *delnode;

    if (node == NULL)
        return NULL;

    /* Search on the left subtree. */
    if ((rc = bst->cmp(key, node->key)) < 0)
        node->left = u_bst_node_delete(bst, node->left, key, pfound);

    /* Search on the right subtree. */
    if (rc > 0)
        node->right = u_bst_node_delete(bst, node->right, key, pfound);

    /* Found ! Evict it. */
    if (rc == 0)
    {
        delnode = node;
        node = u_bst_node_join_lr(node->left, node->right);
        u_bst_node_do_free(bst, delnode);
        *pfound = 1;
    }

    /* Update child nodes' counter. */
    if (node && *pfound)
        node->nelem -= 1;

    return node;
}

static u_bst_node_t *u_bst_node_find_nth (u_bst_node_t *node, size_t n)
{
    size_t t;

    if (node == NULL)
        return NULL;

    /* Store the number of elements in the left subtree. */
    t = (node->left == NULL) ? 0 : node->left->nelem;

    /* If 't' is smaller than the searched index, the n-th node hides
     * in the left subtree. */
    if (t > n)
        return u_bst_node_find_nth(node->left, n);   

    /* If 't' is larger than the searched index, the n-th node hides
     * in the right subtree at index n-(t+1). */
    if (t < n)
        return u_bst_node_find_nth(node->right, n - (t + 1));

    return node;    /* Found ! */
}

static u_bst_node_t *u_bst_node_balance (u_bst_node_t *node)
{
    if (node == NULL || node->nelem < 2)
        return node;

    /* Promote the median node to the BST root. */
    node = u_bst_node_promote_nth(node, node->nelem / 2);

    /* Then go recursively into its subtrees. */
    node->left = u_bst_node_balance(node->left);
    node->right = u_bst_node_balance(node->right);

    return node;
}

static u_bst_node_t *u_bst_node_join (u_bst_node_t *b1, u_bst_node_t *b2)
{
    if (b2 == NULL)
        return b1;

    if (b1 == NULL)
        return b2;

    /* TODO */
    return NULL;
}

static int u_bst_keycmp (const void *a, const void *b)
{
    return strcmp((const char *) a, (const char *) b);
}

static int u_bst_keycmp_dbg (const void *a, const void *b)
{
    int rc;
    const char *x = (const char *) a;
    const char *y = (const char *) b;

    rc = strcmp(x, y);
    u_con("%s %c %s", x, (rc == 0) ? '=' : (rc > 0) ? '>' : '<', y);

    return rc;
}

static void u_bst_genfree (void *p) 
{ 
    u_free(p); 
    return;
}
