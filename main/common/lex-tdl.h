/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* lex TDL files */

#ifndef _LEX_TDL_H_
#define _LEX_TDL_H_

#include "lex-io.h"

#define N_KEYWORDS 19
extern char *keywords[N_KEYWORDS];

enum TOKEN_TAG {T_NONE, T_EOF, T_WS, T_COMM, T_STRING, T_ID, T_INT, T_DOT, T_COMMA, T_COLON,
		 T_EQUALS, T_HASH, T_QUOTE, T_AMPERSAND, T_AT, T_DOLLAR, T_LPAREN,
		 T_RPAREN, T_LBRACKET, T_RBRACKET, T_LANGLE, T_RANGLE, T_LDIFF, T_RDIFF,
		 T_ISA, T_ISEQ, T_LISP, T_KEYWORD, T_FLOAT, T_INFLR, T_ERROR };


struct lex_token
{
  char *text;
  enum TOKEN_TAG tag;
  struct lex_location *loc;
};

extern int tokensdelivered;

struct lex_token *get_token();
struct lex_token *LA(int n);

void consume(int n);
void optional(enum TOKEN_TAG tag);

char *match(enum TOKEN_TAG tag, char *s, bool readonly);
void match_keyword(char *kwd);
int is_keyword(struct lex_token *t, char *kwd);

void syntax_error(char *msg, struct lex_token *t);
void recover(enum TOKEN_TAG tag);

extern char *lexer_idchars;

#endif


