/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* implementation of LKB style morphological analysis and generation */

#ifndef _MORPH_H_
#define _MORPH_H_

#include "grammar.h"

class morph_analysis
{
 public:
  morph_analysis() {}

  morph_analysis(list<string> forms, list<type_t> rules,
		 list<lex_stem *> stems)
    : _forms(forms), _rules(rules), _stems(stems) 
    {}

  list<string> &forms() { return _forms; }

  string base() { return _forms.front(); }
  string complex() { return _forms.back(); }

  list<type_t> &rules() { return _rules; }

  list<lex_stem *> &stems() { return _stems; }

  bool valid() { return !_stems.empty(); }
  
  void print(FILE *f);
  void print_lkb(FILE *f);

 private:
  list<string> _forms;
  list<type_t> _rules;
  list<lex_stem *> _stems;
};

class morph_analyzer
{
 public:
  morph_analyzer(class grammar *G);
  ~morph_analyzer();

  void add_global(string rule);
  void add_rule(type_t t, string rule);
  void add_irreg(string stem, type_t t, string form);

  void set_irregular_only(bool b)
    { _irregs_only = b; }

  class morph_letterset *letterset(string name);
  void undo_letterset_bindings();

  list<morph_analysis> analyze(string form);
  string generate(morph_analysis);

  void print(FILE *f);

 private:
  void parse_rule(type_t t, string rule, bool suffix);

  list<morph_analysis> analyze1(morph_analysis form);
  bool matching_irreg_form(morph_analysis a);

  void add_subrule(class morph_subrule *sr) 
    { _subrules.push_back(sr); }

  class grammar *_grammar;

  class morph_lettersets *_lettersets;
  class morph_trie *_suffixrules;
  class morph_trie *_prefixrules;

  list<class morph_subrule *> _subrules;

  bool _irregs_only;

  multimap<string, morph_analysis *> _irregs_by_stem;
  multimap<string, morph_analysis *> _irregs_by_form;

  friend class morph_trie;
};

#endif
