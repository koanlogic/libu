/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l.
 */

#include <ctype.h>

#include <toolbox/carpal.h>
#include <toolbox/lexer.h>
#include <toolbox/memory.h>
#include <toolbox/misc.h>

/* Lexer context. */
struct u_lexer_s
{
    char *s;        /* NUL-terminated string, that shall be parsed. */
    size_t slen;    /* String length (excluding ending '\0'). */
    size_t pos;     /* Actual lexer position. */
    size_t lmatch;  /* Offset of current left side match. */
    size_t rmatch;  /* Offset of current right side match. */
    char err[U_LEXER_ERR_SZ];   /* Error string. */
};

static void u_lexer_incr (u_lexer_t *l);
static int u_lexer_next_ex (u_lexer_t *l, int eat_ws, char *pb);
static size_t u_lexer_strlen_match (u_lexer_t *l);

/**
    \defgroup lexer Lexical Tokenizer
    \{
 */

/**
 *  \brief  Create a new lexer context associated to the NUL-terminated 
 *          string \p s.
 *
 *  \param  s   Pointer to the string that has to be parsed.
 *  \param  pl  Handler for the associated lexer instance as a result argument.
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_lexer_new (const char *s, u_lexer_t **pl)
{
    u_lexer_t *l = NULL;

    dbg_return_if (s == NULL, ~0);
    dbg_return_if (pl == NULL, ~0);
    
    /* Make room for the object. */
    warn_err_sif ((l = u_zalloc(sizeof *l)) == NULL);

    /* Internalize the string to be parsed. */
    warn_err_if ((l->s = u_strdup(s)) == NULL);
    l->slen = strlen(s);

    /* Init offset counters. */
    l->pos = l->rmatch = l->lmatch = 0;

    /* Error string. */
    l->err[0] = '\0';

    /* Copy out. */
    *pl = l;

    return 0;
err:
    u_lexer_free(l);
    return ~0;
}

/**
 *  \brief  Return a pointer to the substring that has not yet been parsed.
 *
 *  \param  l   An active lexer context.
 *
 *  \return a pointer to the substring still to be parsed
 */
const char *u_lexer_lookahead (u_lexer_t *l)
{
    return &l->s[l->pos];
}

/** 
 *  \brief  Destroy a previously allocated lexer context. 
 *
 *  \param  l   Handler to a previously created ::u_lexer_t object.
 *
 *  \return nothing
 */
void u_lexer_free (u_lexer_t *l)
{
    /* Don't moan on NULL objects. */
    if (l)
    {
        if (l->s)
            u_free(l->s);
        u_free(l);
    }

    return;
}

/** 
 *  \brief  Accessor method for lexer error string. 
 *
 *  \param  l   Handler of a lexer context that may have generated an error.
 *
 *  \return the error string (if any)
 */
const char *u_lexer_geterr (u_lexer_t *l)
{
    dbg_return_if (l == NULL, NULL);

    return l->err;
}

/** 
 *  \brief  Setter method for lexer error string. 
 *
 *  \param  l   Handler of an active lexer context.
 *  \param  fmt printf-like format string.
 *  \param  ... variable argument list to feed \p fmt.
 *
 *  \retval  0  on success
 *  \retval ~0  on failure (bad parameters supplied)
 */
int u_lexer_seterr (u_lexer_t *l, const char *fmt, ...)
{
    va_list ap;

    dbg_return_if (l == NULL, ~0);
    dbg_return_if (fmt == NULL, ~0);

    va_start(ap, fmt);
    (void) vsnprintf(l->err, sizeof l->err, fmt, ap); 
    va_end(ap);

    return 0;
}

/** 
 *  \brief  Get next char (included whitespaces) and advance lexer position 
 *          by one. 
 *
 *  \param  l   Handler of an active lexer context.
 *  \param  pb  If non-NULL it will get the next stream character.
 *
 *  \return See ::u_lexer_next_ex return codes
 */
int u_lexer_next (u_lexer_t *l, char *pb)
{
    return u_lexer_next_ex(l, 0, pb);
}

/** 
 *  \brief  Get next non-whitespace char and advance lexer position 
 *          accordingly. 
 *
 *  \param  l   Handler of an active lexer context.
 *  \param  pb  If non-NULL it will get the next non-whitespace characted from
 *              the stream.
 *
 *  \return See ::u_lexer_next_ex return codes
 */
int u_lexer_skip (u_lexer_t *l, char *pb)
{
    return u_lexer_next_ex(l, 1, pb);
}

/** 
 *  \brief  Tell if we've reached the end of the string. 
 *
 *  \param  l   Handler of an active lexer context.
 *
 *  \retval 0   if actual position is not at the end of text
 *  \retval 1   if actual position is at the end of text 
 */
int u_lexer_eot (u_lexer_t *l)
{
    return (l->pos >= l->slen);
}

/** 
 *  \brief  Advance lexer position until a non-whitespace char is found. 
 *
 *  \param  l   An active lexer context.
 *
 *  \retval  0  on success
 *  \retval -1  on end of text
 */
int u_lexer_eat_ws (u_lexer_t *l)
{
    dbg_return_if (l == NULL, -1);
    dbg_return_if (u_lexer_eot(l), -1);

    while (isspace((int) l->s[l->pos]))
    {
        dbg_return_if (u_lexer_eot(l), -1);
        u_lexer_incr(l);
    }

    return 0;
}

/** 
 *  \brief  Get the character actually under the lexer cursor.
 *
 *  \param  l   An active lexer context.
 *
 *  \return the character actually under the cursor.
 */
char u_lexer_peek (u_lexer_t *l)
{
    return l->s[l->pos];
}

/** 
 *  \brief  Register left side (i.e. beginning) of a match. 
 *
 *  \param  l   An active lexer context.
 *
 *  \return nothing
 */
void u_lexer_record_lmatch (u_lexer_t *l)
{
    l->lmatch = l->pos;
    return;
}

/** 
 *  \brief  Register right side (i.e. closing) of the current match.
 *
 *  \param  l   An active lexer context.
 *
 *  \return nothing
 */
void u_lexer_record_rmatch (u_lexer_t *l)
{
    l->rmatch = l->pos;
    return;
}

/** 
 *  \brief  Extract the matched sub-string. 
 *
 *  \param  l       An active lexer context.
 *  \param  match   A buffer of at least ::U_TOKEN_SZ bytes that will hold
 *                  the matched NUL-terminated substring.
 *
 *  \return the matched substring
 */
char *u_lexer_get_match (u_lexer_t *l, char match[U_TOKEN_SZ])
{
    size_t len;

    dbg_return_if (match == NULL, NULL);
    dbg_return_if (l->rmatch < l->lmatch, NULL);
    dbg_return_if ((len = u_lexer_strlen_match(l)) >= U_TOKEN_SZ, NULL);

    memcpy(match, l->s + l->lmatch, len);
    match[len] = '\0';

    return match;
}

/**
 *  \brief  Expect to find the supplied character under lexer cursor.
 *
 *  \param  l           An active lexer context.
 *  \param  expected    The character that we expect to be found under the
 *                      lexer cursor.
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_lexer_expect_char (u_lexer_t *l, char expected)
{
    char c = u_lexer_peek(l);

    /* The expected char must be under lexer cursor.  If matched, the
     * cursor is advanced by one position. */
    if (c == expected)
        U_LEXER_NEXT(l, NULL);
    else
        U_LEXER_ERR(l, "expecting \'%c\', got \'%c\' instead", expected, c);

    return 0;
err:
    return ~0;
}

/**
 *  \brief  Return the actual position of the lexer cursor.
 *
 *  \param  l   An active lexer context.
 *
 *  \return the actual string offset of the lexer cursor.
 */ 
size_t u_lexer_pos (u_lexer_t *l)
{
    return l->pos;
}

/**
 *  \}
 */ 

/* Must always be called after proper sanitization by u_lexer_eot. */
static void u_lexer_incr (u_lexer_t *l)
{
    l->pos += 1;
#ifdef U_LEXER_DEBUG
    u_con("\'%c\' -> \'%c\'", *(l->s + l->pos), *(l->s + l->pos + 1));
#endif  /* U_LEXER_DEBUG */
    return;
}

/* '-1' EOT, '0' ok */
static int u_lexer_next_ex (u_lexer_t *l, int eat_ws, char *pb)
{
    dbg_return_if (u_lexer_eot(l), -1);

    /* Consume at least one char. */
    u_lexer_incr(l);

    /* If requested, skip white spaces. */
    if (eat_ws)
        dbg_return_if (u_lexer_eat_ws(l) == -1, -1);

    /* If requested, copy out the accepted char. */
    if (pb)
        *pb = u_lexer_peek(l);

    return 0;
}

static size_t u_lexer_strlen_match (u_lexer_t *l)
{
    return (l->rmatch - l->lmatch + 1);
}
