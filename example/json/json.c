#include <ctype.h>
#include <u/libu.h>
#include "json.h"

/* (Still) unparsed string. */
#define STR_REM(jl) (&jl->s[jl->pos])

/*
 * All the following JSON_LEX_ macros need the "err" label in scope.
 */

#define JSON_LEX_ERR(jl, ...)                                   \
    do {                                                        \
        (void) json_lex_seterr(jl, __VA_ARGS__);                \
        goto err;                                               \
    } while (0)

/* json_lex_skip() wrapper. */
#define JSON_LEX_SKIP(jl, pc)                                   \
    do {                                                        \
        if (json_lex_skip(jl, pc))                              \
            JSON_LEX_ERR(jl, "EOT at offset %zu", jl->pos);     \
    } while (0)

/* json_lex_next() wrapper. */
#define JSON_LEX_NEXT(jl, pc)                                   \
    do {                                                        \
        if (json_lex_next(jl, pc))                              \
            JSON_LEX_ERR(jl, "EOT at offset %zu", jl->pos);     \
    } while (0)

/* Internal representation of any JSON value. */
struct json_obj_s
{
    enum { 
        JSON_TYPE_UNKNOWN = 0,
        JSON_TYPE_STRING, 
        JSON_TYPE_NUMBER, 
        JSON_TYPE_OBJECT, 
        JSON_TYPE_ARRAY, 
        JSON_TYPE_TRUE, 
        JSON_TYPE_FALSE, 
        JSON_TYPE_NULL
    } type; 

    char key[JSON_KEY_SZ];
    char val[JSON_VAL_SZ];  /* if applicable, i.e. (!object && !array) */

    TAILQ_ENTRY(json_obj_s) siblings;
    TAILQ_HEAD(, json_obj_s) children;
};

/* JSON lexer context. */
struct json_lex_s
{
    char *s;        /* string to be parsed */
    size_t slen;    /* string length */
    size_t pos;     /* actual lexer position */
    size_t lmatch;  /* offset of actual left side match */
    size_t rmatch;  /* offset of actual right side match */
    char err[512];
};

/* Lexer methods */
static int json_match_value (json_lex_t *jl, json_obj_t *jo);
static int json_match_number_first (json_lex_t *jl);
static int json_match_number (json_lex_t *jl, json_obj_t *jo);
static int json_match_int (json_lex_t *jl);
static int json_match_frac_first (char c);
static int json_match_frac (json_lex_t *jl);
static int json_match_exp_first (char c);
static int json_match_exp (json_lex_t *jl);
static int json_match_true_first (json_lex_t *jl);
static int json_match_false_first (json_lex_t *jl);
static int json_match_true (json_lex_t *jl, json_obj_t *jo);
static int json_match_false (json_lex_t *jl, json_obj_t *jo);
static int json_match_null_first (json_lex_t *jl);
static int json_match_null (json_lex_t *jl, json_obj_t *jo);
static int json_match_string (json_lex_t *jl, json_obj_t *jo);
static int json_match_string_first (json_lex_t *jl);
static int json_match_escaped_unicode (json_lex_t *jl);
static int json_match_object (json_lex_t *jl, json_obj_t *jo);
static int json_match_object_first (json_lex_t *jl);
static int json_match_array (json_lex_t *jl, json_obj_t *jo);
static int json_match_array_first (json_lex_t *jl);
static int json_match_pair (json_lex_t *jl, json_obj_t *jo);
static int json_match_pair_first (json_lex_t *jl);

/* Lexer misc stuff. */
static int json_lex_next (json_lex_t *jl, char *pb);
static int json_lex_skip (json_lex_t *jl, char *pb);
static char json_lex_peek (json_lex_t *jl);
static int json_lex_seterr (json_lex_t *jl, const char *fmt, ...);
static void json_record_lmatch (json_lex_t *jl);
static void json_record_rmatch (json_lex_t *jl);
static size_t json_strlen_match (json_lex_t *jl);
static char *json_get_match (json_lex_t *jl, char match[JSON_VAL_SZ]);
static int json_match_seq (json_lex_t *jl, json_obj_t *jo, int type, char first,
        const char *rem, size_t rem_sz);
static const char *json_type_str (int type);
static int json_lex_eot (json_lex_t *jl);
static void json_lex_incr (json_lex_t *jl);
static int json_lex_next_ex (json_lex_t *jl, int eat_ws, char *pb);
static int json_lex_eat_ws (json_lex_t *jl);

/* Objects misc stuff. */
static void json_obj_do_print (json_obj_t *jo, size_t l);
static void json_obj_do_free (json_obj_t *jo, size_t l);

/* Encoder. */
static int json_do_encode (json_obj_t *jo, u_string_t *s);

int json_obj_new (json_obj_t **pjo)
{
    json_obj_t *jo = NULL;

    dbg_return_if (pjo == NULL, ~0);

    warn_err_sif ((jo = u_zalloc(sizeof *jo)) == NULL);
    TAILQ_INIT(&jo->children);
    jo->type = JSON_TYPE_UNKNOWN;
    jo->key[0] = jo->val[0] = '\0';

    *pjo = jo;

    return 0;
err:
    if (jo)
        u_free(jo);
    return ~0;
}

int json_obj_set_type (json_obj_t *jo, int type)
{
    dbg_return_if (jo == NULL, ~0);

    switch (type)
    {
        case JSON_TYPE_STRING:
        case JSON_TYPE_NUMBER:
        case JSON_TYPE_ARRAY:
        case JSON_TYPE_OBJECT:
        case JSON_TYPE_TRUE:
        case JSON_TYPE_FALSE:
        case JSON_TYPE_NULL:
        case JSON_TYPE_UNKNOWN:
            jo->type = type;
            break;
        default:
           u_err("unhandled type %d", type);
           return ~0;
    }

    return 0;
}

int json_obj_set_val (json_obj_t *jo, const char *val)
{
    dbg_return_if (jo == NULL, ~0);
    dbg_return_if (val == NULL, ~0);

    dbg_return_if (u_strlcpy(jo->val, val, sizeof jo->val), ~0);

    return 0;
}

int json_obj_set_key (json_obj_t *jo, const char *key)
{
    dbg_return_if (jo == NULL, ~0);
    dbg_return_if (key == NULL, ~0);

    dbg_return_if (u_strlcpy(jo->key, key, sizeof jo->key), ~0);

    return 0;
}

int json_obj_add (json_obj_t *head, json_obj_t *jo)
{
    dbg_return_if (head == NULL, ~0);
    dbg_return_if (jo == NULL, ~0);

#ifdef JSON_OBJ_DEBUG
    u_con("child (%p): %s {%s} added", jo, json_type_str(jo->type), jo->key);
    u_con("parent(%p): %s {%s}\n", head, json_type_str(head->type), head->key);
#endif  /* JSON_OBJ_DEBUG */

    TAILQ_INSERT_TAIL(&head->children, jo, siblings);

    return 0;
}

int json_lex_new (const char *s, json_lex_t **pjl)
{
    json_lex_t *jl = NULL;
    
    warn_err_sif ((jl = u_zalloc(sizeof *jl)) == NULL);
    warn_err_if ((jl->s = u_strdup(s)) == NULL);
    jl->slen = strlen(s);
    jl->err[0] = '\0';
    jl->pos = jl->rmatch = jl->lmatch = 0;

    *pjl = jl;

    return 0;

err:
    json_lex_free(jl);
    return ~0;
}

void json_lex_free (json_lex_t *jl)
{
    if (jl)
    {
        if (jl->s)
            u_free(jl->s);
        u_free(jl);
    }

    return;
}

int json_lex (json_lex_t *jl, json_obj_t **pjo)
{
    json_obj_t *jo = NULL;

    dbg_return_if (jl == NULL, ~0);
    dbg_return_if (pjo == NULL, ~0);
    dbg_return_if (jl->s == NULL, ~0);

    /* Create top level json object. */
    warn_err_if (json_obj_new(&jo));

    /* Consume any trailing white space before starting actual parsing. */
    if (json_lex_eat_ws(jl) == -1)
        JSON_LEX_ERR(jl, "Empty JSON text !");

    /* Launch the lexer expecting the input JSON text as a serialized object 
     * or array. */ 
    if (json_match_object_first(jl))
        warn_err_if (json_match_object(jl, jo));
    else if (json_match_array_first(jl))
        warn_err_if (json_match_array(jl, jo));
    else
        JSON_LEX_ERR(jl, "Expecting \'{\' or \'[\', found \'%c\'.", *jl->s);

    /* Copy out the broken down tree. */
    *pjo = jo;

    return 0;
err:
    json_obj_free(jo);
    return ~0;
}

void json_obj_free (json_obj_t *jo)
{
    size_t dummy = 0;

    if (jo == NULL)
        return;

    /* Walk the tree in post order and free each node while we traverse. */
    json_obj_walk(jo, JSON_WALK_POSTORDER, dummy, json_obj_do_free);

    return;
}

int json_encode (json_obj_t *jo, const char **ps)
{
    u_string_t *s = NULL;

    dbg_return_if (jo == NULL, ~0);
    dbg_return_if (ps == NULL, ~0);

    dbg_err_if (u_string_create(NULL, 0, &s));
    dbg_err_if (json_do_encode(jo, s));
    *ps = u_string_c(s);

    return 0;
err:
    if (s)
        u_string_free(s);
    return ~0;
}

static int json_do_encode (json_obj_t *jo, u_string_t *s)
{
    if (jo == NULL)
        return 0;

    /* [key:] val, */
    if (strlen(jo->key))
        dbg_err_if (u_string_aprintf(s, "%s: ", jo->key));

    switch (jo->type)
    {
        case JSON_TYPE_STRING:
        case JSON_TYPE_NUMBER:
            dbg_err_if (u_string_aprintf(s, "%s", jo->val));
            break;
        case JSON_TYPE_OBJECT:
            dbg_err_if (u_string_aprintf(s, "{"));
            break;
        case JSON_TYPE_ARRAY:
            dbg_err_if (u_string_aprintf(s, "["));
            break;
        case JSON_TYPE_TRUE:
            dbg_err_if (u_string_aprintf(s, "true"));
            break;
        case JSON_TYPE_FALSE:
            dbg_err_if (u_string_aprintf(s, "false"));
            break;
        case JSON_TYPE_NULL:
            dbg_err_if (u_string_aprintf(s, "null"));
            break;
        default:
            dbg_err("!");
    }

    /* TODO if siblings, then add ',' */

    dbg_err_if (json_do_encode(TAILQ_FIRST(&jo->children), s));
    dbg_err_if (json_do_encode(TAILQ_NEXT(jo, siblings), s));

    /* TODO Add tail for array and objects. */ 

    return 0;
err:
    return ~0;
}


/* {Pre,post}-order tree walker, depending on 'strategy'. */
void json_obj_walk (json_obj_t *jo, int strategy, size_t l, 
        void (*cb)(json_obj_t *, size_t))
{
    dbg_return_if (
            strategy != JSON_WALK_PREORDER && 
            strategy != JSON_WALK_POSTORDER, );

    if (jo == NULL)
        return;

    if (strategy == JSON_WALK_PREORDER && cb)
        cb(jo, l);

    /* When recurring into the children branch, increment depth by one. */
    json_obj_walk(TAILQ_FIRST(&jo->children), strategy, l + 1, cb);

    /* Siblings are at the same depth as the current node. */
    json_obj_walk(TAILQ_NEXT(jo, siblings), strategy, l, cb);

    if (strategy == JSON_WALK_POSTORDER && cb)
        cb(jo, l);

    return;
}

void json_obj_print (json_obj_t *jo)
{
    dbg_return_if (jo == NULL, );

    /* Tree root is at '0' depth. */
    json_obj_walk(jo, JSON_WALK_PREORDER, 0, json_obj_do_print);

    return;
}

static int json_match_value (json_lex_t *jl, json_obj_t *jo)
{
    dbg_return_if (jl == NULL, ~0);
    dbg_return_if (jo == NULL, ~0);

    if (json_match_string_first(jl))
        warn_err_if (json_match_string(jl, jo));
    else if (json_match_number_first(jl))
        warn_err_if (json_match_number(jl, jo));
    else if (json_match_object_first(jl))
        warn_err_if (json_match_object(jl, jo));
    else if (json_match_array_first(jl))
        warn_err_if (json_match_array(jl, jo));
    else if (json_match_true_first(jl))
        warn_err_if (json_match_true(jl, jo));
    else if (json_match_false_first(jl))
        warn_err_if (json_match_false(jl, jo));
    else if (json_match_null_first(jl))
        warn_err_if (json_match_null(jl, jo));
    else
        JSON_LEX_ERR(jl, "unexpected value syntax at \'%s\'", STR_REM(jl));

    return 0;
err:
    return ~0;
}

static int json_match_array_first (json_lex_t *jl)
{
    return (json_lex_peek(jl) == '[');
}

static int json_match_object_first (json_lex_t *jl)
{
    return (json_lex_peek(jl) == '{');
}

static int json_match_number_first (json_lex_t *jl)
{
    char r, c = json_lex_peek(jl);

    if ((r = (c == '-' || isdigit(c))))
        json_record_lmatch(jl);

    return r;
}

static int json_match_pair_first (json_lex_t *jl)
{
    return json_match_string_first(jl);
}

static int json_match_false_first (json_lex_t *jl)
{
    return (json_lex_peek(jl) == 'f');
}

static int json_match_true_first (json_lex_t *jl)
{
    return (json_lex_peek(jl) == 't');
}

static int json_match_null_first (json_lex_t *jl)
{
    return (json_lex_peek(jl) == 'n');
}

static int json_match_string_first (json_lex_t *jl)
{
    char r, c = json_lex_peek(jl);

    if ((r = (c == '"')))
        json_record_lmatch(jl);

    return r;
}

const char *json_lex_geterr (json_lex_t *jl)
{
    dbg_return_if (jl == NULL, "uh?!");
    return jl->err;
}

static int json_lex_seterr (json_lex_t *jl, const char *fmt, ...)
{
    va_list ap;

    dbg_return_if (jl == NULL, ~0);

    va_start(ap, fmt);
    (void) vsnprintf(jl->err, sizeof jl->err, fmt, ap); 
    va_end(ap);

    return 0;
}

/* Get next char (whitespace too). */
static int json_lex_next (json_lex_t *jl, char *pb)
{
    return json_lex_next_ex(jl, 0, pb);
}

/* Get next non-whitespace char. */
static int json_lex_skip (json_lex_t *jl, char *pb)
{
    return json_lex_next_ex(jl, 1, pb);
}

static int json_lex_eot (json_lex_t *jl)
{
    return (jl->pos > jl->slen);
}

/* Must always be called after proper sanitization by json_lex_eot. */
static void json_lex_incr (json_lex_t *jl)
{
    jl->pos += 1;
#ifdef JSON_LEX_DEBUG
    u_con("\'%c\' -> \'%c\'", *(jl->s + jl->pos), *(jl->s + jl->pos + 1));
#endif  /* JSON_LEX_DEBUG */
    return;
}

/* '-1' EOT, '0' ok */
static int json_lex_next_ex (json_lex_t *jl, int eat_ws, char *pb)
{
    dbg_return_if (json_lex_eot(jl), -1);

    /* Consume at least one char. */
    json_lex_incr(jl);

    /* If requested, skip white spaces. */
    if (eat_ws)
        dbg_return_if (json_lex_eat_ws(jl) == -1, -1);

    /* If requested, copy out the accepted char. */
    if (pb)
        *pb = json_lex_peek(jl);

    return 0;
}

static int json_lex_eat_ws (json_lex_t *jl)
{
    dbg_return_if (json_lex_eot(jl), -1);

    while (isspace(jl->s[jl->pos]))
    {
        dbg_return_if (json_lex_eot(jl), -1);
        json_lex_incr(jl);
    }

    return 0;
}

static char json_lex_peek (json_lex_t *jl)
{
    return jl->s[jl->pos];
}

/* number ::= INT[FRAC][EXP] */
static int json_match_number (json_lex_t *jl, json_obj_t *jo)
{
    char match[JSON_VAL_SZ];

    /* INT is mandatory */
    warn_err_if (json_match_int(jl));

    /* c IN first(FRAC) */
    if (json_match_frac_first(json_lex_peek(jl)))
        warn_err_if (json_match_frac(jl));

    /* c IN first(EXP) */
    if (json_match_exp_first(json_lex_peek(jl)))
        warn_err_if (json_match_exp(jl));

    /* Register right side of the matched number. */
    json_record_rmatch(jl);

    /* Push the matched number into the supplied json object. */
    warn_err_if (json_obj_set_type(jo, JSON_TYPE_NUMBER));
    warn_err_if (json_obj_set_val(jo, json_get_match(jl, match)));

#ifdef JSON_LEX_DEBUG
    u_con("matched number: %s", json_get_match(jl, match));
#endif  /* JSON_LEX_DEBUG */

    return 0;
err:
    return ~0;
}

static int json_match_int (json_lex_t *jl)
{
    char c = json_lex_peek(jl);

    /* optional minus sign */
    if (c == '-')
        JSON_LEX_NEXT(jl, &c);

    /* on '0' as the first char, we're done */
    if (c == '0')
    {
        JSON_LEX_NEXT(jl, &c);
        goto end;
    }

    /* [1-9][0-9]+ */
    if (c >= 48 && c <= 57)
        do { JSON_LEX_NEXT(jl, &c); } while (isdigit(c));
    else 
        JSON_LEX_ERR(jl, "bad int syntax at %s", STR_REM(jl));

    /* fall through */
end:
    return 0;
err:
    return ~0;
}

static int json_match_exp_first (char c)
{
    return (c == 'e' || c == 'E');
}

static int json_match_frac_first (char c)
{
    return (c == '.');
}

static int json_match_frac (json_lex_t *jl)
{
    char c = json_lex_peek(jl);

    /* Mandatory dot. */
    if (c != '.')
        JSON_LEX_ERR(jl, "bad frac syntax at %s", STR_REM(jl));

    JSON_LEX_NEXT(jl, &c);

    /* [0-9] */
    if (!isdigit(c))
        JSON_LEX_ERR(jl, "bad frac syntax at %s", STR_REM(jl));

    /* [0-9]* */
    while (isdigit(c))
        JSON_LEX_NEXT(jl, &c);

    return 0;
err:
    return ~0;
}

static int json_match_exp (json_lex_t *jl)
{
    char c = json_lex_peek(jl);

    /* [eE] */
    if (c != 'e' && c != 'E')
        JSON_LEX_ERR(jl, "bad exp syntax at %s", STR_REM(jl));

    JSON_LEX_NEXT(jl, &c);

    /* Optional plus/minus sign. */
    if (c == '+' || c == '-')
        JSON_LEX_NEXT(jl, &c);

    /* [0-9] */
    if (!isdigit(c))
        JSON_LEX_ERR(jl, "bad exp syntax at %s", STR_REM(jl));

    /* [0-9]* */
    while (isdigit(c))
        JSON_LEX_NEXT(jl, &c);

    return 0;
err:
    return ~0;
}

static int json_match_seq (json_lex_t *jl, json_obj_t *jo, int type, char first,
        const char *rem, size_t rem_sz)
{
    char c;
    size_t i = 0;

    if ((c = json_lex_peek(jl)) != first)
    {
        JSON_LEX_ERR(jl, "expect \'%c\', got %c at %s", 
                first, c, STR_REM(jl));
    }

    for (i = 0; i < rem_sz; i++)
    {
        JSON_LEX_SKIP(jl, &c);
        if (c != *(rem + i))
        {
            JSON_LEX_ERR(jl, "expect \'%c\', got %c at %s", 
                    *(rem + i), c, STR_REM(jl));
        }
    }

    /* Consume last checked char. */
    JSON_LEX_SKIP(jl, NULL);

    warn_err_if (json_obj_set_type(jo, type));

#ifdef JSON_LEX_DEBUG
    u_con("matched \'%s\' sequence", json_type_str(type));
#endif  /* JSON_LEX_DEBUG */
    return 0;
err:
    return ~0;
}

static int json_match_null (json_lex_t *jl, json_obj_t *jo)
{
    return json_match_seq(jl, jo, JSON_TYPE_NULL, 'n', "ull", strlen("ull"));
}

static int json_match_true (json_lex_t *jl, json_obj_t *jo)
{
    return json_match_seq(jl, jo, JSON_TYPE_TRUE, 't', "rue", strlen("rue"));
}

static int json_match_false (json_lex_t *jl, json_obj_t *jo)
{
    return json_match_seq(jl, jo, JSON_TYPE_FALSE, 'f', "alse", strlen("alse"));
}

static int json_match_array (json_lex_t *jl, json_obj_t *jo)
{
    char c;
    json_obj_t *elem = NULL;

#ifdef JSON_LEX_DEBUG
    u_con("ARRAY");
#endif  /* JSON_LEX_DEBUG */

    if ((c = json_lex_peek(jl)) != '[')
        JSON_LEX_ERR(jl, "expect \'[\', got %c at %s", c, STR_REM(jl));

    /* Parent object is an array. */
    warn_err_if (json_obj_set_type(jo, JSON_TYPE_ARRAY));

    do {
        JSON_LEX_SKIP(jl, &c);

        if (c == ']')   /* break on empty array */
            break;

        /* Create a new object to store next array element. */
        warn_err_if (json_obj_new(&elem));
        warn_err_if (json_obj_set_type(elem, JSON_TYPE_UNKNOWN));

        /* Fetch new value. */
        warn_err_if (json_match_value(jl, elem));

        /* Push the fetched element to its parent array. */
        warn_err_if (json_obj_add(jo, elem)); 
        elem = NULL;

        /* Consume any trailing white spaces. */
        if (isspace(json_lex_peek(jl)))
            JSON_LEX_SKIP(jl, NULL);

    } while (json_lex_peek(jl) == ',');

    if ((c = json_lex_peek(jl)) != ']')
        JSON_LEX_ERR(jl, "expect \']\', got %c at %s", c, STR_REM(jl));

    JSON_LEX_SKIP(jl, &c);

    return 0;
err:
    json_obj_free(elem);
    return ~0;
}

static int json_match_object (json_lex_t *jl, json_obj_t *jo)
{
    char c;

#ifdef JSON_LEX_DEBUG
    u_con("OBJECT");
#endif  /* JSON_LEX_DEBUG */

    if ((c = json_lex_peek(jl)) != '{')
        JSON_LEX_ERR(jl, "expect \'{\', got %c at %s", c, STR_REM(jl));

    warn_err_if (json_obj_set_type(jo, JSON_TYPE_OBJECT));

    do {
        JSON_LEX_SKIP(jl, &c);

        /* Break on empty object. */
        if (c == '}')
            break;

        /* Process assignement. */
        warn_err_if (!json_match_pair_first(jl) || json_match_pair(jl, jo));

        /* Consume trailing white spaces, if any. */
        if (isspace(json_lex_peek(jl)))
            JSON_LEX_SKIP(jl, NULL);

    } while (json_lex_peek(jl) == ',');

    if ((c = json_lex_peek(jl)) != '}')
        JSON_LEX_ERR(jl, "expect \'}\', got %c at %s", c, STR_REM(jl));

    JSON_LEX_SKIP(jl, &c);

    return 0;
err:
    return ~0;
}

static int json_match_pair (json_lex_t *jl, json_obj_t *jo)
{
    char c, match[JSON_VAL_SZ];
    json_obj_t *pair = NULL;

    dbg_return_if (jl == NULL, ~0);
    dbg_return_if (jo == NULL, ~0);

#ifdef JSON_LEX_DEBUG
    u_con("PAIR");
#endif  /* JSON_LEX_DEBUG */

    /* Here we use the matched string as the 'key' for the associated value, 
     * hence there is no associated json object. */
    warn_err_if (json_match_string(jl, NULL));

    /* Initialize new json object to store the key/value pair. */
    warn_err_if (json_obj_new(&pair));
    warn_err_if (json_obj_set_key(pair, json_get_match(jl, match)));

    /* Consume ':' */
    if ((c = json_lex_peek(jl)) != ':')
        JSON_LEX_ERR(jl, "expect \':\', got %c at %s", c, STR_REM(jl));
    JSON_LEX_SKIP(jl, &c);

    /* Assign value. */
    warn_err_if (json_match_value(jl, pair));

    /* Push the new value to the parent json object. */
    warn_err_if (json_obj_add(jo, pair));
    pair = NULL;

    return 0;
err:
    json_obj_free(pair);
    return ~0;
}

static int json_match_string (json_lex_t *jl, json_obj_t *jo)
{
    char c, match[JSON_VAL_SZ];

    /* In case string is matched as an lval (i.e. the key side of a 'pair'),
     * there is no json object. */
    dbg_return_if (jl == NULL, ~0);

#ifdef JSON_LEX_DEBUG
    u_con("STRING");
#endif  /* JSON_LEX_DEBUG */

    if ((c = json_lex_peek(jl)) != '"')
        JSON_LEX_ERR(jl, "expect \", got %c at %s", c, STR_REM(jl));

    JSON_LEX_NEXT(jl, &c);

    while (c != '"')
    {
        if (c == '\\')
        {
            JSON_LEX_NEXT(jl, &c);
            switch (c)
            {
                case 'u':
                    warn_err_if (json_match_escaped_unicode(jl));
                    break;
                case '"': case '\\': case '/': case 'b':
                case 'f': case 'n':  case 'r': case 't':
                    JSON_LEX_NEXT(jl, &c);
                    break;
                default:
                    JSON_LEX_ERR(jl, "invalid char %c in escape", c);
            }
        }
        else if (iscntrl(c))
            JSON_LEX_ERR(jl, "control character in string", c); 
        else    
            JSON_LEX_NEXT(jl, &c);
    }

    /* Consume last '"'. */
    JSON_LEX_NEXT(jl, &c);

    /* Record right match pointer. */
    json_record_rmatch(jl);

#ifdef JSON_LEX_DEBUG
    u_con("matched string: \'%s\'", json_get_match(jl, match));
#endif  /* JSON_LEX_DEBUG */

    /* In case the string is matched as an rval, the caller shall
     * supply the json object that has to be set. */
    if (jo)
    {
        warn_err_if (json_obj_set_type(jo, JSON_TYPE_STRING));
        warn_err_if (json_obj_set_val(jo, json_get_match(jl, match)));
    }

    return 0;
err:
    return ~0;
}

static int json_match_escaped_unicode (json_lex_t *jl)
{
    char i, c;

    for (i = 0; i < 4; i++)
    {
        JSON_LEX_SKIP(jl, &c);

        if (!isxdigit(c))
            JSON_LEX_ERR(jl, "non hex digit %c in escaped unicode", c); 
    }

    return 0;
err:
    return ~0;
}

static void json_record_lmatch (json_lex_t *jl)
{
    jl->lmatch = jl->pos;
    return;
}

static void json_record_rmatch (json_lex_t *jl)
{
    jl->rmatch = jl->pos;
    return;
}

static size_t json_strlen_match (json_lex_t *jl)
{
    return (jl->rmatch - jl->lmatch);
}

static char *json_get_match (json_lex_t *jl, char match[JSON_VAL_SZ])
{
    size_t len;

    dbg_return_if (match == NULL, NULL);
    dbg_return_if (jl->rmatch < jl->lmatch, NULL);
    dbg_return_if ((len = json_strlen_match(jl)) >= JSON_VAL_SZ, NULL);

    memcpy(match, jl->s + jl->lmatch, len);
    match[len] = '\0';

    return match;
}

static const char *json_type_str (int type)
{
    switch (type)
    {
        case JSON_TYPE_STRING:  return "string";
        case JSON_TYPE_NUMBER:  return "number";
        case JSON_TYPE_ARRAY:   return "array";
        case JSON_TYPE_OBJECT:  return "object";
        case JSON_TYPE_TRUE:    return "true";
        case JSON_TYPE_FALSE:   return "false";
        case JSON_TYPE_NULL:    return "null";
        case JSON_TYPE_UNKNOWN: default: break;
    }

    return "unknown";
}

static void json_obj_do_free (json_obj_t *jo, size_t l)
{
    u_unused_args(l);

    if (jo)
        u_free(jo);

    return;
}

static void json_obj_do_print (json_obj_t *jo, size_t l)
{
    dbg_return_if (jo == NULL, );

    switch (jo->type)
    {
        case JSON_TYPE_ARRAY:
        case JSON_TYPE_OBJECT:
            /* No value. */
            u_con("%*c %s %s", l, ' ', json_type_str(jo->type), jo->key);
            break;
        default:
            u_con("%*c %s %s : \'%s\'", l, ' ', 
                    json_type_str(jo->type), jo->key, jo->val);
            break;
    }

    return;
}
