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

/* grammar rules, grammar, stems and lexicon */

#ifndef _GRAMMAR_H_
#define _GRAMMAR_H_

#include "list-int.h"
#include "../common/utility.h"
#include "types.h"
#include "fs.h"
#include "sm.h"
#ifdef EXTDICT
#include "extdict.h"
#endif

// global variables for quick check
extern int qc_len;
extern qc_node *qc_paths;

class lex_stem
{
 public:

  lex_stem(type_t t, const modlist &mods = modlist(), const list<string> &orths = list<string>());
  lex_stem(const lex_stem &le);
  ~lex_stem();

  lex_stem & operator=(const lex_stem &le);

  fs instantiate();

  inline char *name() const { return typenames[_type]; }
  inline char *printname() const { return printnames[_type]; }

  inline int type() const { return _type; }
  inline int length() const { return _nwords; }
  inline int inflpos() const { return _nwords - 1; }

  inline const char *orth(int i) const { return _orth[i]; }
  
  void print(FILE *f) const;

 private:
  static int next_id;

  int _id;

  int _type;        // type index

  modlist _mods;

  int _nwords;      // length of _orth
  char **_orth;     // array of _nwords strings

  vector<string> get_stems();

  friend class grammar;
};

class full_form
{
 public:
  full_form() :
    _stem(0), _affixes(0), _offset(0) {};

  full_form(lex_stem *st, modlist mods = modlist(), list_int *affixes = 0)
    : _stem(st), _affixes(copy_list(affixes)), _offset(0), _mods(mods)
    { if(st) _offset = st->inflpos(); }

  full_form(dumper *f, class grammar *G);

#ifdef ONLINEMORPH
  full_form(lex_stem *st, class morph_analysis a);
#endif

  full_form(const full_form &that)
    : _stem(that._stem), _affixes(copy_list(that._affixes)),
    _offset(that._offset), _form(that._form), _mods(that._mods) {}

  full_form &operator=(const full_form &that)
  {
    _stem = that._stem;
    free_list(_affixes);
    _affixes = copy_list(that._affixes);
    _offset = that._offset;
    _form = that._form;
    _mods = that._mods;
    return *this;
  }

  ~full_form()
  {
    free_list(_affixes);
  }

  bool valid() const { return _stem != 0; }

  bool operator==(const full_form &f) const
  {
    return (compare(_affixes, f._affixes) == 0) && _stem == f._stem;
    // it's ok to compare pointers here, as stems are not copied
  }

  bool operator<(const full_form &f) const
  {
    if(_stem == f._stem)
      return compare(_affixes, f._affixes) == -1;
    else
      return _stem < f._stem;
      // it's ok to compare pointers here, as stems are not copied
  }

  fs instantiate();

  inline const lex_stem *stem() const { return _stem; }

  inline int offset() { return _offset; }

  inline int length()
  {
    if(valid()) return _stem->length(); else return 0;
  }

  inline string orth(int i)
  {
    if(i >= 0 && i < length())
      {
	if(i != offset()) return _stem->orth(i); else return _form;
      }
    else
      return 0;
  }
  
  inline string key()
  {
    return orth(offset());
  }

  list_int *affixes() { return _affixes; }

  const char *stemprintname()
  {
    if(valid()) return _stem->printname(); else return 0;
  }

  string affixprintname();

  void print(FILE *f);

  string description();

 private:
  lex_stem *_stem;
  list_int* _affixes;
  int _offset;

  string _form;
  modlist _mods;

  friend class grammar;
};

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

  inline type_t *qc_vector(int arg) { return _qc_vector[arg - 1]; }

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
  
  type_t **_qc_vector;
  void init_qc_vector();

  bool _hyper;
  bool _spanningonly;

  friend class grammar;
};

class grammar
{
 public:
  grammar(const char *filename);
  ~grammar();

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

  inline bool filter_compatible(type_t mother, int arg,
                                type_t daughter)
  {
    grammar_rule *mother_r = _rule_dict[mother];
    grammar_rule *daughter_r = _rule_dict[daughter];
    if(mother_r == 0 || daughter_r == 0)
      throw error("Unknown rule passed to filter_compatible");
    return filter_compatible(mother_r, arg, daughter_r);
  }

  inline int nrules() { return _rules.size(); }
  int nhyperrules();

  inline int nstems() { return _lexicon.size(); }
  lex_stem *lookup_stem(int inst_key);
  list<lex_stem *> lookup_stem(string s);

#ifdef EXTDICT
  extDictionary *extDict() { return _extDict; }
  void clear_dynamic_stems();
#endif
  
  list<full_form> lookup_form(const string form);

  list_int *generics() { return _generics; }

  bool punctuationp(const string &s);

#ifdef ONLINEMORPH
  class morph_analyzer *morph() { return _morph; }
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
  class morph_analyzer *_morph;
#endif

  int _nrules;
  list<grammar_rule *> _rules;
  map<type_t, grammar_rule *> _rule_dict;

  list_int *_root_insts;

  list_int *_generics;
  
  char *_filter;
  void initialize_filter();

  void init_rule_qc();
  int _qc_inst;

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
  rule_iter(grammar *G)
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
  grammar *_G;
};

#endif
