/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l.
 */

#include <toolbox/bst.h>
#include <toolbox/carpal.h>
#include <toolbox/memory.h>
#include <toolbox/misc.h>

struct u_bst_node_s
{
    void *key, *val;                    /* node's key and value. */
    size_t nelem;                       /* number of elements in the subtrees
                                           rooted in this node. */
    struct u_bst_node_s *left, *right;  /* left and right subtrees. */
};

struct u_bst_s
{
    int opts;
    int (*cmp) (const void *, const void *);    /* key compare function. */
    u_bst_type_t keytype, valtype;              /* nature of keys and values. */
    size_t keysize, valsize;                    /* size of keys and values. */
    void (*keyfree) (void *);                   /* key dtor. */
    void (*valfree) (void *);                   /* value dtor. */
    struct u_bst_node_s *root;                  /* parent node reference. */
};

static int u_bst_keycmp (const void *a, const void *b);
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
static u_bst_node_t *u_bst_node_balance (u_bst_node_t *node);

#ifdef BST_DEBUG
static int u_bst_keycmp_dbg (const void *a, const void *b);
#endif  /* BST_DEBUG */

/**
    \defgroup   bst Binary Search Tree
    \{
        This module implements interfaces that let you work with a simple
        binary search tree.

    \section load Load

    A new BST instance is created via ::u_bst_new.  New nodes are added
    one after another by calling ::u_bst_push:

    \code
    size_t i;
    char key[KEY_SZ];
    u_bst_t *bst = NULL;

    // Create the root container
    dbg_err_if (u_bst_new(U_BST_OPT_NONE, &bst));

    // Add random keys referencing dummy NULL values
    for (i = 0; i < NELEMS; i++)
    {
        (void) u_snprintf(key, sizeof key, "%12.12d", rand());
        dbg_err_if (u_bst_push(bst, key, NULL));
    }
    \endcode

    By default keys are strings -- of which the BST holds a private copy -- 
    and values are pointers to any data type, under the user's complete 
    responsibility.
    If you need to handle other key or value types, or different ownership 
    logics, use the u_bst_set_* family of functions.


    \section search Search

    Typically, once the tree is loaded, specific key values are searched,
    via ::u_bst_search, to retrieve their associated values (e.g. symbol table 
    lookups), like in the following:

    \code
    ...
    dbg_err_if (u_bst_push(symtbl, "HAVE_LIBU_BST", "0"));
    ...
    if ((node = u_bst_search(symtbl, "HAVE_LIBU_BST")) == NULL ||
            strcmp((const char *) u_bst_node_val(node), "1"))
    {
        u_con("HAVE_LIBU_BST undefined.");
        return ~0;
    }
    \endcode


    \section term Termination

    When you are done, the resources allocated to the BST can be reclaimed back
    with ::u_bst_free:
    
    \code
    u_bst_free(bst);
    \endcode

 */

/**
 *  \brief  Create a new ::u_bst_t object
 *
 *  Create a new ::u_bst_t object with given \p opts options
 *
 *  \param  opts    bitwise inclusive OR of ::U_BST_OPTS_* values
 *  \param  pbst    handler of the newly created ::u_bst_t object as a result 
 *                  argument
 *
 *  \retval  0  on success
 *  \retval ~0  on error
 */
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

/**
 *  \brief  Destroy an ::u_bst_t object
 *
 *  Destroy the (previously allocated) ::u_bst_t object \p bst
 *
 *  \param  bst handler of the object that needs to be destroyed
 *
 *  \return nothing
 */
void u_bst_free (u_bst_t *bst)
{
    if (bst)
    {
        u_bst_node_free(bst, bst->root);
        u_free(bst);
    }

    return;
}

/**
 *  \brief  Insert a new node into the BST
 *
 *  Insert a new node with key \p key and value \p val into the BST headed
 *  by \p bst.  The new node will be (initially) pushed to the bottom of the 
 *  tree, unless ::U_BST_OPT_PUSH_TOP (in which case the node is injected 
 *  on the top) or ::U_BST_OPT_RANDOMIZED (the injection point is chosen at 
 *  random) have been supplied at \p bst creation.
 *
 *  \param  bst an ::u_bst_t object handler
 *  \param  key the key of the node that will be inserted
 *  \param  val value stick to the inserted node
 *
 *  \retval  0  on success
 *  \retval ~0  on error
 */
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

/**
 *  \brief  Evict a node from the BST given its key
 *
 *  Evict the first node matching the supplied key \p key from \p bst
 *
 *  \param  bst an ::u_bst_t object handler
 *  \param  key the key to match for eviction
 *
 *  \retval  0  on success
 *  \retval ~0  if \p key was not found
 */
int u_bst_delete (u_bst_t *bst, const void *key)
{
    int found = 0;

    dbg_return_if (bst == NULL, ~0);
    dbg_return_if (key == NULL, ~0);

    bst->root = u_bst_node_delete(bst, bst->root, key, &found);

    return found ? 0 : ~0;
}

/**
 *  \brief  Search for a node matching a given key
 *
 *  Search the ::u_bst_t object \p bst for the first node matching the
 *  supplied key \p key
 *
 *  \param  bst an ::u_bst_t object handler
 *  \param  key the key to match
 *
 *  \return the handler for the found node on success, \c NULL in case no 
 *          matching node was found
 */
u_bst_node_t *u_bst_search (u_bst_t *bst, const void *key)
{
    dbg_return_if (bst == NULL, NULL);
    dbg_return_if (key == NULL, NULL);

    return u_bst_node_search(bst->root, key, bst->cmp);
}

/**
 *  \brief  Find the n-th element in the BST
 *
 *  Find the n-th (according to the comparison function -- \sa ::u_bst_set_cmp)
 *  element in the BST.
 *
 *  \param  bst an ::u_bst_t object handler
 *  \param  n   the ordinal position of the element that must be retrieved
 *
 *  \return the handler for the found node on success, \c NULL in case \p n
 *          is out of current BST bounds
 */
u_bst_node_t *u_bst_find_nth (u_bst_t *bst, size_t n)
{
    dbg_return_if (bst == NULL, NULL);

    return u_bst_node_find_nth(bst->root, n);
}

/**
 *  \brief  Count elements stored in the BST
 *
 *  Count the number of nodes actually stored in the supplied \p bst
 *
 *  \param  bst an ::u_bst_t object handler
 *
 *  \return the number of elements in BST, or \c -1 in case an invalid handler
 *          was supplied
 */
ssize_t u_bst_count (u_bst_t *bst)
{
    dbg_return_if (bst == NULL, (ssize_t) -1);

    if (bst->root == NULL)
        return 0;

    return bst->root->nelem;
}

/**
 *  \brief  In-order walk the BST invoking a function on each traversed node
 *
 *  In-order walk the BST handled by \p bst, invoking the supplied function
 *  \p cb with optional arguments \p cb_args at each traversed node.
 *
 *  \param  bst     an ::u_bst_t object handler
 *  \param  cb      the callback function to invoke on each node
 *  \param  cb_args auxiliary arguments (aside from the BST node handler, which
 *                   is automatically feeded) to be given to \p cb
 *
 *  \retval  0  on success
 *  \retval ~0  if \p bst or \p cb parameters are invalid
 */
int u_bst_foreach (u_bst_t *bst, void (*cb)(u_bst_node_t *, void *), 
        void *cb_args)
{
    dbg_return_if (bst == NULL, ~0);
    dbg_return_if (cb == NULL, ~0);

    u_bst_node_foreach(bst->root, cb, cb_args);

    return 0;
}

/**
 *  \brief  Re-balance a BST internal structure
 *
 *  Try to rebalance the supplied \p bst internal structure by doing the 
 *  needed promote/rotate dance
 *
 *  \param  bst     an ::u_bst_t object handler
 *
 *  \retval  0  on success
 *  \retval -1  on error
 */
int u_bst_balance (u_bst_t *bst)
{
    dbg_return_if (bst == NULL, ~0);

    bst->root = u_bst_node_balance(bst->root);

    return 0;
}

/**
 *  \brief  Do left/right rotation of the BST around a given pivot node
 *
 *  Rotate -- left or right depending on \p dir -- the \p bst around the 
 *  \p pivot node. 
 *
 *  \param  pivot   the node in the BST around which the BST is rotated
 *  \param  dir     one of ::U_BST_ROT_LEFT or ::U_BST_ROT_RIGHT
 *
 *  \return the new parent node, i.e the left or right child of the \p pivot, 
 *          depending on \p dir.
 */
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

/**
 *  \brief  Set custom key attributes on the supplied BST
 *
 *  Set (global) custom key attributes on the given ::u_bst_t object.
 *
 *  \param  bst an ::u_bst_t object handler
 *  \param  kt  type of the key, one of ::U_BST_TYPE_STRING, ::U_BST_TYPE_PTR
 *              or ::U_BST_TYPE_OPAQUE
 *  \param  ks  size of the key (needed for ::U_BST_TYPE_OPAQUE types)
 *
 *  \retval  0  on success
 *  \retval ~0  on error (i.e. invalid parameter)
 */
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

/**
 *  \brief  Set custom value attributes on the supplied BST
 *
 *  Set (global) custom value attributes on the given ::u_bst_t object.
 *
 *  \param  bst an ::u_bst_t object handler
 *  \param  vt  type of the value, one of ::U_BST_TYPE_STRING, ::U_BST_TYPE_PTR
 *              or ::U_BST_TYPE_OPAQUE
 *  \param  vs  size of the value (needed for ::U_BST_TYPE_OPAQUE types)
 *
 *  \retval  0  on success
 *  \retval ~0  on error (i.e. invalid parameter)
 */
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

/**
 *  \brief  Set custom key comparison function
 *
 *  Set a custom key comparison function \p f for the ::u_bst_t object \p bst
 *
 *  \param  bst an ::u_bst_t object handler
 *  \param  f   custom compare function
 *
 *  \retval  0  on success
 *  \retval ~0  on error (i.e. invalid parameter)
 */
int u_bst_set_cmp (u_bst_t *bst, int (*f)(const void *, const void *))
{
    dbg_return_if (bst == NULL, ~0);
    dbg_return_if (f == NULL, ~0);

    bst->cmp = f;

    return 0;
}

/**
 *  \brief  Set custom key free function
 *
 *  Set a custom key free function \p f for the ::u_bst_t object \p bst
 *
 *  \param  bst an ::u_bst_t object handler
 *  \param  f   custom key free function
 *
 *  \retval  0  on success
 *  \retval ~0  on error (i.e. invalid parameter)
 */
int u_bst_set_keyfree (u_bst_t *bst, void (*f)(void *))
{
    dbg_return_if (bst == NULL, ~0);
    dbg_return_if (f == NULL, ~0);

    bst->keyfree = f;

    return 0;
}

/**
 *  \brief  Set custom value free function
 *
 *  Set a custom value free function \p f for the ::u_bst_t object \p bst
 *
 *  \param  bst an ::u_bst_t object handler
 *  \param  f   custom value free function
 *
 *  \retval  0  on success
 *  \retval ~0  on error (i.e. invalid parameter)
 */
int u_bst_set_valfree (u_bst_t *bst, void (*f)(void *))
{
    dbg_return_if (bst == NULL, ~0);
    dbg_return_if (f == NULL, ~0);

    bst->valfree = f;

    return 0;
}

/**
 *  \brief  Node's key getter
 *
 *  Return the key stored in the given \p node    
 *
 *  \param  node    the node whose key needs to be retrieved
 *
 *  \return the node's key or \c NULL in case \p node was \c NULL
 */
const void *u_bst_node_key (u_bst_node_t *node)
{
    dbg_return_if (node == NULL, NULL);

    return node->key;
}

/**
 *  \brief  Node's value getter
 *
 *  Return the value stored in the given \p node
 *
 *  \param  node    the node whose value needs to be retrieved
 *
 *  \return the node's value or \c NULL in case \p node was \c NULL
 */
const void *u_bst_node_val (u_bst_node_t *node)
{
    dbg_return_if (node == NULL, NULL);

    return node->val;
}

/**
 *  \brief  Return the number of elements in the subtree rooted at \p node
 *
 *  Return the number of elements in the subtree rooted at \p node
 *
 *  \param  node    node's handler
 *
 *  \return the number of elements "under" \p node or -1 on error
 */
ssize_t u_bst_node_count (u_bst_node_t *node)
{
    dbg_return_if (node == NULL, -1);

    return node->nelem;
}

/**
 *  \brief  Tell if the supplied BST is empty
 *
 *  Tell if the supplied ::u_bst_t object \p bst is empty
 *
 *  \param  bst an ::u_bst_t object handler
 *
 *  \return 1 if empty, 0 otherwise
 */
int u_bst_empty (u_bst_t *bst)
{
    dbg_return_if (bst == NULL, 1); 

    return (bst->root == NULL);
}

/**
 *  \}  
 */ 

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
    node->left  = u_bst_node_balance(node->left);
    node->right = u_bst_node_balance(node->right);

    return node;
}

static int u_bst_keycmp (const void *a, const void *b)
{
    return strcmp((const char *) a, (const char *) b);
}

static void u_bst_genfree (void *p) 
{ 
    u_free(p); 
    return;
}

#ifdef BST_DEBUG
static int u_bst_keycmp_dbg (const void *a, const void *b)
{
    int rc;
    const char *x = (const char *) a;
    const char *y = (const char *) b;

    rc = strcmp(x, y);
    u_con("%s %c %s", x, (rc == 0) ? '=' : (rc > 0) ? '>' : '<', y);

    return rc;
}
#endif  /* BST_DEBUG */
