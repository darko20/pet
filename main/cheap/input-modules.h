/* -*- Mode: C++ -*- */
/* \file input-modules.h
 * Base classes for cheap input modules:
 * -- Tokenizer
 * -- Morphology
 * -- POS Tagger
 * -- Named Entity Recognition
 * -- Lexicon
 */

#ifndef _INPUT_MODULES_H
#define _INPUT_MODULES_H

#include "pet-config.h"
#include "item.h"
#ifdef HAVE_ICU
#include "unicode.h"
#endif

#include <list>
#include <string>

typedef std::string myString;

/** The pure virtual base class for preprocessing modules */
class tInputModule {
public:
  virtual ~tInputModule() {}

  /** Reset internal state after processing one sentence */
  virtual void clear() {};

  /** A string to describe the module */
  virtual std::string description() = 0; 
  
  /** All modules which are on the same level are tried in parallel and
   *  all their results are considered. Modules on a lower level are only
   *  considered if no result came from some higher level module. Smaller
   *  number means higher level, the highest level modules should return 1.
   */
  virtual int level() const { return 1; }

};

/** function object to compare two modules according to their priority (level)
 */
struct less_than_module 
  : public std::binary_function<bool, const tInputModule *, const tInputModule *> {
  bool operator()(const tInputModule *a, const tInputModule *b) {
    return a->level() < b->level();
  }
};

enum position_map { NO_POSITION_MAP = 0, STANDOFF_POINTS, STANDOFF_COUNTS};

/** Tokenize the input (not necessarily unambiguous)
 *
 * The token class must be registered, either as morphologizable token, or
 * explicitely as "lex type" token with FS interpretation (i.e. the token name
 * must be a type name). All unknown token classes will be skipped. This might
 * be refined by specifying skip classes and handling the unknown token classes
 * just like unknown lex entries.
 * 
 * Should morphology be called inside tokenisation? If the tokenizer "decides"
 * or "knows" which tokens should be processed morphologically, why not leaving
 * it to the tokenizer "module" to call morphology??
 *  
 * Other way round: Since morphology possibly makes several items out of one
 * item, processing of skip items should precede morphology.
 */
class tTokenizer : public tInputModule {
public:
  virtual ~tTokenizer() { }

  /** Produce a set of tokens from the given string. */
  virtual void tokenize(myString s, inp_list &result) = 0;

  /** Return \c STANDOFF_COUNTS if the position in the returned tokens are counts
   *  , \c STANDOFF_POINTS if they are points
   */
  virtual position_map position_mapping() = 0;

protected:
  tTokenizer();
  
  /** Determine if a string only consists of punctuation characters */
  bool punctuationp(const std::string &s);

  /** \brief Translate the german ISO umlaut and sz characters in \a s by their
   *  isomorphix (ae, ue, ss, etc.) counterparts, if \c _translate_iso_chars
   *  is true, leave it unchanged otherwise.
   */
  void translate_iso(std::string &s);

  /** A string with all characters considered as punctuation.
   * May be set by the global setting \c punctuation-characters.
   */
#ifdef HAVE_ICU
  UnicodeString _punctuation_characters;
#else
  std::string _punctuation_characters;
#endif

  /** If \c true, Translate the german ISO umlaut and sz characters in stem and
   *  surface forms.
   */
  bool _translate_iso_chars;
};

/** Call a Named Entity Recognizer.
 *
 * All NE's have to have a valid class() that * is connected with a lex entry
 * FS, i.e. the mapping onto HPSG types must have * been carried out in the NE
 * recognizer. Morphological information may be provided in the _inflrs_todo.
 */
class tNE_recognizer : public tInputModule {
public:
  virtual ~tNE_recognizer() {}

  /** Add Named Entities to the list of tokens. */
  virtual void compute_ne(myString s, inp_list &tokens_result) = 0;
};

/** Add POS information (destructively) to the tokens in \a tokens_result.
 */
class tPOSTagger : public tInputModule {
public:
  virtual ~tPOSTagger() {}

  /** Add POS tags to the tokens in tokens_result */
  virtual void compute_tags(myString s, inp_list &tokens_result) = 0;
};

/** Take an input token and compute a list of morphological analyses, 
 *  which contain the stem and inflection rules.
 */
class tMorphology : public tInputModule {
public:
  virtual ~tMorphology() {}
  
  /** Compute morphological results for \a form. */
  virtual std::list<class tMorphAnalysis> operator()(const myString &form) = 0;
};

/** Get \c lex_stem entries for a given stem (normalized form).
 *  A \c lex_stem contains information about the HPSG type of the lexicon
 *  entry, the inflection position, arity and possible FS modifications 
 */
class tLexicon : public tInputModule {
public:
  virtual ~tLexicon() {}

  /** Get lexicon entries for \a stem  */
  virtual std::list<lex_stem *> operator()(const myString &stem) = 0;
};

/** \todo fix this declaration */
extern tGrammar *Grammar;

/** Lexicon stored internally in the grammar. */
class tInternalLexicon : public tLexicon {
public:
  virtual ~tInternalLexicon() {}

  /** Get lexicon entries for \c stem in \c result */
  virtual std::list<lex_stem *> operator()(const myString &stem) {
    return Grammar->lookup_stem(stem);
  };

  /** A string to describe the module */
  virtual std::string description() { return "Internal stem lexicon"; }
};

#endif
