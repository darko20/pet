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

/** \file lex-tdl.h
 * Lexer definitions for TDL files
 */

#ifndef _LEX_TDL_H_
#define _LEX_TDL_H_

#include "lex-io.h"

/** The number of predefined keywords of the TDL syntax. */
#define N_KEYWORDS 18
/** The predefined TDL keywords */
extern char *keywords[N_KEYWORDS];

/** Enumeration for the different token types */
enum TOKEN_TAG {T_NONE, T_EOF, T_WS, T_COMM, T_STRING, T_ID, T_DOT, T_COMMA, T_COLON,
		 T_EQUALS, T_HASH, T_QUOTE, T_AMPERSAND, T_AT, T_DOLLAR, T_LPAREN,
		 T_RPAREN, T_LBRACKET, T_RBRACKET, T_LANGLE, T_RANGLE, T_LDIFF, T_RDIFF,
		 T_ARROW, T_ISA, T_ISEQ, T_LISP, T_KEYWORD, T_INFLR, T_ERROR };

/** lexical token object, containing surface string, type and location. */
struct lex_token
{
  /** the token value, verbatim */
  char *text;
  /** the token type */
  enum TOKEN_TAG tag;
  /** location where the token occured */
  struct lex_location *loc;
};

/** The number of consumed tokens, except for whitespace and comments */
extern int tokensdelivered;

/** get the next token */
struct lex_token *get_token();
/** Syntactic lookahead: get the token \a n tokens away from the current
 *  position.
 * Works for \f$ 0 \leq n \leq \mbox{\tt MAX\_LA} \f$ , which is configurable in
 * lex-tdl.cpp
 */
struct lex_token *LA(int n);

/** Consume \a n tokens */
void consume(int n);
/** Consume the next token if its type is equal to \a tag */
void optional(enum TOKEN_TAG tag);

/** \brief Try to match the next token. Issue a syntax error if the next token
 *  does not fit.
 *  \param tag The type of token that is expected
 *  \param s A more detailed description of the expected token for error
 *           messages.
 *  \param readonly If \c false, the surface string of the token is returned,
 *                  \c NULL otherwise.
 */
char *match(enum TOKEN_TAG tag, char *s, bool readonly);
/**  \brief Match and consume the keyword with surface string \a kwd. Issue a
 *  syntax error if the next token does not fit.
 */
void match_keyword(char *kwd);
/** \brief Check if the token \a t is a keyword with surface form \a
 *  kwd. Return non-zero if so, zero otherwise. 
 */
int is_keyword(struct lex_token *t, char *kwd);

/** Print a syntax error to the error channel */
void syntax_error(char *msg, struct lex_token *t);
/** Consume tokens until one with type \a tag is found, then consume this token
 *  too and return.
 */
void recover(enum TOKEN_TAG tag);

/** The set of non-alphanumeric characters that may occur in an \c id token */
extern char *lexer_idchars;
/** A predicate that returns \c true if \a c is a character that may occur in
 *  an \c id token .
 */
extern int is_idchar(int c);

#endif


