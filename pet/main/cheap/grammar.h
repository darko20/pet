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

/* grammar rules, grammar*/

#ifndef _GRAMMAR_H_
#define _GRAMMAR_H_

#include "list-int.h"
#include "../common/utility.h"
#include "types.h"
#include "fs.h"
#include "sm.h"
#include "lexicon.h"

// global variables for quick check
extern int qc_len_unif;
extern qc_node *qc_paths_unif;

extern int qc_len_subs;
extern qc_node *qc_paths_subs;

enum rule_trait { SYNTAX_TRAIT, LEX_TRAIT, INFL_TRAIT };

class grammar_rule
{
 public:
  grammar_rule(type_t t);

  inline int arity() { return _arity; }
  inline int nextarg() { return first(_tofill); }
  inline bool left_extending() { return first(_tofill) == 1; }

  inline int type() { return _type; }
  inline char *printname() { return printnames[_type]; }
  inline int id() { return _id; }
  inline rule_trait trait() { return _trait; }
  inline void trait(rule_trait t) { _trait = t; }

  void print(FILE *f);

  fs instantiate(bool full = false);

  inline fs nextarg(fs &f) { return f.nth_arg(first(_tofill)); }
  inline list_int *restargs() { return rest(_tofill); }
  inline list_int *allargs() { return _tofill; }

  inline type_t *qc_vector_unif(int arg) { return _qc_vector_unif[arg - 1]; }

  inline bool hyperactive() { return _hyper; }
  inline bool spanningonly() { return _spanningonly; }

  int actives, passives;

 private:
  static int next_id;

  int _id;
  int _type;        // type index
  rule_trait _trait;
  int _arity;
  list_int *_tofill;
  
  fs _f_restriced;  // The feature structure corresponding to this rule
                    // with the packing restrictor applied.
  
  type_t **_qc_vector_unif;
  void init_qc_vector_unif();

  bool _hyper;
  bool _spanningonly;

  friend class tGrammar;
};

class tGrammar
{
 public:
  tGrammar(const char *filename);
  ~tGrammar();

  // Look up a grammar property.
  string property(string key);
  
  inline map<string, string> &properties()
    { return _properties; }

  bool root(fs &, type_t &rule);

  list_int *deleted_daughters() { return _deleted_daughters; }
  list_int *packing_restrictor() { return _packing_restrictor; }

  inline bool filter_compatible(grammar_rule *mother, int arg,
                                grammar_rule *daughter)
  {
    if(daughter == NULL) return true;
    return _filter[daughter->id() + _nrules * mother->id()] &
      (1 << (arg - 1));
  }

  inline void subsumption_filter_compatible(grammar_rule *a, grammar_rule *b,
                                            bool &forward, bool &backward)
  {
      if(a == 0 || b == 0)
      {
          forward = backward = true;
          return;
      }
      forward = _subsumption_filter[a->id() + _nrules * b->id()];
      backward = _subsumption_filter[b->id() + _nrules * a->id()];
  }

  inline int nrules() { return _rules.size(); }
  int nhyperrules();

  inline int nstems() { return _lexicon.size(); }
  lex_stem *find_stem(int inst_key);
  list<lex_stem *> lookup_stem(string s);

#ifdef EXTDICT
  extDictionary *extDict() { return _extDict; }
  void clear_dynamic_stems();
#endif
  
  list<full_form> lookup_form(const string form);

  list_int *generics() { return _generics; }

  bool punctuationp(const string &s);

#ifdef ONLINEMORPH
  class tMorphAnalyzer *morph() { return _morph; }
#endif
  
  inline tSM *sm() { return _sm; }

 private:
  map<string, string> _properties;

#ifndef ICU
  string _punctuation_characters;
#else
  UnicodeString _punctuation_characters;
#endif

  map<type_t, lex_stem *> _lexicon;
  multimap<string, lex_stem *> _stemlexicon;

#ifdef EXTDICT
  extDictionary *_extDict;
  list<lex_stem *> _dynamicstems;
  int _extdict_discount;
#endif

  typedef multimap<string, full_form *> ffdict;
  ffdict _fullforms;

#ifdef ONLINEMORPH
  class tMorphAnalyzer *_morph;
#endif

  int _nrules;
  list<grammar_rule *> _rules;

  list_int *_root_insts;

  list_int *_generics;
  
  char *_filter;
  char *_subsumption_filter;
  void initialize_filter();

  void init_rule_qc_unif();
  int _qc_inst_unif;
  int _qc_inst_subs;

  list_int *_deleted_daughters;
  list_int *_packing_restrictor;

  /// Stochastic model.
  class tSM *_sm;

  void undump_properties(dumper *f);
  void init_parameters();

  // friend class le_iter;
  friend class rule_iter;
};

class rule_iter
{
 public:
  rule_iter(tGrammar *G)
    {
      _G = G; _curr = _G->_rules.begin();
    }

  inline bool valid()
    {
      return _curr != _G->_rules.end();
    }

  inline grammar_rule *current()
    {
      if(valid()) return *_curr; else return 0;
    }

  rule_iter &operator++(int)
    {
      ++_curr; return *this;
    }

 private:
  list<grammar_rule *>::iterator _curr;
  tGrammar *_G;
};

#endif
