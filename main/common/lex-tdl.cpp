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

/* lexer for files in TDL syntax */

#include "lex-tdl.h"
#include "errors.h"
#include "logging.h"

#ifdef FLOP
#include "flop.h"
#endif

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iomanip>

const char *keywords[N_KEYWORDS] = { "declare", "domain", "instance", "lisp",
"template", "type", "begin", "defdomain", "deldomain", "delete-package-p",
"end", "end!", "errorp", "expand-all-instances", "include", "leval",
"sorts", "status" };

const char *lexer_idchars = "_+-*?";

int is_idchar(int c)
{
  return isalnum(c) || strchr(lexer_idchars, c) || c > 127 || c < 0;
}

int lisp_mode = 0; // shall lexer recognize lisp expressions 

void print_token(std::ostream &out, struct lex_token *t);

struct lex_token *make_token(enum TOKEN_TAG tag, const char *s, int len,
    int rlen = -1)
{
  struct lex_token *t;

  t = (struct lex_token *) malloc(sizeof(struct lex_token));

  if(tag != T_EOF)
    {
      t->loc = new_location(curr_fname(), curr_line(), curr_col());
    }
  else
    {
      t->loc = new_location("unknown", 0, 0);
    }

  t->tag = tag;

  if(s == NULL)
    {
      t->text = NULL;
    }
  else
    {
      if (rlen == -1)
        rlen = len;
      char *dest = t->text = (char *) malloc(rlen + 1);
      if (rlen == len)
        strncpy(dest, s, len);
      else // s contains a string with escaped characters
        {
          int j = 0;
          for (int i = 0; i < len; i++)
            {
              if (s[i] == '\\')
                i++; // skip, copy following character
              dest[j++] = s[i];
            }
          assert(j == rlen);
        }
      dest[rlen] = '\0';
    }

  return t;
}

void check_id(struct lex_token *t)
{
  int i;

  for(i = 0; i< N_KEYWORDS; i++)
    {
      if(strcmp(t->text, keywords[i]) == 0)
        {
          t->tag = T_KEYWORD;
          break;
        }
    }
}

struct lex_token *get_next_token()
{
  int c;
  struct lex_token *t;

  c = LLA(0);

  if(c == EOF) 
    {
      t = make_token(T_EOF, NULL, 0);
    }
  else if(isspace(c))
    { // whitespace
      int i;
      
      i = 1;
      while(isspace(LLA(i))) i++;
      
      t = make_token(T_WS, NULL, i);
      LConsume(i);
    }
  else if(c == ';')
    { // single line comment
      char *start;
      int i;

      start = LMark();
      i = 1;

      while(LLA(i) != '\n' && LLA(i) != EOF) i++;

      t = make_token(T_COMM, start, i);
      
#ifdef FLOP
      if(t->text[1] == '%')
        {
          if(strncmp(t->text+2, "+redefine", 9) == 0)
            allow_redefinitions = 1;
          else if(strncmp(t->text, "-redefine", 9) == 0)
            allow_redefinitions = 0;
        }
#endif

      LConsume(i);
    }
  else if(c == '#' && LLA(1) == '|')
  { /* block comment */
      char *start;

      start = LMark();
      
      int i = 2;
      
      while(LLA(i) != EOF)
      {
          if(LLA(i) == '|' && LLA(i+1) == '#')
          {
              break;
          }
          i++;
      }
      
      if(LLA(i) == EOF)
      { // runaway comment
        throw tError("runaway block comment", curr_fname(),
                     curr_line(), curr_col());
      }
      
      i += 2;
      t = make_token(T_COMM, start, i);
      LConsume(i);
  }
  else if(c == '"')
    { // string
      char *start;
      int i;
      int l; // length of the resolved string (without escape backslashes)

      start = LMark();
      i = 1;
      l = 0;

      while(LLA(i) != EOF && LLA(i) != '"')
        {
          if (LLA(i) == '\\')
            i++; // skip
          i++;
          l++;
        }

      if(LLA(i) == EOF)
        { // runaway string
          throw tError("runaway string", curr_fname(), curr_line(), curr_col());
        }

      i += 1;
      t = make_token(T_STRING, start + 1, i - 2, l);
      LConsume(i);
    }
  else if(c == '(' && lisp_mode)
    { // LISP expression
      char *start;
      int i, parlevel;

      start = LMark();
      i = 1; parlevel = 1;

      while(parlevel > 0 && LLA(i) != EOF)
        {
          if(LLA(i) == '(') parlevel++;
          else if(LLA(i) == ')') parlevel--;
          i++;
        }

      if(LLA(i) == EOF)
       { // runaway LISP expression
         throw tError("runaway LISP expression", curr_fname(),
                      curr_line(), curr_col());
       }
      
      t = make_token(T_LISP, start, i);
      LConsume(i);

    }
  else if(c == '-' && LLA(1) == '-' && LLA(2) == '>')
    // this has to be checked before we check for identifiers, because
    // -- is the start of a valid TDL identifier
    {
      t = make_token(T_ARROW, "-->", 3);
      LConsume(3);
    }
  else if(is_idchar(c) || isdigit(c) || c == '-')
    { /* identifier/number (or command etc.) */

      // _fix_me_
      // This isn't particularly elegant or robust. The error
      // handling is not good. There's no clear way to integrate
      // general numbers seamlessly into TDL...

      char *start;
      int i = 0;
      
      start = LMark();

      char *endptr = 0;
      if(c == '-' || isdigit(c))
      {
          // First, let's see if this can be a number.
          i = 1;
          while(is_idchar(LLA(i)) || LLA(i) == '.' || LLA(i) == '-'
                || LLA(i) == '+' || LLA(i) == 'e' || LLA(i) == 'E' )
              i++;
          
          // Remove stuff that shouldn't be at the end.
          while(i > 1 && (LLA(i-1) == '-' || LLA(i-1) == '+' || LLA(i-1) == '.'
                          || LLA(i-1) == 'e' || LLA(i-1) == 'E'))
              i--;
          
          strtod(start, &endptr);
      }

      if(endptr != start + i)
      {
          // See if it's an identifier.
          i = 0;
          while(is_idchar(LLA(i)) || LLA(i) == '!') i++;
          
          if(i == 0)
          { char str[3] = { (char) c, '\'', '\0' };
            throw tError("unexpected character '" + std::string(str),
                         curr_fname(), curr_line(), curr_col());
          }
      }
          
      t = make_token(T_ID, start, i);
      LConsume(i);
          
      check_id(t);
  }
  else if(c == ':')
    {
      if(LLA(1) == '<')
        {
          t = make_token(T_ISA, ":<", 2);
          LConsume(2);
        }
      else if(LLA(1) == '=')
        {
          t = make_token(T_ISEQ, ":=", 2);
          LConsume(2);
        }
      else if(LLA(1) == '+')
        {
          t = make_token(T_ISPLUS, ":+", 2);
          LConsume(2);
        }
      else
        {
          t = make_token(T_COLON, ":", 1);
          LConsume(1);
        }
    }
  else if(c == '<' && LLA(1) == '!')
    {
      t = make_token(T_LDIFF, "<!", 2);
      LConsume(2);
    }
  else if(c == '!' && LLA(1) == '>')
    {
      t = make_token(T_RDIFF, "!>", 2);
      LConsume(2);
    }
  else if(c == '%' && curr_col() == 1)
    { /* LKB inflr line */
      char *start;
      int i;

      start = LMark();
      i = 1;

      while(LLA(i) != '\n' && LLA(i) != EOF) i++;
      
      t = make_token(T_INFLR, start + 1, i - 1);

      LConsume(i);
    }
  else
    {
      enum TOKEN_TAG tag;
      char txt[2] = "x";

      switch(c)
        {
        case '.':
          tag = T_DOT;
          break;
        case ',':
          tag = T_COMMA;
          break;
        case '=':
          tag = T_EQUALS;
          break;
        case '#':
          tag = T_HASH;
          break;
        case '\'':
          tag = T_QUOTE;
          break;
        case '&':
          tag = T_AMPERSAND;
          break;
        case '@':
          tag = T_AT;
          break;
        case '^':
          tag = T_CAP;
          break;
        case '$':
          tag = T_DOLLAR;
          break;
        case '(':
          tag = T_LPAREN;
          break;
        case ')':
          tag = T_RPAREN;
          break;
        case '[':
          tag = T_LBRACKET;
          break;
        case ']':
          tag = T_RBRACKET;
          break;
        case '{':
          tag = T_LBRACE;
          break;
        case '}':
          tag = T_RBRACE;
          break;
        case '<':
          tag = T_LANGLE;
          break;
        case '>':
          tag = T_RANGLE;
          break;
        default:
          { char str[3] = { (char) c, '\'', '\0' };
            throw tError("unexpected character '" + std::string(str),
                         curr_fname(), curr_line(), curr_col());
          }
        }
      txt[0] = (char) c;
      t = make_token(tag, txt, 1);
      LConsume(1);
    }
  
  return t;
}

void print(std::ostream &out, const lex_token &t)
{
  if(t.tag == T_EOF)
    {
      out << "*EOF*" << std::endl;
    }
  else
    {
      out << "[" << t.tag << "]<" << t.text << ">" << std::endl;
    }
}

inline std::ostream &operator<<(std::ostream &o, const lex_token &tok) {
  print(o, tok); return o;
}

int tokensdelivered = 0;

struct lex_token *
get_token()
{
    struct lex_token *t;
    int hope = 1;
    
    while(hope)
    {
        while((t = get_next_token())->tag != T_EOF)
        {
            if(t->tag != T_WS && t->tag != T_COMM)
            {
#ifdef PETDEBUG
              LOG(logSyntax, DEBUG, "delivering " << t) ;
#endif
                tokensdelivered++;
                return t;
            }
#ifdef PETDEBUG
            else
            {
              LOG(logSyntax, DEBUG, "not delivering " << t);
            }
#endif
            free(t);
        }
        if(!pop_file()) hope = 0;
    }

#ifdef PETDEBUG
  LOG(logSyntax, DEBUG, "delivering " << t);
#endif
    
    tokensdelivered++;
    return t;
}

/* the parser operates on a stream of tokens. these are provided by get_token().
   to make lookahead transparent, parser accesses token via functions LA and consume */

#define MAX_LA 2 /* we (don't) need a LL(2) parser for TDL */

struct lex_token *LA_BUF[MAX_LA+1] = {NULL, NULL, NULL};

int allow_redefinitions = 0;
int lexicon_mode = 0;
int semrels_mode = 0;

struct lex_token *
LA(int n)
/* works for 0 <= i <= MAX_LA */
{
    int i;
    assert(LA >= 0); assert(n <= MAX_LA);
    
    if(LA_BUF[n] != NULL)
    {
        return LA_BUF[n];
    }
    
    /* we have to fill buffer */
    
    for(i = 0; i <= n; i++)
    {
        if(LA_BUF[i] == NULL)
        {
            LA_BUF[i] = get_token();
        }
    }
  
    return LA_BUF[n];
}

void consume1()
{
  int i;

  if(LA_BUF[0] == NULL)
    {
      LOG(logSyntax, WARN, "consuming tokens not looked at yet");
      get_token();
    }
  else
    {
      if(LA_BUF[0]->text != NULL) free(LA_BUF[0]->text);
      if(LA_BUF[0]->loc != NULL) free(LA_BUF[0]->loc);
      free(LA_BUF[0]);
    }

  for(i = 0; i < MAX_LA; i++)
    {
      LA_BUF[i] = LA_BUF[i+1];
      LA_BUF[i+1] = NULL;
    }
}

void consume(int n)
/* consumes specified number of tokens */
{
  int i;
  assert(n > 0);

  for(i = 0; i < n; i++) consume1();
}

int syntax_errors = 0;

void syntax_error(const char *msg, struct lex_token *t)
{
  syntax_errors ++;
  if(t->tag == T_EOF)
    {
      LOG(logSyntax, ERROR, "error: syntax error at end of input: " << msg);
    }
  else if(t->loc == NULL)
    {
      LOG(logSyntax, ERROR, "error: syntax error - got `" << std::setw(20)
          << t->text << "', " << msg);
    }
  else
    {
      LOG(logSyntax, ERROR, t->loc->fname << ":" << t->loc->linenr << ":"
          << t->loc->colnr << ": error: (syntax) - got `" << std::setw(20)
          << t->text << "', " << msg);
    }
}

void recover(enum TOKEN_TAG tag)
{
  static struct lex_token *last_t;
  
  while(LA(0)->tag != tag && LA(0)->tag != T_EOF) consume(1);

  if(LA(0) ->tag == T_EOF)
    {
      throw tError("confused by previous errors, bailing out...\n");
    }
  
  if(LA(0) == last_t) consume(1);

  last_t = LA(0);
}

char *match(enum TOKEN_TAG tag, const char *s, bool readonly)
{
  char *text = NULL;
  
  if(tag == T_ID && LA(0)->tag == T_KEYWORD)
    LA(0)->tag = T_ID;

  if(LA(0)->tag != tag)
    {
      char msg[80];
      sprintf(msg, "expecting %s", s);
      syntax_error(msg, LA(0));
      // assume it was forgotten, continue
    }
  else
    {
      if(!readonly)
        {
          text = LA(0)->text;
          LA(0)->text = NULL;
        }

      consume(1);
    }

  return text;
}

bool consume_if(enum TOKEN_TAG tag)
{
  if(LA(0)->tag == tag)
    {
      consume(1);
      return true;
    }
  else
    return false;
}

int is_keyword(struct lex_token *t, const char *kwd)
{
  if(t->tag == T_KEYWORD && strcmp(t->text, kwd) == 0)
    return 1;
  else
    return 0;
}

void match_keyword(const char *kwd)
{
  if(!is_keyword(LA(0), kwd))
    {
      char msg[80];
      sprintf(msg, "expecting `%s'", kwd);
      syntax_error(msg, LA(0));
      // assume it was forgotten, continue
    }
  else
    consume(1);
}
