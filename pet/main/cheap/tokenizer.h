/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
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
