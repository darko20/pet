/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2002 Ulrich Callmeier uc@coli.uni-sb.de
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* lex TDL files */

#ifndef _LEX_TDL_H_
#define _LEX_TDL_H_

#include "lex-io.h"

#define N_KEYWORDS 19
extern char *keywords[N_KEYWORDS];

enum TOKEN_TAG {T_NONE, T_EOF, T_WS, T_COMM, T_STRING, T_ID, T_DOT, T_COMMA, T_COLON,
		 T_EQUALS, T_HASH, T_QUOTE, T_AMPERSAND, T_AT, T_DOLLAR, T_LPAREN,
		 T_RPAREN, T_LBRACKET, T_RBRACKET, T_LANGLE, T_RANGLE, T_LDIFF, T_RDIFF,
		 T_ISA, T_ISEQ, T_LISP, T_KEYWORD, T_INFLR, T_ERROR };


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
extern int is_idchar(int c);

#endif


