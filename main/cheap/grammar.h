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
#include "types.h"
#include "fs.h"
#include "sm.h"
#include "lexicon.h"
#include "item.h"

// global variables for quick check
extern int qc_len_unif;
extern qc_node *qc_paths_unif;

extern int qc_len_subs;
extern qc_node *qc_paths_subs;

class tGrammarRule
{
 public:
    tGrammarRule(type_t t);

    int
    id()
    {
        return _id;
    }

    type_t
    type()
    {
        return _type;
    }

    tItemTrait
    trait()
    {
        return _trait;
    }

    void
    setTrait(tItemTrait trait)
    {
        _trait = trait;
    }

    int
    arity()
    {
        return length(_toFill);
    }
    
    bool
    hyperActive()
    {
        return _hyperActive;
    }

    bool
    spanningOnly()
    {
        return _spanningOnly;
    }

    const string
    printName()
    {
        return string(printnames[type()]);
    }

    void
    print(FILE *f);

    tPhrasalItem *
    instantiate();

    int actives, passives;

 private:
    static int nextId;
    
    int _id;
    type_t _type;        // type index
    tItemTrait _trait;

    list_int *_toFill;
  
    bool _hyperActive;
    bool _spanningOnly;
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

  inline bool filter_compatible(tGrammarRule *mother, int arg,
                                tGrammarRule *daughter)
  {
    if(daughter == NULL) return true;
    return _filter[daughter->id() + _nrules * mother->id()] &
      (1 << (arg - 1));
  }

  inline void subsumption_filter_compatible(tGrammarRule *a, tGrammarRule *b,
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

  int
  nRules()
  {
      return _rules.size();
  }
  
  int
  nHyperActiveRules();

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
  list<tGrammarRule *> _rules;

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

  inline tGrammarRule *current()
    {
      if(valid()) return *_curr; else return 0;
    }

  rule_iter &operator++(int)
    {
      ++_curr; return *this;
    }

 private:
  list<tGrammarRule *>::iterator _curr;
  tGrammar *_G;
};

#endif
