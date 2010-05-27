/* 
 * Copyright (c) 2005-2010 by KoanLogic s.r.l. - All rights reserved.  
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <toolbox/json.h>
#include <toolbox/carpal.h>
#include <toolbox/misc.h>
#include <toolbox/memory.h>
#include <toolbox/lexer.h>

/* Internal representation of any JSON value. */
struct u_json_obj_s
{
    u_json_type_t type;

    char key[U_TOKEN_SZ];
    char val[U_TOKEN_SZ];  /* if applicable, i.e. (!object && !array) */

    TAILQ_ENTRY(u_json_obj_s) siblings;
    TAILQ_HEAD(, u_json_obj_s) children;
};

/* Lexer methods */
static int u_json_match_value (u_lexer_t *jl, u_json_obj_t *jo);
static int u_json_match_number_first (u_lexer_t *jl);
static int u_json_match_number (u_lexer_t *jl, u_json_obj_t *jo);
static int u_json_match_int (u_lexer_t *jl);
static int u_json_match_frac_first (char c);
static int u_json_match_frac (u_lexer_t *jl);
static int u_json_match_exp_first (char c);
static int u_json_match_exp (u_lexer_t *jl);
static int u_json_match_true_first (u_lexer_t *jl);
static int u_json_match_false_first (u_lexer_t *jl);
static int u_json_match_true (u_lexer_t *jl, u_json_obj_t *jo);
static int u_json_match_false (u_lexer_t *jl, u_json_obj_t *jo);
static int u_json_match_null_first (u_lexer_t *jl);
static int u_json_match_null (u_lexer_t *jl, u_json_obj_t *jo);
static int u_json_match_string (u_lexer_t *jl, u_json_obj_t *jo);
static int u_json_match_string_first (u_lexer_t *jl);
static int u_json_match_escaped_unicode (u_lexer_t *jl);
static int u_json_match_object (u_lexer_t *jl, u_json_obj_t *jo);
static int u_json_match_object_first (u_lexer_t *jl);
static int u_json_match_array (u_lexer_t *jl, u_json_obj_t *jo);
static int u_json_match_array_first (u_lexer_t *jl);
static int u_json_match_pair (u_lexer_t *jl, u_json_obj_t *jo);
static int u_json_match_pair_first (u_lexer_t *jl);

/* Lexer misc stuff. */
static int u_json_match_seq (u_lexer_t *jl, u_json_obj_t *jo, int type, 
        char first, const char *rem, size_t rem_sz);
static const char *u_json_type_str (int type);

/* Objects misc stuff. */
static void u_json_obj_do_print (u_json_obj_t *jo, size_t l);
static void u_json_obj_do_free (u_json_obj_t *jo, size_t l);

/* Encoder. */
static int u_json_do_encode (u_json_obj_t *jo, u_string_t *s);

/**
    \defgroup json JSON
    \{
        The \ref json module ...
 */

/**
 *  \brief  Create a new and empty JSON object container
 *
 *  Create a new and empty JSON object container and return its handler as
 *  the result argument \p *pjo
 *
 *  \param  pjo Pointer to the ::u_json_obj_t that will be returned
 *
 *  \retval ~0  on failure
 *  \retval  0  on success
 */
int u_json_obj_new (u_json_obj_t **pjo)
{
    u_json_obj_t *jo = NULL;

    dbg_return_if (pjo == NULL, ~0);

    warn_err_sif ((jo = u_zalloc(sizeof *jo)) == NULL);
    TAILQ_INIT(&jo->children);
    jo->type = U_JSON_TYPE_UNKNOWN;
    jo->key[0] = jo->val[0] = '\0';

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
 *  \param  jo      Pointer to a ::u_json_obj_t object
 *  \param  type    One of the available ::u_json_type_t types
 *
 *  \retval ~0  on failure
 *  \retval  0  on success
 */
int u_json_obj_set_type (u_json_obj_t *jo, u_json_type_t type)
{
    dbg_return_if (jo == NULL, ~0);

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
 *  \brief  Set the value of a JSON object
 *
 *  Set the value of the JSON object \p jo to the string value pointed by
 *  \p val.  This operation is meaningful only in case the underlying object
 *  is a number or a string.
 *
 *  \param  jo  Pointer to a ::u_json_obj_t object
 *  \param  val Pointer to the (non-NULL) value string
 *
 *  \retval ~0  on failure
 *  \retval  0  on success
 */
int u_json_obj_set_val (u_json_obj_t *jo, const char *val)
{
    dbg_return_if (jo == NULL, ~0);
    dbg_return_if (val == NULL, ~0);

    /* Non-critical error, just emit some debug info. */
    dbg_if (jo->type != U_JSON_TYPE_STRING && jo->type != U_JSON_TYPE_NUMBER);

    dbg_return_if (u_strlcpy(jo->val, val, sizeof jo->val), ~0);

    return 0;
}

/**
 *  \brief  Set the key of a JSON object
 *
 *  Set the key of the JSON object \p jo to the string value pointed by
 *  \p key.
 *
 *  \param  jo  Pointer to a ::u_json_obj_t object
 *  \param  key Pointer to the (non-NULL) key string
 *
 *  \retval ~0  on failure
 *  \retval  0  on success
 */
int u_json_obj_set_key (u_json_obj_t *jo, const char *key)
{
    dbg_return_if (jo == NULL, ~0);
    dbg_return_if (key == NULL, ~0);

    dbg_return_if (u_strlcpy(jo->key, key, sizeof jo->key), ~0);

    return 0;
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
int u_json_obj_add (u_json_obj_t *head, u_json_obj_t *jo)
{
    dbg_return_if (head == NULL, ~0);
    dbg_return_if (jo == NULL, ~0);

#ifdef U_JSON_OBJ_DEBUG
    u_con("chld (%p): %s {%s} added", jo, u_json_type_str(jo->type), jo->key);
    u_con("prnt (%p): %s {%s}\n", head, u_json_type_str(head->type), head->key);
#endif  /* U_JSON_OBJ_DEBUG */

    TAILQ_INSERT_TAIL(&head->children, jo, siblings);

    return 0;
}

/**
 *  \brief  Break down a JSON string into pieces
 *
 *  Parse and validate the supplied JSON string \p json and, in case of success,
 *  return its internal representation into the result argument \p *pjo.
 *
 *  \param  json    A NUL-terminated string containing some serialized JSON
 *  \param  pjo     Result argument which will point to the internal 
 *                  representation of the parsed \p json string
 *
 *  \retval ~0  on failure
 *  \retval  0  on success
 */
int u_json_parse (const char *json, u_json_obj_t **pjo)
{
    u_json_obj_t *jo = NULL;
    u_lexer_t *jl = NULL;

    dbg_return_if (json == NULL, ~0);
    dbg_return_if (pjo == NULL, ~0);

    /* Create a disposable lexer context associated to the supplied
     * 'json' string. */
    warn_err_if (u_lexer_new(json, &jl));

    /* Create top level json object. */
    warn_err_if (u_json_obj_new(&jo));

    /* Consume any trailing white space before starting actual parsing. */
    if (u_lexer_eat_ws(jl) == -1)
        U_LEXER_ERR(jl, "Empty JSON text !");

    /* Launch the lexer expecting the input JSON text as a serialized object 
     * or array. */ 
    if (u_json_match_object_first(jl))
        warn_err_if (u_json_match_object(jl, jo));
    else if (u_json_match_array_first(jl))
        warn_err_if (u_json_match_array(jl, jo));
    else
    {
        U_LEXER_ERR(jl, 
                "Expecting \'{\' or \'[\', found \'%c\'.", u_lexer_peek(jl));
    }

    /* Dispose the lexer context. */
    u_lexer_free(jl);

    /* Copy out the broken down tree. */
    *pjo = jo;

    return 0;
err:
    u_json_obj_free(jo);
    u_lexer_free(jl);
    return ~0;
}

/**
 *  \brief  Dispose any resource allocated to a JSON object
 *
 *  Dispose any resource allocated to the supplied JSON object \p jo
 *
 *  \param  jo  Pointer to the ::u_json_obj_t object that must be free'd
 *
 *  \return nothing
 */
void u_json_obj_free (u_json_obj_t *jo)
{
    size_t dummy = 0;

    if (jo == NULL)
        return;

    /* Walk the tree in post order and free each node while we traverse. */
    u_json_obj_walk(jo, U_JSON_WALK_POSTORDER, dummy, u_json_obj_do_free);

    return;
}

/**
 *  \brief  Encode a JSON object
 *
 *  Encode the supplied JSON object \p jo to the result string pointed by 
 *  \p *ps
 *
 *  \param  jo  Pointer to the ::u_json_obj_t object that must be encoded
 *  \param  ps  serialized JSON text corresponding to \p jo
 *
 *  \retval ~0  on failure
 *  \retval  0  on success
 */
int u_json_encode (u_json_obj_t *jo, char **ps)
{
    u_string_t *s = NULL;

    dbg_return_if (jo == NULL, ~0);
    dbg_return_if (ps == NULL, ~0);

    dbg_err_if (u_string_create(NULL, 0, &s));
    dbg_err_if (u_json_do_encode(jo, s));
    *ps = u_string_c(s);

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
 *  \param  jo          Pointer to ::u_json_obj_t object to traverse
 *  \param  strategy    one of ::U_JSON_WALK_PREORDER or ::U_JSON_WALK_POSTORDER
 *  \param  l           depth level in the JSON tree (the root is at depth 0)
 *  \param  cb          function to invoke on each traversed node
 *
 *  \return nothing
 */ 
void u_json_obj_walk (u_json_obj_t *jo, int strategy, size_t l, 
        void (*cb)(u_json_obj_t *, size_t))
{
    dbg_return_if (strategy != U_JSON_WALK_PREORDER && 
            strategy != U_JSON_WALK_POSTORDER, );

    if (jo == NULL)
        return;

    if (strategy == U_JSON_WALK_PREORDER && cb)
        cb(jo, l);

    /* When recurring into the children branch, increment depth by one. */
    u_json_obj_walk(TAILQ_FIRST(&jo->children), strategy, l + 1, cb);

    /* Siblings are at the same depth as the current node. */
    u_json_obj_walk(TAILQ_NEXT(jo, siblings), strategy, l, cb);

    if (strategy == U_JSON_WALK_POSTORDER && cb)
        cb(jo, l);

    return;
}

/**
 *  \brief  Print to stderr the internal representation of a JSON object
 *
 *  Print to stderr the supplied JSON object \p jo
 *
 *  \param  jo  Pointer to the ::u_json_obj_t object that must be printed
 *
 *  \return nothing
 */
void u_json_obj_print (u_json_obj_t *jo)
{
    dbg_return_if (jo == NULL, );

    /* Tree root is at '0' depth. */
    u_json_obj_walk(jo, U_JSON_WALK_PREORDER, 0, u_json_obj_do_print);

    return;
}

/**
 *  \}
 */ 

static int u_json_do_encode (u_json_obj_t *jo, u_string_t *s)
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

static int u_json_match_value (u_lexer_t *jl, u_json_obj_t *jo)
{
    dbg_return_if (jl == NULL, ~0);
    dbg_return_if (jo == NULL, ~0);

    if (u_json_match_string_first(jl))
        warn_err_if (u_json_match_string(jl, jo));
    else if (u_json_match_number_first(jl))
        warn_err_if (u_json_match_number(jl, jo));
    else if (u_json_match_object_first(jl))
        warn_err_if (u_json_match_object(jl, jo));
    else if (u_json_match_array_first(jl))
        warn_err_if (u_json_match_array(jl, jo));
    else if (u_json_match_true_first(jl))
        warn_err_if (u_json_match_true(jl, jo));
    else if (u_json_match_false_first(jl))
        warn_err_if (u_json_match_false(jl, jo));
    else if (u_json_match_null_first(jl))
        warn_err_if (u_json_match_null(jl, jo));
    else
    {
        U_LEXER_ERR(jl, "unexpected value syntax at \'%s\'", 
                u_lexer_lookahead(jl));
    }

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
static int u_json_match_number (u_lexer_t *jl, u_json_obj_t *jo)
{
    char match[U_TOKEN_SZ];

    /* INT is mandatory */
    warn_err_if (u_json_match_int(jl));

    /* c IN first(FRAC) */
    if (u_json_match_frac_first(u_lexer_peek(jl)))
        warn_err_if (u_json_match_frac(jl));

    /* c IN first(EXP) */
    if (u_json_match_exp_first(u_lexer_peek(jl)))
        warn_err_if (u_json_match_exp(jl));

    /* Register right side of the matched number. */
    u_lexer_record_rmatch(jl);

    /* Take care of the fact that the match includes the first non-number char
     * (see u_json_match_{int,exp,frac} for details). */
    (void) u_lexer_get_match(jl, match);
    match[strlen(match) - 1] = '\0';

    /* Push the matched number into the supplied json object. */
    warn_err_if (u_json_obj_set_type(jo, U_JSON_TYPE_NUMBER));
    warn_err_if (u_json_obj_set_val(jo, match));

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

static int u_json_match_seq (u_lexer_t *jl, u_json_obj_t *jo, int type, 
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

    warn_err_if (u_json_obj_set_type(jo, type));

#ifdef U_JSON_LEX_DEBUG
    u_con("matched \'%s\' sequence", u_json_type_str(type));
#endif  /* U_JSON_LEX_DEBUG */
    return 0;
err:
    return ~0;
}

static int u_json_match_null (u_lexer_t *jl, u_json_obj_t *jo)
{
    return u_json_match_seq(jl, jo, U_JSON_TYPE_NULL, 
            'n', "ull", strlen("ull"));
}

static int u_json_match_true (u_lexer_t *jl, u_json_obj_t *jo)
{
    return u_json_match_seq(jl, jo, U_JSON_TYPE_TRUE, 
            't', "rue", strlen("rue"));
}

static int u_json_match_false (u_lexer_t *jl, u_json_obj_t *jo)
{
    return u_json_match_seq(jl, jo, U_JSON_TYPE_FALSE, 
            'f', "alse", strlen("alse"));
}

static int u_json_match_array (u_lexer_t *jl, u_json_obj_t *jo)
{
    char c;
    u_json_obj_t *elem = NULL;

#ifdef U_JSON_LEX_DEBUG
    u_con("ARRAY");
#endif  /* U_JSON_LEX_DEBUG */

    if ((c = u_lexer_peek(jl)) != '[')
    {
        U_LEXER_ERR(jl, "expect \'[\', got %c at %s", 
                c, u_lexer_lookahead(jl));
    }

    /* Parent object is an array. */
    warn_err_if (u_json_obj_set_type(jo, U_JSON_TYPE_ARRAY));

    do {
        U_LEXER_SKIP(jl, &c);

        if (c == ']')   /* break on empty array */
            break;

        /* Create a new object to store next array element. */
        warn_err_if (u_json_obj_new(&elem));
        warn_err_if (u_json_obj_set_type(elem, U_JSON_TYPE_UNKNOWN));

        /* Fetch new value. */
        warn_err_if (u_json_match_value(jl, elem));

        /* Push the fetched element to its parent array. */
        warn_err_if (u_json_obj_add(jo, elem)); 
        elem = NULL;

        /* Consume any trailing white spaces. */
        if (isspace(u_lexer_peek(jl)))
            U_LEXER_SKIP(jl, NULL);

    } while (u_lexer_peek(jl) == ',');

    if ((c = u_lexer_peek(jl)) != ']')
    {
        U_LEXER_ERR(jl, "expect \']\', got %c at %s", 
                c, u_lexer_lookahead(jl));
    }

    U_LEXER_SKIP(jl, &c);

    return 0;
err:
    u_json_obj_free(elem);
    return ~0;
}

static int u_json_match_object (u_lexer_t *jl, u_json_obj_t *jo)
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

    warn_err_if (u_json_obj_set_type(jo, U_JSON_TYPE_OBJECT));

    do {
        U_LEXER_SKIP(jl, &c);

        /* Break on empty object. */
        if (c == '}')
            break;

        /* Process assignement. */
        warn_err_if (!u_json_match_pair_first(jl) || u_json_match_pair(jl, jo));

        /* Consume trailing white spaces, if any. */
        if (isspace(u_lexer_peek(jl)))
            U_LEXER_SKIP(jl, NULL);

    } while (u_lexer_peek(jl) == ',');

    if ((c = u_lexer_peek(jl)) != '}')
    {
        U_LEXER_ERR(jl, "expect \'}\', got %c at %s", 
                c, u_lexer_lookahead(jl));
    }

    U_LEXER_SKIP(jl, &c);

    return 0;
err:
    return ~0;
}

static int u_json_match_pair (u_lexer_t *jl, u_json_obj_t *jo)
{
    size_t mlen;
    char c, match[U_TOKEN_SZ];
    u_json_obj_t *pair = NULL;

    dbg_return_if (jl == NULL, ~0);
    dbg_return_if (jo == NULL, ~0);

#ifdef U_JSON_LEX_DEBUG
    u_con("PAIR");
#endif  /* U_JSON_LEX_DEBUG */

    /* Here we use the matched string as the 'key' for the associated value, 
     * hence there is no associated json object. */
    warn_err_if (u_json_match_string(jl, NULL));

    /* Initialize new json object to store the key/value pair. */
    warn_err_if (u_json_obj_new(&pair));

    (void) u_lexer_get_match(jl, match);

    /* Trim trailing '"'. */
    if ((mlen = strlen(match)) >= 1)
        match[mlen - 1] = '\0';

    warn_err_if (u_json_obj_set_key(pair, match));

    /* Consume ':' */
    if ((c = u_lexer_peek(jl)) != ':')
    {
        U_LEXER_ERR(jl, "expect \':\', got %c at %s", 
                c, u_lexer_lookahead(jl));
    }

    U_LEXER_SKIP(jl, &c);

    /* Assign value. */
    warn_err_if (u_json_match_value(jl, pair));

    /* Push the new value to the parent json object. */
    warn_err_if (u_json_obj_add(jo, pair));
    pair = NULL;

    return 0;
err:
    u_json_obj_free(pair);
    return ~0;
}

static int u_json_match_string (u_lexer_t *jl, u_json_obj_t *jo)
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
                    warn_err_if (u_json_match_escaped_unicode(jl));
                    break;
                case '"': case '\\': case '/': case 'b':
                case 'f': case 'n':  case 'r': case 't':
                    U_LEXER_NEXT(jl, &c);
                    break;
                default:
                    U_LEXER_ERR(jl, "invalid char %c in escape", c);
            }
        }
        else if (iscntrl(c))
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
        warn_err_if (u_json_obj_set_type(jo, U_JSON_TYPE_STRING));

        /* Remove trailing '"' from match. */
        (void) u_lexer_get_match(jl, match);

        /* Trim trailing '"'. */
        if ((mlen = strlen(match)) >= 1)
            match[mlen - 1] = '\0';

        warn_err_if (u_json_obj_set_val(jo, match));
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

        if (!isxdigit(c))
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

static void u_json_obj_do_free (u_json_obj_t *jo, size_t l)
{
    u_unused_args(l);

    if (jo)
        u_free(jo);

    return;
}

static void u_json_obj_do_print (u_json_obj_t *jo, size_t l)
{
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
