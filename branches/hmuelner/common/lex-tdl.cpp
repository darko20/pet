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
#include <string>
#include <iomanip>

using std::string;

TDL_MODE tdl_mode = STANDARD_TDL;

const char *keywords[N_KEYWORDS] = { "declare", "domain", "instance", "lisp",
"template", "type", "begin", "defdomain", "deldomain", "delete-package-p",
"end", "end!", "errorp", "expand-all-instances", "include", "leval",
"sorts", "status" };

const char *lexer_idchars = "_+-*?";

int is_idchar(int c)
{
  return isalnum(c&0xFF) || strchr(lexer_idchars, c) || c > 127 || c < 0;
}

int lisp_mode = 0; // shall lexer recognize lisp expressions 

void print_token(std::ostream &out, struct lex_token *t);

lex_token *make_token(enum TOKEN_TAG tag, const char *s, int len,
                      int rlen = -1, bool regex = false)
{
  lex_token *t = new lex_token();;

  if(tag != T_EOF)
  {
    t->loc.assign(curr_fname(), curr_line(), curr_col());
  }
  else
  {
    t->loc.assign("unknown", 0, 0);
  }

  t->tag = tag;

  if(s == NULL)
  {
    t->text.clear();
  }
  else
  {
    if (rlen == -1)
      rlen = len;

    if (rlen == len) {
      t->text = string(s, len);
    } else // s contains a string with escaped characters
    {
      for (int i = 0; i < len; i++) {
        if (s[i] == '\\') {
          i++; // skip, copy following character
        }
        t->text += s[i];
      }
      assert(t->text.size() == rlen);
    }
  }
  return t;
}

void check_id(struct lex_token *t)
{
  int i;

  for(i = 0; i< N_KEYWORDS; i++)
  {
    if(t->text == keywords[i])
    {
      t->tag = T_KEYWORD;
      break;
    }
  }
}

lex_token *get_next_token()
{
  lex_token *t = 0;
  int c = LLA(0);

  if(c == EOF) 
  {
    t = make_token(T_EOF, NULL, 0);
  }
  else if(isspace(c &0xFF))
  { // whitespace
    int i = 1;
    while(isspace(LLA(i) & 0xFF)) i++;

    t = make_token(T_WS, NULL, i);
    LConsume(i);
  }
  else if(c == ';')
  { // single line comment
    const char* start = LMark();
    int i = 1;
    while(LLA(i) != '\n' && LLA(i) != EOF) i++;
    t = make_token(T_COMM, start, i);

#ifdef FLOP
    if(t->text[1] == '%')
    {
      if(strncmp(t->text.c_str()+2, "+redefine", 9) == 0)
        allow_redefinitions = 1;
      else if(strncmp(t->text.c_str(), "-redefine", 9) == 0)
        allow_redefinitions = 0;
    }
#endif

    LConsume(i);
  }
  else if(c == '#' && LLA(1) == '|')
  { /* block comment */
    const char* start = LMark();
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
    int i = 1;
    int l = 0; // length of the resolved string (without escape backslashes)
    const char* start = LMark();
    while(LLA(i) != EOF && LLA(i) != '"') {
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
  else if((c == '^') && (tdl_mode == STANDARD_TDL))
    { // regular expression
      const char *start = LMark();
      int i = 1;

      while(LLA(i) != EOF && LLA(i) != '$')
        {
          if (LLA(i) == '\\') // skip escaped character
            i++;
          i++;
        }

      if(LLA(i) == EOF)
        {
          throw tError("runaway regular expression", curr_fname(),
                      curr_line(), curr_col());
        }

      i += 1; // for '$'
      t = make_token(T_STRING, start, i);
      LConsume(i);
    }
  else if(c == '(' && lisp_mode)
  { // LISP expression
    const char* start = LMark();
    int i = 1;
    int parlevel = 1;
    while(parlevel > 0 && LLA(i) != EOF) {
      if(LLA(i) == '(') parlevel++;
      else if(LLA(i) == ')') parlevel--;
      i++;
    }

    if(LLA(i) == EOF) {
      // runaway LISP expression
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
  else if(is_idchar(c)) // "_+-*?" or isalnum
  { /* identifier/number (or command etc.) */

    /** \todo
    * This isn't particularly elegant or robust. The error
    * handling is not good. There's no clear way to integrate
    * general numbers seamlessly into TDL...
    */

    int i = 0;
    const char* start = LMark();
    while(is_idchar(LLA(i)) || LLA(i) == '!') i++;
    /// \todo check if it is necessary to check for floating point numbers (are those allowed in TDL?)
    if (i == 0) {
      string str = string(1, c) + "'";
      throw tError("unexpected character '" + str,
        curr_fname(), curr_line(), curr_col());
    }

    t = make_token(T_ID, start, i);
    LConsume(i);

    check_id(t);
  }
  else if(c == ':')
  {
    //
    // in november 2008, following an email discussion on the `developers' 
    // list, we decided to treat `:<' (originally a sub-type definition
    // without local constraints) as a syntactic variant of `:='.  i apply
    // the corresponding change to the LKB today.            (29-nov-08; oe)
    //
    if(LLA(1) == '=' || LLA(1) == '<')
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
    const char* start = LMark();
    int i = 1;
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
          assert(tdl_mode == SM_TDL);
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
    delete LA_BUF[0];
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
  else
  {
    LOG(logSyntax, ERROR, t->loc.fname << ":" << t->loc.linenr << ":"
      << t->loc.colnr << ": error: (syntax) - got `" << std::setw(20)
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

string match(enum TOKEN_TAG tag, const char *s, bool readonly)
{
  string text;

  if(tag == T_ID && LA(0)->tag == T_KEYWORD)
    LA(0)->tag = T_ID;

  if(LA(0)->tag != tag)
  {
    string msg = string("expecting ") + s;
    syntax_error(msg.c_str(), LA(0));
    // assume it was forgotten, continue
  }
  else
  {
    if(!readonly)
    {
      text = LA(0)->text;
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
  if(t->tag == T_KEYWORD && t->text == kwd)
    return 1;
  else
    return 0;
}

void match_keyword(const char *kwd)
{
  if(!is_keyword(LA(0), kwd))
  {
    string msg = string("expecting `") + kwd + "'";
    syntax_error(msg.c_str(), LA(0));
    // assume it was forgotten, continue
  }
  else
    consume(1);
}
