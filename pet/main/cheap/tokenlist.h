/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* tokenized input */

#ifndef _TOKENLIST_H_
#define _TOKENLIST_H_

#include <stdio.h>
#include <vector>
#include "cheap.h"
#include "postags.h"
#include "errors.h"

// tokenlist is an abstract base class

class tokenlist
{
 protected:

  vector<string> _tokens;
  int _ntokens;
  int _offset;
  static char *stop_characters;

 public:

  tokenlist() { 
    _ntokens = 0;
    _offset = 0;
    stop_characters = cheap_settings->value("stop-characters");
    if(stop_characters == NULL) stop_characters = "\t?!.:;,()-+*$\n";
  }
  tokenlist(const tokenlist &tl);
  virtual ~tokenlist();

  tokenlist &operator=(const tokenlist &tl);

  int length() { return _ntokens; }
  int offset() { return _offset; }
  int size() { return _ntokens - _offset; }

  void print(FILE *f);

  string operator[](int i)
    {
      if(i >= 0 && i < _ntokens)
	return _tokens[i];
      else
	throw error("index out of bounds for tokenlist");
    }

  bool punctuatiop(int i);

  void skew(int n) { _offset = n; }

  // returns a set of POS tags for token at this position
  virtual postags POS(int i) = 0;

  virtual string description() = 0;
};

class lingo_tokenlist : public tokenlist
{
 public:
  lingo_tokenlist(const char *s);

  virtual postags POS(int i) { return postags(); }

  virtual string description() { return "LinGO tokenization"; }
};

#endif

  //
  // _fix_me_
  // i guess we will need to rework the input to the parser to accomodate
  // YY-style formats: the YY preprocessor may (ultimately, at least :-)
  // hand us something like a word graph, potentially enriched with (POS)
  // category information.  once categories become available, we want to use
  // these in the retrieval of default lexical entries.  also, we envisage
  // that the input from YY contains multi-word lexemes (e.g. `bank account')
  // such that the matching (shaping) of lexical items retrieved against our
  // input will need improvement.                          (7-apr-00  -  oe)
  //
