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

/* tokenized input */

#ifndef _TOKENIZER_H_
#define _TOKENIZER_H_

#include "cheap.h"

// tokenizer is an abstract base class

class tokenizer
{
 protected:
  string _input;

 public:
  tokenizer(string s) :
    _input(s) {} ;
  virtual ~tokenizer() {};

  virtual void add_tokens(class input_chart *i_chart) = 0;
  virtual void print(FILE *f);

  virtual string description() = 0;
};

class lingo_tokenizer : public tokenizer
{
 public:
  lingo_tokenizer(string s) :
    tokenizer(s) {};
  
  virtual void add_tokens(class input_chart *i_chart);

  virtual string description() { return "LinGO tokenization"; }
 private:
  list<string> tokenize();
};

#endif
