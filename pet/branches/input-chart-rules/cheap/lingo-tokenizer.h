/* -*- Mode: C++ -*- */
/** \file lingo-tokenizer.h
 * Simple tokenizer, mainly for English.
 */

#ifndef _LINGO_TOKENIZER_H
#define _LINGO_TOKENIZER_H

#include "input-modules.h"

#include <string>
#include <list>

/** Simple tokenizer, mainly for English.
 *
 *  Removes punctuation, translates german iso umlauts depending on the setting
 *  of \c translate-iso-chars and handles single apostrophes in words by
 *  splitting this word into two.
 */
class tLingoTokenizer : public tTokenizer {
public:
  tLingoTokenizer() : tTokenizer() {}

  /** Produce a set of tokens from the given string. */
  virtual void tokenize(std::string s, inp_list &result);

  virtual std::string description() { return "LinGO tokenization"; }

  virtual position_map position_mapping() { return STANDOFF_POINTS; }

private:
  std::list<std::string> do_it(std::string s);
};

#endif
