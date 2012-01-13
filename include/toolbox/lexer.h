/*
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.
 */

#ifndef _U_LEXER_H_
#define _U_LEXER_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef U_LEXER_ERR_SZ
  #define U_LEXER_ERR_SZ    512
#endif  /* !U_LEXER_ERR_SZ */

struct u_lexer_s;

/**
 *  \addtogroup lexer
 *  \{
 */ 

/** \brief  Lexer base type. */
typedef struct u_lexer_s u_lexer_t;

/** \brief  Maximum size of input tokens (can be changed at compile time via 
 *          \c -DU_TOKEN_SZ=nnn compiler flag) */
#ifndef U_TOKEN_SZ
  #define U_TOKEN_SZ    128
#endif  /* !U_TOKEN_SZ */

/*
 * All the following U_LEXER_ macros need the "err" label in scope.
 */

/** \brief  ::u_lexer_pos wrapper. */
#define U_LEXER_ERR(l, ...)                                         \
    do {                                                            \
        (void) u_lexer_seterr(l, __VA_ARGS__);                      \
        goto err;                                                   \
    } while (0)

/** \brief  ::u_lexer_skip wrapper. */
#define U_LEXER_SKIP(l, pc)                                         \
    do {                                                            \
        if (u_lexer_skip(l, pc))                                    \
            U_LEXER_ERR(l, "EOT at offset %zu", u_lexer_pos(l));    \
    } while (0)

/** \brief  ::u_lexer_next wrapper. */
#define U_LEXER_NEXT(l, pc)                                         \
    do {                                                            \
        if (u_lexer_next(l, pc))                                    \
            U_LEXER_ERR(l, "EOT at offset %zu", u_lexer_pos(l));    \
    } while (0)

int u_lexer_new (const char *s, u_lexer_t **pl);
void u_lexer_free (u_lexer_t *l);
const char *u_lexer_geterr (u_lexer_t *l);

int u_lexer_next (u_lexer_t *l, char *pb);
int u_lexer_skip (u_lexer_t *l, char *pb);
char u_lexer_peek (u_lexer_t *l);
int u_lexer_seterr (u_lexer_t *l, const char *fmt, ...);
void u_lexer_record_lmatch (u_lexer_t *l);
void u_lexer_record_rmatch (u_lexer_t *l);
char *u_lexer_get_match (u_lexer_t *l, char match[U_TOKEN_SZ]);
int u_lexer_eot (u_lexer_t *l);
int u_lexer_eat_ws (u_lexer_t *l);
int u_lexer_expect_char (u_lexer_t *l, char expected);
size_t u_lexer_pos (u_lexer_t *l);
const char *u_lexer_lookahead (u_lexer_t *l);

/**
 *  \}
 */ 

#ifdef __cplusplus
}
#endif

#endif  /* !_U_LEXER_H_ */
