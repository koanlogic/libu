/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.  
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include <toolbox/json.h>
#include <toolbox/carpal.h>
#include <toolbox/misc.h>
#include <toolbox/memory.h>
#include <toolbox/lexer.h>

/* Internal representation of any JSON value. */
struct u_json_s
{
    u_json_type_t type;

    char fqn[U_JSON_FQN_SZ];    /* Fully qualified name of this (sub)object. */
    char key[U_TOKEN_SZ];       /* Local name, if applicable (i.e. !anon) */
    char val[U_TOKEN_SZ];       /* If applicable, i.e. (!OBJECT && !ARRAY) */

    /* Parent container. */
    struct u_json_s *parent;            

    /* Nodes at the same level as this one (if any). */
    TAILQ_ENTRY(u_json_s) siblings;

    /* Children nodes' list when i.e. (ARRAY || OBJECT). */
    TAILQ_HEAD(u_json_chld_s, u_json_s) children;

    unsigned int depth;         /* Depth of this node in the decoded tree. */

    /* Cacheing machinery. */
    unsigned int icur, count;   /* Aux stuff used when indexing arrays. */
    u_hmap_t *map;              /* Alias reference to the global cache. */
};

/* Pointer to the name part of .fqn */
#define U_JSON_OBJ_NAME(jo) \
        ((jo->parent != NULL) ? jo->fqn + strlen(p->fqn) : jo->fqn)

#define U_JSON_OBJ_IS_CONTAINER(jo) \
        ((jo->type == U_JSON_TYPE_OBJECT) || (jo->type == U_JSON_TYPE_ARRAY))

#define U_JSON_OBJ_IS_BOOL(jo) \
        ((jo->type == U_JSON_TYPE_TRUE) || (jo->type == U_JSON_TYPE_FALSE))

/* Lexer methods */
static int u_json_match_value (u_lexer_t *jl, u_json_t *jo);
static int u_json_match_number_first (u_lexer_t *jl);
static int u_json_match_number (u_lexer_t *jl, u_json_t *jo);
static int u_json_match_int (u_lexer_t *jl);
static int u_json_match_frac_first (char c);
static int u_json_match_frac (u_lexer_t *jl);
static int u_json_match_exp_first (char c);
static int u_json_match_exp (u_lexer_t *jl);
static int u_json_match_true_first (u_lexer_t *jl);
static int u_json_match_false_first (u_lexer_t *jl);
static int u_json_match_true (u_lexer_t *jl, u_json_t *jo);
static int u_json_match_false (u_lexer_t *jl, u_json_t *jo);
static int u_json_match_null_first (u_lexer_t *jl);
static int u_json_match_null (u_lexer_t *jl, u_json_t *jo);
static int u_json_match_string (u_lexer_t *jl, u_json_t *jo);
static int u_json_match_string_first (u_lexer_t *jl);
static int u_json_match_escaped_unicode (u_lexer_t *jl);
static int u_json_match_object (u_lexer_t *jl, u_json_t *jo);
static int u_json_match_object_first (u_lexer_t *jl);
static int u_json_match_array (u_lexer_t *jl, u_json_t *jo);
static int u_json_match_array_first (u_lexer_t *jl);
static int u_json_match_pair (u_lexer_t *jl, u_json_t *jo);
static int u_json_match_pair_first (u_lexer_t *jl);

/* Lexer misc stuff. */
static int u_json_match_seq (u_lexer_t *jl, u_json_t *jo, int type, 
        char first, const char *rem, size_t rem_sz);
static const char *u_json_type_str (int type);

/* Objects misc stuff. */
static void u_json_do_print (u_json_t *jo, size_t l, void *opaque);
static void u_json_do_free (u_json_t *jo, size_t l, void *opaque);
static void u_json_do_index (u_json_t *jo, size_t l, void *map);

/* Encode/Decode/Validate. */
static int u_json_do_encode (u_json_t *jo, u_string_t *s);
static int u_json_do_parse (const char *json, u_json_t **pjo, 
        char status[U_LEXER_ERR_SZ]);

/* Needed by hmap_easy* because we are storing pointer data not owned by the
 * hmap. */
static void nopf (void *dummy) { u_unused_args(dummy); return; }

static int u_json_new_container (u_json_type_t type, const char *key, 
        u_json_t **pjo);
static int u_json_new_atom (u_json_type_t type, const char *key, 
        const char *val, char check, u_json_t **pjo);

static int u_json_set_depth (u_json_t *jo, unsigned int depth);

/**
    \defgroup json JSON
    \{
        The \ref json module implements a family of routines that allow 
        encoding, decoding and validation of JSON objects as defined in 
        <a href="http://www.ietf.org/rfc/rfc4627.txt">RFC 4627</a>.
        

    \section decode Decoding

    A NUL-terminated string in JSON syntax is decoded into its parse
    tree by means of the ::u_json_decode function as the showed in the
    following snippet:

    \code
    u_json_t *jo = NULL;
    const char *i2 = "[ [ 1, 0 ], [ 0, 1 ] ]";

    dbg_err_if (u_json_decode(i2, &jo));
    \endcode


    \section validate Validating

        Should you just need to check the supplied string for syntax compliance
        (i.e. without actually creating the syntax tree), the ::u_json_validate
        interface can be used:
    \code
    char status[U_LEXER_ERR_SZ];

    if (u_json_validate(i2, status))
        u_con("Syntax error: %s", status);
    \endcode

    You should not use the validating interface on untrusted input.
    In fact no maximum nesting depth is enforced on validation -- on the 
    contrary, the parsing interface has the compile-time define 
    ::U_JSON_MAX_DEPTH for such purpose -- so a malicious user could make your 
    application stack explode by simply supplying a big-enough string made by 
    all \c '[' chars.
    The intended use of the validating interface is for checking your 
    hand-crafted JSON strings before pushing them out, i.e. those you've
    created without going through all the ::u_json_new -> ::u_json_add -> 
    ::u_json_encode chain.


    \section cache Indexing

    Once the string has been parsed and (implicitly or explicitly) 
    validated, should your application request frequent and/or massive access 
    to its deeply nested attributes, then you may want to create an auxiliary 
    index to the parse tree via ::u_json_index.  This way its nodes can be 
    accessed via a unique (and unambiguous -- provided that no anonymous key is
    embedded into the JSON object) naming scheme, similar to the typical 
    struct/array access in C, i.e.:
        - "." is the root node;
        - ".k" is used to access value in the parent object stored under the 
          key "k";
        - "[i]" is used to access the i-th value in the parent array.

    So, for example, the string ".I2[0][0]" would retrieve the value "1" from 
    "{ "I2": [ [ 1, 0 ], [ 0, 1 ] ] }".

    \code
    long l;

    dbg_err_if (u_json_index(jo));

    u_json_cache_get_int(jo, ".[0][0]", &l);    // l = 1
    u_json_cache_get_int(jo, ".[0][1]", &l);    // l = 0
    u_json_cache_get_int(jo, ".[1][0]", &l);    // l = 0
    u_json_cache_get_int(jo, ".[1][1]", &l);    // l = 1
    \endcode

    Please note that when index'ed, the parse tree enters a "frozen" state
    in which nothing but values and types of non-container objects (i.e. 
    string, number, boolean and null's) can be changed.  So, if you want to 
    come back to full tree manipulation, you must remove the indexing structure 
    by means of ::u_json_deindex -- which invalidates any subsequent cached 
    access attempt.


    \section build Building and Encoding

    JSON objects can be created ex-nihil via the ::u_json_new_* family
    of functions, and then encoded in their serialized form via the 
    ::u_json_encode interface:
    \code
    char *s = NULL;
    u_json_t *root = NULL, *leaf = NULL;

    dbg_err_if (u_json_new_object(NULL, &root));
    dbg_err_if (u_json_new_int("integer", 999, &leaf));
    dbg_err_if (u_json_add(root, leaf));
    leaf = NULL;

    // encode it, should give: "{ "integer": 999 }"
    u_json_encode(root, &s);
    \endcode


    \section iter Iterators

    The last basic concept that the user needs to know to work effectively with
    the JSON module is iteration.  Iterators allow efficient and safe traversal
    of container types (i.e. arrays and objects), where a naive walk strategy 
    based on ::u_json_array_get_nth would instead lead to performance collapse 
    as access time is quadratic in the number of elements in the container.

    \code
    long i, e;
    u_json_it_t jit;
    u_json_t *jo = NULL, *cur;
    const char *s = "[ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 ]";

    dbg_err_if (u_json_decode(s, &jo));

    // init array iterator from first element and go forward.
    dbg_err_if (u_json_it(u_json_child_first(jo), &jit));

    for (i = 1; (cur = u_json_it_next(&jit)) != NULL; i++)
    {
        dbg_err_if (u_json_get_int(cur, &e));
        dbg_err_if (e != i);    // e = 1..10
    }
    \endcode

    
    \section rfcimpl RFC implementation

    The following implementation choices have been made (re Section 4. of 
    RFC 4627):

    <table>
      <tr>
        <th>MAY</th>
        <th>Answer</th>
        <th>Notes</th>
      </tr>
      <tr>
        <td>Accept non-JSON forms or extensions ?</td>
        <td><b>NO</b></td>
        <td>
            Trailing non-JSON text is allowed (though warned) at the end of
            string.  I.e. the input string in scanned until the outermost 
            container is matched.
        </td>
      </tr>
      <tr>
        <td>Set limits on the size of accepted texts ?</td>
        <td><b>NO</b></td>
        <td>
            Depending only upon the memory available to the parsing process.
        </td>
      </tr>
      <tr>
        <td>Set limits on the maximum depth of nesting ?</td>
        <td><b>YES</b></td>
        <td>
            Made available to the parsing interface through the compile-time
            constant ::U_JSON_MAX_DEPTH.  The validating interface ignores this
            limit, and as such should be used with care (i.e. never on untrusted
            input).
        </td>
      </tr>
      <tr>
        <td>Set limits on the range of numbers ?</td>
        <td><b>NO</b></td>
        <td>
            All numerical values are stored as strings.  Truncation/conversion
            issues can arise only when trying to extract their \c long or 
            \c double counterparts through the ::u_json_get_int or 
            ::u_json_get_real commodity interfaces.  In case they fail you can
            still access the original (C-string) value through ::u_json_get_val.
        </td>
      </tr>
      <tr>
        <td>Set limits on the length and character contents of strings ?</td>
        <td><b>YES</b></td>
        <td>
            String length for both keys and values are upper-bounded by
            the compile-time constant ::U_TOKEN_SZ.
        </td>
      </tr>
    </table>

 */

/**
 *  \brief  Create a new and empty JSON object container
 *
 *  Create a new and empty JSON object container and return its handler as
 *  the result argument \p *pjo
 *
 *  \param  pjo Pointer to the ::u_json_t that will be returned
 *
 *  \retval ~0  on failure
 *  \retval  0  on success
 */
int u_json_new (u_json_t **pjo)
{
    u_json_t *jo = NULL;

    dbg_return_if (pjo == NULL, ~0);

    warn_err_sif ((jo = u_zalloc(sizeof *jo)) == NULL);
    TAILQ_INIT(&jo->children);
    jo->type = U_JSON_TYPE_UNKNOWN;
    jo->key[0] = jo->val[0] = jo->fqn[0] = '\0';
    jo->parent = NULL;
    jo->map = NULL;
    jo->count = 0;
    jo->depth = 0;

    *pjo = jo;

    return 0;
err:
    if (jo)
        u_free(jo);
    return ~0;
}

/**
 *  \brief  Set the type of a JSON object
 *
 *  Set the type of the supplied JSON object \p jo to \p type.
 *
 *  \param  jo      Pointer to a ::u_json_t object
 *  \param  type    One of the available ::u_json_type_t types
 *
 *  \retval ~0  on failure
 *  \retval  0  on success
 */
int u_json_set_type (u_json_t *jo, u_json_type_t type)
{
    dbg_return_if (jo == NULL, ~0);
    dbg_return_ifm (jo->map, ~0, "Cannot set type of a cached object");

    switch (type)
    {
        case U_JSON_TYPE_STRING:
        case U_JSON_TYPE_NUMBER:
        case U_JSON_TYPE_ARRAY:
        case U_JSON_TYPE_OBJECT:
        case U_JSON_TYPE_TRUE:
        case U_JSON_TYPE_FALSE:
        case U_JSON_TYPE_NULL:
        case U_JSON_TYPE_UNKNOWN:
            jo->type = type;
            break;
        default:
           u_err("unhandled type %d", type);
           return ~0;
    }

    return 0;
}

/**
 *  \brief  ::u_json_set_val_ex wrapper with no \p val syntax check
 */
int u_json_set_val (u_json_t *jo, const char *val)
{
    return u_json_set_val_ex(jo, val, 0);
}

/*
 *  \brief  Set the value of a JSON object
 *
 *  Set the value of the JSON object \p jo to the string value pointed by
 *  \p val.  This operation is meaningful only in case the underlying object
 *  is a number or a string, in which case the syntax of the supplied value
 *  can be checked by passing non-0 value to the \p check parameter.
 *
 *  \param  jo      Pointer to a ::u_json_t object
 *  \param  val     Pointer to the (non-NULL) value string
 *  \param  check   Set to non-0 if you need to syntax check \p val
 *
 *  \retval ~0  on failure
 *  \retval  0  on success
 */
int u_json_set_val_ex (u_json_t *jo, const char *val, char check)
{
    u_lexer_t *vl = NULL;

    dbg_return_if (jo == NULL, ~0);
    dbg_return_if (val == NULL, ~0);
    /* Note that cached objects allow for value overwrite. */

    if (check)
    {
        /* XXX The following declaration assumes C99 or C89+support of variable 
         *     length automatic arrays -- as in GCC since at least version 
         *     2.95.3. */
        char qval[strlen(val) + 3]; 

        /* If we are supposed to set a string value, we need to quote it. */
        (void) u_snprintf(qval, sizeof qval, 
                (jo->type == U_JSON_TYPE_STRING) ? "\"%s\"" : "%s", val);

        dbg_err_if (u_lexer_new(qval, &vl));
    }

    /* If requested, pass 'val' through its validator. */
    switch (jo->type)
    {
        case U_JSON_TYPE_STRING:
            dbg_err_if (check && u_json_match_string(vl, NULL));
            break;
        case U_JSON_TYPE_NUMBER:
            dbg_err_if (check && u_json_match_number(vl, NULL));
            break;
        default:
            /* Non-critical error, just emit some debug info. */
            goto end;
    }

    dbg_return_if (u_strlcpy(jo->val, val, sizeof jo->val), ~0);

    /* Fall through. */       
end:
    u_lexer_free(vl);
    return 0;
err:
    u_lexer_free(vl);
    return ~0;
}
/**
 *  \brief  Set the key of a JSON object
 *
 *  Set the key of the JSON object \p jo to the string value pointed by
 *  \p key.
 *
 *  \param  jo  Pointer to a ::u_json_t object
 *  \param  key Pointer to the key string.  In case \p key \c NULL the value
 *              is reset to the empty string.
 *
 *  \retval ~0  on failure
 *  \retval  0  on success
 */
int u_json_set_key (u_json_t *jo, const char *key)
{
    dbg_return_if (jo == NULL, ~0);
    dbg_return_ifm (jo->map, ~0, "Cannot set key of a cached object");

    dbg_return_if (u_strlcpy(jo->key, key ? key : "", sizeof jo->key), ~0);

    return 0;
}

/** \brief  Wrapper function that creates an array container.
 *          (\p key may be \c NULL). */
int u_json_new_array (const char *key, u_json_t **pjo)
{
    return u_json_new_container(U_JSON_TYPE_ARRAY, key, pjo);
}

/** \brief  Wrapper function that creates an object container 
 *          (\p key may be \c NULL). */
int u_json_new_object (const char *key, u_json_t **pjo)
{
    return u_json_new_container(U_JSON_TYPE_OBJECT, key, pjo);
}

/**
 *  \brief  Add a child JSON object to its parent container
 *
 *  Add the child JSON object \p jo to its parent container \p head.
 *
 *  \param  head    Pointer to the parent container
 *  \param  jo      Pointer to the child JSON object that shall be attached
 *
 *  \retval ~0  on failure
 *  \retval  0  on success
 */
int u_json_add (u_json_t *head, u_json_t *jo)
{
    dbg_return_if (head == NULL, ~0);
    dbg_return_if (!U_JSON_OBJ_IS_CONTAINER(head), ~0);
    dbg_return_ifm (head->map, ~0, "Cannot add new child to a cached object");
    dbg_return_if (jo == NULL, ~0);

#ifdef U_JSON_OBJ_DEBUG
    u_con("chld (%p): %s {%s} added at depth %u", 
            jo, u_json_type_str(jo->type), jo->key, jo->depth);
    u_con("prnt (%p): %s {%s} at depth %u\n", 
            head, u_json_type_str(head->type), head->key, head->depth);
#endif  /* U_JSON_OBJ_DEBUG */

    TAILQ_INSERT_TAIL(&head->children, jo, siblings);
    jo->parent = head;

    /* Adjust children counter for array-type parents. */
    if (head->type == U_JSON_TYPE_ARRAY)
        head->count += 1;

    return 0;
}

/**
 *  \brief  Break down a JSON string into pieces
 *
 *  Parse and (implicitly) validate the supplied JSON string \p json. 
 *  In case of success, its internal representation is returned into the result 
 *  argument \p *pjo.
 *
 *  \param  json    A NUL-terminated string containing some serialized JSON
 *  \param  pjo     Result argument which will point to the internal 
 *                  representation of the parsed \p json string
 *
 *  \retval ~0  on failure
 *  \retval  0  on success
 */
int u_json_decode (const char *json, u_json_t **pjo)
{
    return u_json_do_parse(json, pjo, NULL);
}

/**
 *  \brief  Validate the supplied JSON string.
 *
 *  Validate the supplied JSON string \p json.  In case \p json contains 
 *  invalid syntax, the parser/lexer error message is returned into \p status.
 *
 *  \param  json    A NUL-terminated string containing some serialized JSON
 *  \param  status  In case of error, this result argument will contain an
 *                  explanation message from the parser/lexer.
 *
 *  \retval ~0  on failure
 *  \retval  0  on success
 */
int u_json_validate (const char *json, char status[U_LEXER_ERR_SZ])
{
    /* Just try to validate the input string (do not build the tree). */
    return u_json_do_parse(json, NULL, status);
}

/**
 *  \brief  Dispose any resource allocated to a JSON object
 *
 *  Dispose any resource allocated to the supplied JSON object \p jo
 *
 *  \param  jo  Pointer to the ::u_json_t object that must be free'd
 *
 *  \return nothing
 */
void u_json_free (u_json_t *jo)
{
    size_t dummy = 0;

    if (jo == NULL)
        return;

    /* If there is an associated hmap free it. */
    if (jo->map)
    {
        u_hmap_easy_free(jo->map);
        jo->map = NULL;
    }
 
    /* Walk the tree in post order and free each node while we traverse. */
    u_json_walk(jo, U_JSON_WALK_POSTORDER, dummy, u_json_do_free, NULL);

    return;
}

/**
 *  \brief  Encode a JSON object
 *
 *  Encode the supplied JSON object \p jo to the result string pointed by 
 *  \p *ps
 *
 *  \param  jo  Pointer to the ::u_json_t object that must be encoded
 *  \param  ps  serialized JSON text corresponding to \p jo
 *
 *  \retval ~0  on failure
 *  \retval  0  on success
 */
int u_json_encode (u_json_t *jo, char **ps)
{
    u_string_t *s = NULL;

    dbg_return_if (jo == NULL, ~0);
    dbg_return_if (ps == NULL, ~0);

    dbg_err_if (u_string_create(NULL, 0, &s));
    dbg_err_if (u_json_do_encode(jo, s));
    *ps = u_string_detach_cstr(s);

    return 0;
err:
    if (s)
        u_string_free(s);
    return ~0;
}

/**
 *  \brief  Pre/post-order tree walker
 *
 *  Traverse the supplied JSON object \p jo in pre/post-order, depending on
 *  \p strategy, invoking the callback function \p cb on each node.
 *
 *  \param  jo          Pointer to ::u_json_t object to traverse
 *  \param  strategy    one of ::U_JSON_WALK_PREORDER or ::U_JSON_WALK_POSTORDER
 *  \param  l           depth level in the JSON tree (the root is at depth 0)
 *  \param  cb          function to invoke on each traversed node
 *  \param  cb_args     optional opaque data which will be supplied to \p cb
 *
 *  \return nothing
 */
void u_json_walk (u_json_t *jo, int strategy, size_t l, 
        void (*cb)(u_json_t *, size_t, void *), void *cb_args)
{
    dbg_return_if (strategy != U_JSON_WALK_PREORDER && 
            strategy != U_JSON_WALK_POSTORDER, );

    if (jo == NULL)
        return;

    if (strategy == U_JSON_WALK_PREORDER && cb)
        cb(jo, l, cb_args);

    /* When recurring into the children branch, increment depth by one. */
    u_json_walk(TAILQ_FIRST(&jo->children), strategy, l + 1, cb, cb_args);

    /* Siblings are at the same depth as the current node. */
    u_json_walk(TAILQ_NEXT(jo, siblings), strategy, l, cb, cb_args);

    if (strategy == U_JSON_WALK_POSTORDER && cb)
        cb(jo, l, cb_args);

    return;
}

/**
 *  \brief  Print to stderr the internal representation of a JSON object
 *
 *  Print to stderr the supplied JSON object \p jo
 *
 *  \param  jo  Pointer to the ::u_json_t object that must be printed
 *
 *  \return nothing
 */
void u_json_print (u_json_t *jo)
{
    dbg_return_if (jo == NULL, );

    /* Tree root is at '0' depth. */
    u_json_walk(jo, U_JSON_WALK_PREORDER, 0, u_json_do_print, NULL);

    return;
}

/**
 *  \brief  Index JSON object contents.
 *
 *  Index all contents of the supplied ::u_json_t top-level object \p jo.
 *  After data has been indexed, no more key/type modifications are possible
 *  on this object; instead, values of leaf nodes can still be changed.  
 *  Also, either child node addition nor removal is possible after the object 
 *  has been cached.  If \p jo needs to be changed in aforementioned ways 
 *  (type, key, addition or removal), it must be explicitly ::u_json_unindex'ed.
 *
 *  \param  jo  Pointer to the ::u_json_t object that must be indexed
 *
 *  \return nothing
 */
int u_json_index (u_json_t *jo)
{
    size_t u;   /* Unused. */
    u_hmap_opts_t *opts = NULL;
    u_hmap_t *hmap = NULL;

    dbg_return_if (jo == NULL, ~0);
    nop_return_if (jo->map, 0);     /* If already cached, return ok. */
    dbg_return_if (jo->parent, ~0); /* Cache can be created on top-objs only. */

    /* Create the associative array. */
    dbg_err_if (u_hmap_opts_new(&opts));
    dbg_err_if (u_hmap_opts_set_val_type(opts, U_HMAP_OPTS_DATATYPE_POINTER));
    dbg_err_if (u_hmap_opts_set_val_freefunc(opts, nopf));
    dbg_err_if (u_hmap_easy_new(opts, &hmap));
    opts = NULL;

    /* Initialize array elems' indexing. */
    jo->icur = 0;

    /* Walk the tree in pre-order and cache each node while we traverse. */
    u_json_walk(jo, U_JSON_WALK_PREORDER, u, u_json_do_index, hmap);

    /* Attach the associative array to the top level object. */
    jo->map = hmap, hmap = NULL;

    return 0;
err:
    if (opts)
        u_hmap_opts_free(opts);
    if (hmap)
        u_hmap_easy_free(hmap);
    return ~0;
}

/**
 *  \brief  Remove cache from JSON object.
 *
 *  Remove the whole cacheing machinery from the previously ::u_json_index'd 
 *  ::u_json_t object \p jo.
 *
 *  \param  jo  Pointer to the ::u_json_t object that must be de-indexed
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_json_unindex (u_json_t *jo)
{
    dbg_return_if (jo == NULL, ~0);
    nop_return_if (jo->map == NULL, 0);

    u_hmap_easy_free(jo->map);
    jo->map = NULL;

    return 0;
}

/**
 *  \brief  Retrieve JSON node by its cache name.
 *
 *  Possibly retrieve a JSON node by its (fully qualified, or relative) cache 
 *  \p name.
 *
 *  \param  jo      Pointer to the ::u_json_t object that must be searched
 *  \param  name    name of the element that must be searched
 *
 *  \return the retrieved JSON (sub)object on success; \c NULL in case \p key 
 *          was not found
 */
u_json_t *u_json_cache_get (u_json_t *jo, const char *name)
{
    u_json_t *res, *p;
    char fqn[U_JSON_FQN_SZ];

    dbg_return_if (jo == NULL, NULL);
    dbg_return_if (jo->map == NULL, NULL);
    dbg_return_if (name == NULL, NULL);

    if ((p = jo->parent) == NULL)
    {
        /* If 'jo' is top level, 'name' must be fully qualified. */ 
        return (u_json_t *) u_hmap_easy_get(jo->map, name);
    }

    /* Else ('jo' != top): first try 'name' as it were fully qualified. */
    if ((res = (u_json_t *) u_hmap_easy_get(jo->map, name)))
        return res;

    /* If 'name' is relative, we need to prefix it with the parent name 
     * space. */
    dbg_if (u_snprintf(fqn, sizeof fqn, "%s%s", jo->fqn, name));

    return (u_json_t *) u_hmap_easy_get(jo->map, fqn);
}

/**
 *  \brief  Set JSON node's type and value by its cache name.
 *
 *  Set type and value of the supplied JSON (leaf) node by its (fully qualified
 *  or relative) cache \p name.  If \p type is ::U_JSON_TYPE_UNKNOWN the
 *  underlying type will be left untouched.
 *
 *  \param  jo      Pointer to an ::u_json_t object
 *  \param  name    name of the element that must be set
 *  \param  type    new type (or ::U_JSON_TYPE_UNKNOWN if no type change)
 *  \param  val     new value.  MUST be non-NULL in case we are setting a 
 *                  string a or number.
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_json_cache_set_tv (u_json_t *jo, const char *name, 
        u_json_type_t type, const char *val)
{
    u_json_t *res;

    dbg_return_if (U_JSON_OBJ_IS_CONTAINER(jo), ~0);
    dbg_return_if (type == U_JSON_TYPE_OBJECT || type == U_JSON_TYPE_ARRAY, ~0);
    /* 'jo' and 'name' will be checked by u_json_cache_get(); 
     * 'val' consistency is checked after type has been set. */

    /* Retrieve the node. */
    dbg_err_if ((res = u_json_cache_get(jo, name)) == NULL);

    /* First set type (in case !unknown) so that we know how to check the
     * subsequent value setting. */
    if (type != U_JSON_TYPE_UNKNOWN)
        res->type = type;

    /* Set value.  The caller must have supplied some non-NULL 'val' in case 
     * the final underlying type is a string or a number. */
    if (res->type == U_JSON_TYPE_STRING || res->type == U_JSON_TYPE_NUMBER)
        dbg_err_if (val == NULL || u_strlcpy(res->val, val, sizeof res->val));

    return 0;
err:
    return ~0;
}

/** \brief  Return the number of elements in array \p jo, or \c 0 on error. */
unsigned int u_json_array_count (u_json_t *jo)
{
    dbg_return_if (jo == NULL, 0);
    dbg_return_if (jo->type != U_JSON_TYPE_ARRAY, 0);

    return jo->count;
}

/** \brief  Get n-th element from \p jo array.  */
u_json_t *u_json_array_get_nth (u_json_t *jo, unsigned int n)
{
    u_json_t *elem;

    dbg_return_if (jo == NULL, NULL);
    dbg_return_if (jo->type != U_JSON_TYPE_ARRAY, NULL);
    dbg_return_if (n >= jo->count, NULL);

    /* Use cache if available. */
    if (jo->map)
    {
        char elem_fqn[U_JSON_FQN_SZ] = { '\0' };
        dbg_if (u_snprintf(elem_fqn, sizeof elem_fqn, "%s[%u]", jo->fqn, n));
        return u_json_cache_get(jo, elem_fqn);
    }
    
    /* Too bad if we don't have cache in place: we have to go through the 
     * list which is quadratic even with the following silly optimisation.
     * So it's ok for a couple of lookups, but if done systematically it's
     * an overkill.  Freeze instead ! */
    if (n > (jo->count / 2))
    {
        unsigned int r = jo->count - (n + 1);

        TAILQ_FOREACH_REVERSE (elem, &jo->children, u_json_chld_s, siblings)
        {
            if (r == 0)
                return elem;
            r -= 1;
        } 
    }
    else
    {
        TAILQ_FOREACH (elem, &jo->children, siblings)
        {
            if (n == 0)
                return elem;
            n -= 1;
        } 
    }

    /* Unreachable. */
    return NULL;
}

/** \brief  Get the value associated with the non-container object \p jo. */
const char *u_json_get_val (u_json_t *jo)
{
    dbg_return_if (jo == NULL, NULL);

    switch (jo->type)
    {
        case U_JSON_TYPE_STRING:
        case U_JSON_TYPE_NUMBER:
            return jo->val;
        case U_JSON_TYPE_TRUE:
            return "true";
        case U_JSON_TYPE_FALSE:
            return "false";
        case U_JSON_TYPE_NULL:
            return "null";
        case U_JSON_TYPE_OBJECT:
        case U_JSON_TYPE_ARRAY:
        case U_JSON_TYPE_UNKNOWN:
        default:
            return NULL;
    }
}

/** \brief  Get the \c long \c int value of the non-container object \p jo. */
int u_json_get_int (u_json_t *jo, long *pl)
{
    dbg_return_if (jo == NULL, ~0);
    dbg_return_if (jo->type != U_JSON_TYPE_NUMBER, ~0);
    dbg_return_if (pl == NULL, ~0);

    dbg_err_if (u_atol(jo->val, pl));

    return 0;
err:
    return ~0;
}

/** \brief  Get the \c double precision FP value of the non-container object 
 *          \p jo. */
int u_json_get_real (u_json_t *jo, double *pd)
{
    dbg_return_if (jo == NULL, ~0);
    dbg_return_if (jo->type != U_JSON_TYPE_NUMBER, ~0);
    dbg_return_if (pd == NULL, ~0);

    dbg_err_if (u_atof(jo->val, pd));

    return 0;
err:
    return ~0;
}

/** \brief  Get the boolean value of the non-container object \p jo. */
int u_json_get_bool (u_json_t *jo, char *pb)
{
    dbg_return_if (jo == NULL, ~0);
    dbg_return_if (pb == NULL, ~0);

    switch (jo->type)
    {
        case U_JSON_TYPE_TRUE:
            *pb = 1;
            break;
        case U_JSON_TYPE_FALSE:
            *pb = 0;
            break;
        default:
            return ~0;
    }

    return 0;
}

/** \brief  Wrapper around ::u_json_cache_get to retrieve string values from 
 *          terminal (i.e. non-container) objects. */
const char *u_json_cache_get_val (u_json_t *jo, const char *name)
{
    u_json_t *res = u_json_cache_get(jo, name);

    return u_json_get_val(res);
}

/** \brief  Wrapper around ::u_json_cache_get to retrieve integer values from 
 *          terminal (i.e. non-container) objects. */
int u_json_cache_get_int (u_json_t *jo, const char *name, long *pval)
{
    u_json_t *res;

    dbg_return_if ((res = u_json_cache_get(jo, name)) == NULL, ~0);

    return u_json_get_int(res, pval);
}

/** \brief  Wrapper around ::u_json_cache_get to retrieve double precision FP
 *          values from terminal (i.e. non-container) objects. */
int u_json_cache_get_real (u_json_t *jo, const char *name, double *pval)
{
    u_json_t *res;

    dbg_return_if ((res = u_json_cache_get(jo, name)) == NULL, ~0);

    return u_json_get_real(res, pval);
}

/** \brief  Wrapper around ::u_json_cache_get to retrieve boolean values from 
 *          terminal (i.e. non-container) objects. */
int u_json_cache_get_bool (u_json_t *jo, const char *name, char *pval)
{
    u_json_t *res;

    dbg_return_if ((res = u_json_cache_get(jo, name)) == NULL, ~0);

    return u_json_get_bool(res, pval);
}

/**
 *  \brief  Remove an object from its JSON container.
 *
 *  Remove an object from its JSON container.  This interface falls back to
 *  ::u_json_free in case the supplied \p jo is the root node.
 *
 *  \param  jo  Pointer to the ::u_json_t object that must be removed
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_json_remove (u_json_t *jo)
{
    u_json_t *p;

    dbg_return_if (jo == NULL, ~0);
    dbg_return_ifm (jo->map, ~0, "Cannot remove (from) a cached object");

    if ((p = jo->parent))
    {            
        /* Fix counters when parent is an array. */
        if (p->type == U_JSON_TYPE_ARRAY)
            p->count -= 1;

        /* Evict from the parent container. */
        TAILQ_REMOVE(&p->children, jo, siblings);
    }

    /* Give back the resources allocated to the node (and its children). */
    u_json_free(jo);

    return 0;
}

/** \brief  Create new JSON string object. */
int u_json_new_string (const char *key, const char *val, u_json_t **pjo)
{
    return u_json_new_atom(U_JSON_TYPE_STRING, key, val, 1, pjo);
}

/** \brief  Create new JSON number object. */
int u_json_new_number (const char *key, const char *val, u_json_t **pjo)
{
    return u_json_new_atom(U_JSON_TYPE_NUMBER, key, val, 1, pjo);
}

/** \brief  Create new JSON number object from double precision FP number. */
int u_json_new_real (const char *key, double val, u_json_t **pjo)
{
    char sval[U_TOKEN_SZ], check = 0;

#ifdef HAVE_ISFINITE
    /* Use isfinite() to avoid infinity's and NaN's which would break the
     * JSON syntax. */
    dbg_return_if (!isfinite(val), ~0);
#else
    /* If isfinite() is not available (i.e. !C99), force check in 
     * u_json_new_atom(). */
    check = 1;
#endif  /* HAVE_ISFINITE */

    /* "%g" does exponential (i.e. [+-]d.d...dE[+-]dd) or fixed-point (i.e.
     * [+-]d...d.d...d) notation depending on 'val'.  Both should be compatible
     * with JSON number spec. */ 
    dbg_return_if (u_snprintf(sval, sizeof sval, "%g", val), ~0);

    return u_json_new_atom(U_JSON_TYPE_NUMBER, key, sval, check, pjo);
}

/** \brief  Create new JSON number object from long integer. */
int u_json_new_int (const char *key, long val, u_json_t **pjo)
{
    char sval[U_TOKEN_SZ];

    dbg_return_if (u_snprintf(sval, sizeof sval, "%ld", val), ~0);

    /* Assume 'sval' correctly formatted (no need to validate). */
    return u_json_new_atom(U_JSON_TYPE_NUMBER, key, sval, 0, pjo);
}

/** \brief  Create new JSON null object. */
int u_json_new_null (const char *key, u_json_t **pjo)
{
    /* No need to validate. */
    return u_json_new_atom(U_JSON_TYPE_NULL, key, NULL, 0, pjo);
}

/** \brief  Create new JSON true or false object. */
int u_json_new_bool (const char *key, char val, u_json_t **pjo)
{
    /* No need to validate. */
    return u_json_new_atom(val ? U_JSON_TYPE_TRUE : U_JSON_TYPE_FALSE, 
            key, NULL, 0, pjo);
}

/** \brief  Return the first child (if any) from the supplied container 
 *          \p jo. */
u_json_t *u_json_child_first (u_json_t *jo)
{
    dbg_return_if (jo == NULL, NULL);
    dbg_return_if (!U_JSON_OBJ_IS_CONTAINER(jo), NULL);

    return TAILQ_FIRST(&jo->children);
}

/** \brief  Return the last child (if any) from the supplied container 
 *          \p jo. */
u_json_t *u_json_child_last (u_json_t *jo)
{
    dbg_return_if (jo == NULL, NULL);
    dbg_return_if (!U_JSON_OBJ_IS_CONTAINER(jo), NULL);

    return TAILQ_LAST(&jo->children, u_json_chld_s);
}

/** \brief  Initialize the iterator at \p it initially attached to \p jo. */
int u_json_it (u_json_t *jo, u_json_it_t *it)
{
    dbg_return_if (it == NULL, ~0);

    it->cur = jo;

    return 0;
}

/** \brief  Get JSON element under cursor and step cursor in the forward
 *          direction. */
u_json_t *u_json_it_next (u_json_it_t *it)
{
    u_json_t *jo;

    dbg_return_if (it == NULL, NULL);

    if ((jo = it->cur) != NULL)
        it->cur = TAILQ_NEXT(jo, siblings);

    return jo;
}

/** \brief  Get JSON element under cursor and step cursor in the backwards
 *          direction. */
u_json_t *u_json_it_prev (u_json_it_t *it)
{
    u_json_t *jo;

    dbg_return_if (it == NULL, NULL);

    if ((jo = it->cur) != NULL)
        it->cur = TAILQ_PREV(jo, u_json_chld_s, siblings);

    return jo;
}

/**
 *  \}
 */ 

static int u_json_do_encode (u_json_t *jo, u_string_t *s)
{
    if (jo == NULL)
        return 0;

    /* Optional key. */
    if (strlen(jo->key))
        dbg_err_if (u_string_aprintf(s, "\"%s\": ", jo->key));

    /* Value. */
    switch (jo->type)
    {
        case U_JSON_TYPE_STRING:
            dbg_err_if (u_string_aprintf(s, "\"%s\"", jo->val));
            break;
        case U_JSON_TYPE_NUMBER:
            dbg_err_if (u_string_aprintf(s, "%s", jo->val));
            break;
        case U_JSON_TYPE_OBJECT:
            dbg_err_if (u_string_aprintf(s, "{ "));
            break;
        case U_JSON_TYPE_ARRAY:
            dbg_err_if (u_string_aprintf(s, "[ "));
            break;
        case U_JSON_TYPE_TRUE:
            dbg_err_if (u_string_aprintf(s, "true"));
            break;
        case U_JSON_TYPE_FALSE:
            dbg_err_if (u_string_aprintf(s, "false"));
            break;
        case U_JSON_TYPE_NULL:
            dbg_err_if (u_string_aprintf(s, "null"));
            break;
        default:
            dbg_err("!");
    }

    /* Explore depth. */
    dbg_err_if (u_json_do_encode(TAILQ_FIRST(&jo->children), s));

    /* Close matching paren. */
    switch (jo->type)
    {
        case U_JSON_TYPE_ARRAY:
            dbg_err_if (u_string_aprintf(s, " ]"));
            break;
        case U_JSON_TYPE_OBJECT:
            dbg_err_if (u_string_aprintf(s, " }"));
            break;
        default:
            break;
    }

    /* When needed, add comma to separate siblings. */
    if (TAILQ_NEXT(jo, siblings))
        dbg_err_if (u_string_aprintf(s, ", "));

    /* Explore horizontally. */
    dbg_err_if (u_json_do_encode(TAILQ_NEXT(jo, siblings), s));

    return 0;
err:
    return ~0;
}

static int u_json_match_value (u_lexer_t *jl, u_json_t *jo)
{
    /* 'jo' can be NULL in case of a validating-only parser. */
    dbg_return_if (jl == NULL, ~0);

    if (u_json_match_string_first(jl))
        dbg_err_if (u_json_match_string(jl, jo));
    else if (u_json_match_number_first(jl))
        dbg_err_if (u_json_match_number(jl, jo));
    else if (u_json_match_object_first(jl))
        dbg_err_if (u_json_match_object(jl, jo));
    else if (u_json_match_array_first(jl))
        dbg_err_if (u_json_match_array(jl, jo));
    else if (u_json_match_true_first(jl))
        dbg_err_if (u_json_match_true(jl, jo));
    else if (u_json_match_false_first(jl))
        dbg_err_if (u_json_match_false(jl, jo));
    else if (u_json_match_null_first(jl))
        dbg_err_if (u_json_match_null(jl, jo));
    else
        U_LEXER_ERR(jl, "value not found at \'%s\'", u_lexer_lookahead(jl));

    return 0;
err:
    return ~0;
}

static int u_json_match_array_first (u_lexer_t *jl)
{
    return (u_lexer_peek(jl) == '[');
}

static int u_json_match_object_first (u_lexer_t *jl)
{
    return (u_lexer_peek(jl) == '{');
}

static int u_json_match_number_first (u_lexer_t *jl)
{
    char r, c = u_lexer_peek(jl);

    if ((r = (c == '-' || isdigit(c))))
        u_lexer_record_lmatch(jl);

    return r;
}

static int u_json_match_pair_first (u_lexer_t *jl)
{
    return u_json_match_string_first(jl);
}

static int u_json_match_false_first (u_lexer_t *jl)
{
    return (u_lexer_peek(jl) == 'f');
}

static int u_json_match_true_first (u_lexer_t *jl)
{
    return (u_lexer_peek(jl) == 't');
}

static int u_json_match_null_first (u_lexer_t *jl)
{
    return (u_lexer_peek(jl) == 'n');
}

static int u_json_match_string_first (u_lexer_t *jl)
{
    char r, c = u_lexer_peek(jl);

    if ((r = (c == '"')))
        u_lexer_record_lmatch(jl);

    return r;
}

/* number ::= INT[FRAC][EXP] */
static int u_json_match_number (u_lexer_t *jl, u_json_t *jo)
{
    char match[U_TOKEN_SZ];

    /* INT is mandatory */
    dbg_err_if (u_json_match_int(jl));

    /* c IN first(FRAC) */
    if (u_json_match_frac_first(u_lexer_peek(jl)))
        dbg_err_if (u_json_match_frac(jl));

    /* c IN first(EXP) */
    if (u_json_match_exp_first(u_lexer_peek(jl)))
        dbg_err_if (u_json_match_exp(jl));

    /* Register right side of the matched number. */
    u_lexer_record_rmatch(jl);

    /* Take care of the fact that the match includes the first non-number char
     * (see u_json_match_{int,exp,frac} for details). */
    (void) u_lexer_get_match(jl, match);
    match[strlen(match) - 1] = '\0';

    /* Push the matched number into the supplied json object. */
    if (jo)
    {
        dbg_err_if (u_json_set_type(jo, U_JSON_TYPE_NUMBER));
        dbg_err_if (u_json_set_val(jo, match));
    }

#ifdef U_JSON_LEX_DEBUG
    u_con("matched number: %s", u_lexer_get_match(jl, match));
#endif  /* U_JSON_LEX_DEBUG */

    return 0;
err:
    return ~0;
}

static int u_json_match_int (u_lexer_t *jl)
{
    char c = u_lexer_peek(jl);

    /* optional minus sign */
    if (c == '-')
        U_LEXER_NEXT(jl, &c);

    /* on '0' as the first char, we're done */
    if (c == '0')
    {
        U_LEXER_NEXT(jl, &c);
        goto end;
    }

    /* [1-9][0-9]+ */
    if (c >= 48 && c <= 57)
        do { U_LEXER_NEXT(jl, &c); } while (isdigit(c));
    else 
        U_LEXER_ERR(jl, "bad int syntax at %s", u_lexer_lookahead(jl));

    /* fall through */
end:
    return 0;
err:
    return ~0;
}

static int u_json_match_exp_first (char c)
{
    return (c == 'e' || c == 'E');
}

static int u_json_match_frac_first (char c)
{
    return (c == '.');
}

static int u_json_match_frac (u_lexer_t *jl)
{
    char c = u_lexer_peek(jl);

    /* Mandatory dot. */
    if (c != '.')
        U_LEXER_ERR(jl, "bad frac syntax at %s", u_lexer_lookahead(jl));

    U_LEXER_NEXT(jl, &c);

    /* [0-9] */
    if (!isdigit(c))
        U_LEXER_ERR(jl, "bad frac syntax at %s", u_lexer_lookahead(jl));

    /* [0-9]* */
    while (isdigit(c))
        U_LEXER_NEXT(jl, &c);

    return 0;
err:
    return ~0;
}

static int u_json_match_exp (u_lexer_t *jl)
{
    char c = u_lexer_peek(jl);

    /* [eE] */
    if (c != 'e' && c != 'E')
        U_LEXER_ERR(jl, "bad exp syntax at %s", u_lexer_lookahead(jl));

    U_LEXER_NEXT(jl, &c);

    /* Optional plus/minus sign. */
    if (c == '+' || c == '-')
        U_LEXER_NEXT(jl, &c);

    /* [0-9] */
    if (!isdigit(c))
        U_LEXER_ERR(jl, "bad exp syntax at %s", u_lexer_lookahead(jl));

    /* [0-9]* */
    while (isdigit(c))
        U_LEXER_NEXT(jl, &c);

    return 0;
err:
    return ~0;
}

static int u_json_match_seq (u_lexer_t *jl, u_json_t *jo, int type, 
        char first, const char *rem, size_t rem_sz)
{
    char c;
    size_t i = 0;

    if ((c = u_lexer_peek(jl)) != first)
    {
        U_LEXER_ERR(jl, "expect \'%c\', got %c at %s", 
                first, c, u_lexer_lookahead(jl));
    }

    for (i = 0; i < rem_sz; i++)
    {
        U_LEXER_SKIP(jl, &c);
        if (c != *(rem + i))
        {
            U_LEXER_ERR(jl, "expect \'%c\', got %c at %s", 
                    *(rem + i), c, u_lexer_lookahead(jl));
        }
    }

    /* Consume last checked char. */
    U_LEXER_SKIP(jl, NULL);

    if (jo)
        dbg_err_if (u_json_set_type(jo, type));

#ifdef U_JSON_LEX_DEBUG
    u_con("matched \'%s\' sequence", u_json_type_str(type));
#endif  /* U_JSON_LEX_DEBUG */
    return 0;
err:
    return ~0;
}

static int u_json_match_null (u_lexer_t *jl, u_json_t *jo)
{
    return u_json_match_seq(jl, jo, U_JSON_TYPE_NULL, 
            'n', "ull", strlen("ull"));
}

static int u_json_match_true (u_lexer_t *jl, u_json_t *jo)
{
    return u_json_match_seq(jl, jo, U_JSON_TYPE_TRUE, 
            't', "rue", strlen("rue"));
}

static int u_json_match_false (u_lexer_t *jl, u_json_t *jo)
{
    return u_json_match_seq(jl, jo, U_JSON_TYPE_FALSE, 
            'f', "alse", strlen("alse"));
}

static int u_json_match_array (u_lexer_t *jl, u_json_t *jo)
{
    char c;
    u_json_t *elem = NULL;

#ifdef U_JSON_LEX_DEBUG
    u_con("ARRAY");
#endif  /* U_JSON_LEX_DEBUG */

    if ((c = u_lexer_peek(jl)) != '[')
    {
        U_LEXER_ERR(jl, "expect \'[\', got %c at %s", 
                c, u_lexer_lookahead(jl));
    }

    /* Parent object is an array. */
    if (jo)
        dbg_err_if (u_json_set_type(jo, U_JSON_TYPE_ARRAY));

    do {
        /* As long as we want to accept empty arrays in this same scan loop, 
         * we could let trailing ',' pass unseen when a value has been already
         * consumed.  So, at each iteration, last non-whitespace char is saved 
         * to 'd' and checked when testing the empty array condition so that
         * we can emit a warn if needed.  
         * NOTE this is done equivalently in u_json_match_object(). */
        char d = u_lexer_peek(jl);

        U_LEXER_SKIP(jl, &c);
        
        if (c == ']')   /* break on empty array */
        {
            if (d == ',')
                u_warn("Trailing \',\' at the end of array !");
            break;
        }

        if (jo)
        {
            /* Create a new object to store next array element. */
            dbg_err_if (u_json_new(&elem));
            dbg_err_if (u_json_set_type(elem, U_JSON_TYPE_UNKNOWN));
            dbg_err_if (u_json_set_depth(elem, jo->depth + 1));
        }

        /* Fetch new value. */
        dbg_err_if (u_json_match_value(jl, elem));

        if (jo)
        {
            /* Push the fetched element to its parent array. */
            dbg_err_if (u_json_add(jo, elem)); 
            elem = NULL;
        }

        /* Consume any trailing white spaces. */
        if (isspace((int) u_lexer_peek(jl)))
            U_LEXER_SKIP(jl, NULL);

    } while (u_lexer_peek(jl) == ',');

    if ((c = u_lexer_peek(jl)) != ']')
    {
        U_LEXER_ERR(jl, "expect \']\', got %c at %s", 
                c, u_lexer_lookahead(jl));
    }

    /* Ignore EOT, shall be catched later. */
    (void) u_lexer_skip(jl, NULL);

    return 0;
err:
    u_json_free(elem);
    return ~0;
}

static int u_json_match_object (u_lexer_t *jl, u_json_t *jo)
{
    char c;

#ifdef U_JSON_LEX_DEBUG
    u_con("OBJECT");
#endif  /* U_JSON_LEX_DEBUG */

    if ((c = u_lexer_peek(jl)) != '{')
    {
        U_LEXER_ERR(jl, "expect \'{\', got %c at %s", 
                c, u_lexer_lookahead(jl));
    }

    if (jo)
        dbg_err_if (u_json_set_type(jo, U_JSON_TYPE_OBJECT));

    do {
        char d = u_lexer_peek(jl);

        U_LEXER_SKIP(jl, &c);

        /* Break on empty object. */
        if (c == '}')
        {
            if (d == ',')
                u_warn("Trailing \',\' at the end of object !");
            break;
        }

        /* Process assignement. */
        dbg_err_if (!u_json_match_pair_first(jl) || u_json_match_pair(jl, jo));

        /* Consume trailing white spaces, if any. */
        if (isspace((int) u_lexer_peek(jl)))
            U_LEXER_SKIP(jl, NULL);

    } while (u_lexer_peek(jl) == ',');

    if ((c = u_lexer_peek(jl)) != '}')
    {
        U_LEXER_ERR(jl, "expect \'}\', got %c at %s", 
                c, u_lexer_lookahead(jl));
    }

    /* Ignore EOT, shall be catched later. */
    (void) u_lexer_skip(jl, NULL);

    return 0;
err:
    return ~0;
}

static int u_json_match_pair (u_lexer_t *jl, u_json_t *jo)
{
    size_t mlen;
    char c, match[U_TOKEN_SZ];
    u_json_t *pair = NULL;

    dbg_return_if (jl == NULL, ~0);

#ifdef U_JSON_LEX_DEBUG
    u_con("PAIR");
#endif  /* U_JSON_LEX_DEBUG */

    /* Here we use the matched string as the 'key' for the associated value, 
     * hence there is no associated json object. */
    dbg_err_if (u_json_match_string(jl, NULL));

    /* Initialize new json object to store the key/value pair. */
    if (jo)
    {
        dbg_err_if (u_json_new(&pair));
        dbg_err_if (u_json_set_depth(pair, jo->depth + 1));
    }

    (void) u_lexer_get_match(jl, match);

    /* Trim trailing '"'. */
    if ((mlen = strlen(match)) >= 1)
        match[mlen - 1] = '\0';

    if (jo)
        dbg_err_if (u_json_set_key(pair, match));

    /* Consume trailing white spaces, if any. */
    if (isspace((int) u_lexer_peek(jl)))
        U_LEXER_SKIP(jl, NULL);

    /* Consume ':' */
    if ((c = u_lexer_peek(jl)) != ':')
    {
        U_LEXER_ERR(jl, "expect \':\', got %c at %s", 
                c, u_lexer_lookahead(jl));
    }

    U_LEXER_SKIP(jl, &c);

    /* Assign value. */
    dbg_err_if (u_json_match_value(jl, pair));

    /* Push the new value to the parent json object. */
    if (jo)
    {
        dbg_err_if (u_json_add(jo, pair));
        pair = NULL;
    }

    return 0;
err:
    u_json_free(pair);
    return ~0;
}

static int u_json_match_string (u_lexer_t *jl, u_json_t *jo)
{
    size_t mlen;
    char c, match[U_TOKEN_SZ];

    /* In case string is matched as an lval (i.e. the key side of a 'pair'),
     * there is no json object. */
    dbg_return_if (jl == NULL, ~0);

#ifdef U_JSON_LEX_DEBUG
    u_con("STRING");
#endif  /* U_JSON_LEX_DEBUG */

    if ((c = u_lexer_peek(jl)) != '"')
        U_LEXER_ERR(jl, "expect \", got %c at %s", c, u_lexer_lookahead(jl));

    U_LEXER_NEXT(jl, &c);

    /* Re-record the left side match pointer (trim leading '"'). */
    u_lexer_record_lmatch(jl);

    while (c != '"')
    {
        if (c == '\\')
        {
            U_LEXER_NEXT(jl, &c);
            switch (c)
            {
                case 'u':
                    dbg_err_if (u_json_match_escaped_unicode(jl));
                    break;
                case '"': case '\\': case '/': case 'b':
                case 'f': case 'n':  case 'r': case 't':
                    U_LEXER_NEXT(jl, &c);
                    break;
                default:
                    U_LEXER_ERR(jl, "invalid char %c in escape", c);
            }
        }
        else if (iscntrl((int) c))
            U_LEXER_ERR(jl, "control character in string", c); 
        else    
            U_LEXER_NEXT(jl, &c);
    }

    /* Record right match pointer.
     * This is a greedy match, which includes the trailing '"', and must
     * be taken into account when pulling out the string. */
    u_lexer_record_rmatch(jl);

    /* Consume last '"'. */
    U_LEXER_NEXT(jl, &c);

#ifdef U_JSON_LEX_DEBUG
    u_con("matched string: \'%s\'", u_lexer_get_match(jl, match));
#endif  /* U_JSON_LEX_DEBUG */

    /* In case the string is matched as an rval, the caller shall
     * supply the json object that has to be set. */
    if (jo)
    {
        dbg_err_if (u_json_set_type(jo, U_JSON_TYPE_STRING));

        /* Remove trailing '"' from match. */
        (void) u_lexer_get_match(jl, match);

        /* Trim trailing '"'. */
        if ((mlen = strlen(match)) >= 1)
            match[mlen - 1] = '\0';

        dbg_err_if (u_json_set_val(jo, match));
    }

    return 0;
err:
    return ~0;
}

static int u_json_match_escaped_unicode (u_lexer_t *jl)
{
    char i, c;

    for (i = 0; i < 4; i++)
    {
        U_LEXER_NEXT(jl, &c);

        if (!isxdigit((int) c))
            U_LEXER_ERR(jl, "non hex digit %c in escaped unicode", c); 
    }

    return 0;
err:
    return ~0;
}

static const char *u_json_type_str (int type)
{
    switch (type)
    {
        case U_JSON_TYPE_STRING:  return "string";
        case U_JSON_TYPE_NUMBER:  return "number";
        case U_JSON_TYPE_ARRAY:   return "array";
        case U_JSON_TYPE_OBJECT:  return "object";
        case U_JSON_TYPE_TRUE:    return "true";
        case U_JSON_TYPE_FALSE:   return "false";
        case U_JSON_TYPE_NULL:    return "null";
        case U_JSON_TYPE_UNKNOWN: default: break;
    }

    return "unknown";
}

static void u_json_do_free (u_json_t *jo, size_t l, void *opaque)
{
    u_unused_args(l, opaque);

    if (jo)
        u_free(jo);

    return;
}

static void u_json_do_print (u_json_t *jo, size_t l, void *opaque)
{
    u_unused_args(opaque);

    dbg_return_if (jo == NULL, );

    switch (jo->type)
    {
        case U_JSON_TYPE_ARRAY:
        case U_JSON_TYPE_OBJECT:
            /* No value. */
            u_con("%*c %s %s", l, ' ', u_json_type_str(jo->type), jo->key);
            break;
        default:
            u_con("%*c %s %s : \'%s\'", l, ' ', 
                    u_json_type_str(jo->type), jo->key, jo->val);
            break;
    }

    return;
}

static void u_json_do_index (u_json_t *jo, size_t l, void *map)
{
    u_json_t *p;
    u_hmap_t *hmap = (u_hmap_t *) map;

    u_unused_args(l);

    if ((p = jo->parent) == NULL)
    {
        /* Root node is named '.', its name and fully qualified name match. */
        (void) u_strlcpy(jo->fqn, ".", sizeof jo->fqn);
    }
    else if (p->type == U_JSON_TYPE_OBJECT)
    {
        /* Nodes in object containers are named after their key. */
        dbg_if (u_snprintf(jo->fqn, sizeof jo->fqn, "%s.%s", p->fqn, jo->key));
    }
    else if (p->type == U_JSON_TYPE_ARRAY)
    {
        /* Nodes in array containers are named after their ordinal position. */
        dbg_if (u_snprintf(jo->fqn, sizeof jo->fqn, "%s[%u]", p->fqn, p->icur));

        /* Increment the counting index in the parent array. */
        p->icur += 1;

        /* In case we have an array inside another array, initialize the 
         * counter for later recursive invocation. */
        if (jo->type == U_JSON_TYPE_ARRAY)
            jo->icur = 0;
    }
    else
        u_warn("Expecting an object, an array, or a top-level node.");

    /* Insert node into the hmap. */
    dbg_if (u_hmap_easy_put(hmap, jo->fqn, (const void *) jo));

    /* Stick the map pointer inside each visited node, so that it can
     * be readily referenced on retrieval. */
    jo->map = map;

#ifdef U_JSON_IDX_DEBUG
    u_con("%p => %s (%s) = %s", jo, jo->fqn, U_JSON_OBJ_NAME(jo), jo->val);
#endif

    return;
}

static int u_json_new_container (u_json_type_t type, const char *key, 
        u_json_t **pjo)
{
    u_json_t *jo = NULL;

    dbg_return_if (pjo == NULL, ~0);

    dbg_err_if (u_json_new(&jo));

    dbg_err_if (u_json_set_type(jo, type));
    dbg_err_if (u_json_set_key(jo, key ? key : ""));

    *pjo = jo;

    return 0;
err:
    if (jo)
        u_json_free(jo);
    return ~0;
}

static int u_json_new_atom (u_json_type_t type, const char *key, 
        const char *val, char check, u_json_t **pjo)
{
    u_json_t *jo = NULL;

    dbg_return_if (pjo == NULL, ~0);

    dbg_err_if (u_json_new(&jo));

    dbg_err_if (u_json_set_type(jo, type));
    dbg_err_if (u_json_set_key(jo, key));

    /* Values are meaningful only in case of string and number objects,
     * "null", "true" and "false" are completely defined by the type. */
    switch (type)
    {
        case U_JSON_TYPE_NUMBER:  
        case U_JSON_TYPE_STRING:  
            dbg_err_if (u_json_set_val_ex(jo, val, check));
        default: break;
    }

    *pjo = jo;

    return 0;
err:
    if (jo)
        u_json_free(jo);
    return ~0;
}

static int u_json_do_parse (const char *json, u_json_t **pjo, 
        char status[U_LEXER_ERR_SZ])
{
    u_json_t *jo = NULL;
    u_lexer_t *jl = NULL;

    /* When 'pjo' is NULL, assume this is a validating-only parser. */
    dbg_return_if (json == NULL, ~0);

    /* Create a disposable lexer context associated to the supplied
     * 'json' string. */
    dbg_err_if (u_lexer_new(json, &jl));

    /* Create top level json object. */
    dbg_err_if (pjo && u_json_new(&jo));

    /* Consume any trailing white space before starting actual parsing. */
    if (u_lexer_eat_ws(jl) == -1)
        U_LEXER_ERR(jl, "Empty JSON text !");

    /* Launch the lexer expecting the input JSON text as a serialized object 
     * or array. */ 
    if (u_json_match_object_first(jl))
        dbg_err_if (u_json_match_object(jl, jo));
    else if (u_json_match_array_first(jl))
        dbg_err_if (u_json_match_array(jl, jo));
    else
        U_LEXER_ERR(jl, "Expect \'{\' or \'[\', got \'%c\'.", u_lexer_peek(jl));

    /* Just warn in case the JSON string has not been completely consumed. */
    if (!u_lexer_eot(jl))
    {
        u_warn("Unparsed trailing text \'%s\' at position %u", 
                u_lexer_lookahead(jl), u_lexer_pos(jl));
    }

    /* Dispose the lexer context. */
    u_lexer_free(jl);

    /* Copy out the broken down tree. */
    if (pjo)
        *pjo = jo;

    return 0;
err:
    /* Copy out the lexer error string (if requested). */
    if (status)
        (void) u_strlcpy(status, u_lexer_geterr(jl), U_LEXER_ERR_SZ);

    u_lexer_free(jl);
    u_json_free(jo);

    return ~0;
}

static int u_json_set_depth (u_json_t *jo, unsigned int depth)
{
    /* Don't let'em smash our stack. */
    warn_err_ifm ((jo->depth = depth) >= U_JSON_MAX_DEPTH,
        "Maximum allowed nesting is %u.", U_JSON_MAX_DEPTH);

    return 0;
err:
    return ~0;
}
