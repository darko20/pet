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

// YY input format and server mode (oe, uc)

#ifndef _YY_H_
#define _YY_H_

#include <stdio.h>
#include <assert.h>

#if 0
#include "tokenizer.h"

class yy_tokenizer : public tokenizer
{
 public:
  yy_tokenizer(string s) :
    tokenizer(s), _yyinput(s), _yypos(0) {};

  virtual ~yy_tokenizer() {};

  virtual void add_tokens(class input_chart *i_chart);

  virtual string description() { return "YY Inc tokenization"; }

 private:
  string _yyinput;
  string::size_type _yypos;

  bool eos();
  char cur();
  const char *res();
  bool adv(int n = 1);

  bool read_ws();
  bool read_special(char);

  bool read_int(int &);
  bool read_double(double &);
  bool read_string(string &, bool, bool = false);

  bool read_pos(string &, double &);
  class yy_token *read_token();
};
#endif

extern int cheap_server_initialize(int);
extern void cheap_server(int);
extern int cheap_server_child(int);

extern int yy_tsdb_summarize_item(class chart &, const char *,
				  int, int, const char *);
extern int yy_tsdb_summarize_error(const char *, int, tError &);
extern int socket_write(int, char *);
extern int socket_readline(int, char *, int);

#endif
