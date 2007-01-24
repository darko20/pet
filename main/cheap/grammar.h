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

/** \file grammar.h 
 * grammar rules, grammar
 */

#ifndef _GRAMMAR_H_
#define _GRAMMAR_H_

#include "pet-config.h"

#include "list-int.h"
#include "utility.h"
#include "types.h"
#include "fs.h"
#include "sm.h"
#include "lexicon.h"

#include <string>
#include <list>
#include <map>

using std::string;
using std::map;
using std::list;

/** @name global variables for quick check */
/*@{*/
/** Number of the unification quick check paths to consider */
extern int qc_len_unif;
/** Compact representation of unification quick check paths as tree of qc_node
    nodes */
extern qc_node *qc_paths_unif;

/** Number of the sumsumption quick check paths to consider */
extern int qc_len_subs;
/** Compact representation of subsumption quick check paths as tree of qc_node
    nodes */
extern qc_node *qc_paths_subs;
/*@}*/

/** The different traits of rules and items:
    -- INPUT_TRAIT an input item, still without feature structure
    -- INFL_TRAIT an incomplete lexical item that needs application of
                  inflection rules to get complete
    -- LEX_TRAIT a lexical item with all inflection rules applied
    -- SYNTAX_TRAIT a phrasal item
 */
enum rule_trait { SYNTAX_TRAIT, LEX_TRAIT, INFL_TRAIT, INPUT_TRAIT };

/** A rule from the HPSG grammar */
class grammar_rule
{
 public:
  /** Constructor for grammar rules. 
   *  \returns If the feature structure of the given type is not a valid rule
   *  (no or empty \c ARGS path), this method returns \c NULL, a grammar rule
   *  for the given type otherwise.
   */
  static grammar_rule* make_grammar_rule(type_t t);

  /** Get the arity (length of right hand side) of the rule */
  inline int arity() { return _arity; }
  /** Get the next arg that should be filled */
  inline int nextarg() const { return first(_tofill); }
  /** Does the rule extend to the left or to the right?
   * \todo Remove the current restriction to binary rules. 
   */
  inline bool left_extending() { return first(_tofill) == 1; }

  /** Get the type of this rule (instance) */
  inline int type() { return _type; }
  /** Get the external name */
  inline const char *printname() { return print_name(_type); }
  /** Get the unique rule id, also used in the rule filter. */
  inline int id() { return _id; }
  /** Get the trait of this rule: INFL_TRAIT, LEX_TRAIT, SYNTAX_TRAIT */
  inline rule_trait trait() { return _trait; }
  /** Set the trait of this rule: INFL_TRAIT, LEX_TRAIT, SYNTAX_TRAIT */
  inline void trait(rule_trait t) { _trait = t; }

  /** Print in readable form for debugging purposes */
  void print(FILE *f);

  /** Dump grammar in a format feasible for LUI (?) into \a directory */
  void lui_dump(const char* directory = "/tmp");

  /** Return the feature structure associated with this rule.
   * If packing is active, the structure is restricted.
   */
  fs instantiate(bool full = false);

  /** Return the feature structure corresponding to the next argument */
  inline fs nextarg(const fs &f) const { return f.nth_arg(first(_tofill)); }

  /** Return all of the arguments but the current one in the order in which
   *  they should be filled.
   */
  inline list_int *restargs() { return rest(_tofill); }

  /** Return all of the arguments in the order in which they should be
   *  filled.
   */ 
  inline list_int *allargs() { return _tofill; }

  /** Return the quick check vector for argument \a arg */
  inline type_t *qc_vector_unif(int arg) { return _qc_vector_unif[arg - 1]; }

  /** Should this rule be treated special when using hyperactive parsing?
   *  Rules whose active items are seldom reused should be made hyperactive
   *  because one dag copying operation is much more expensive than several
   *  unsuccessful unifications. 
   */
  inline bool hyperactive() { return _hyper; }

  /** Return \c true if the items using this rule should always span the whole
   *  chart 
   */
  inline bool spanningonly() { return _spanningonly; }

  /** @name Rule Statistics
   * How many active (incomplete) and passive (complete)
   * edges were produced using this rule.
   */
  /*@{*/
  int actives, passives;
  /*@}*/

 private:
  grammar_rule(type_t t);
  grammar_rule() { }

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

/** Class containing all the infos from the HPSG grammar */
class tGrammar
{
 public:
  /** Initalize this grammar by reading it from the file \a filename */
  tGrammar(const char *filename);
  ~tGrammar();

  /** \brief Look up a grammar property with name \a key. If it does not exist,
   *  the returned string will be empty.
   */
  string property(string key);
  
  /** Return the map containing the grammar properties */
  inline map<string, string> &properties()
    { return _properties; }

  /** Check if \a fs is compatible with one of the root nodes.
   *
   * \return \c true if \a f is compatible with one of the root FS nodes and
   * return the type of this root node in \a rule.
   */
  bool root(fs &f, type_t &rule);

  /** Return the list of attributes that should be deleted in the root dags of
   *  passive items.
   */
  list_int *deleted_daughters() const { return _deleted_daughters; }

  /** Return the packing restrictor for this grammar */
  const class restrictor &packing_restrictor() const {
    return *_packing_restrictor;
  }

  /** Is successful unification of \a daugther as \a arg th argument of \a
   *  mother possible?
   * This is a precomputed table containing all the possible rule combinations,
   * which can be rapidly looked up to avoid failing unifications.
   */
  inline bool filter_compatible(grammar_rule *mother, int arg,
                                grammar_rule *daughter)
  {
    if(daughter == NULL) return true;
    return _filter[daughter->id() + _nrules * mother->id()] &
      (1 << (arg - 1));
  }

  /** Is successful unification of \a daugther as \a arg th argument of \a
   *  mother possible? 
   * This is a precomputed table containing all the possible rule combinations,
   * which can be rapidly looked up to avoid failing unifications.
   * This version is used in the morphological analyzer.
   */
  inline bool filter_compatible(type_t mother, int arg,
                                type_t daughter)
  {
    grammar_rule *mother_r = _rule_dict[mother];
    grammar_rule *daughter_r = _rule_dict[daughter];
    if(mother_r == 0 || daughter_r == 0)
      throw tError("Unknown rule passed to filter_compatible");
    return filter_compatible(mother_r, arg, daughter_r);
  }

  /** Is rule \a a more general than \a b or vice versa?
   * If \a a or \a b are NULL, \a forward and \a backward are set to \c true,
   * otherwise, both are looked up in a precomputed table. This is a similar
   * approach to the unification rule filter.
   */
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

  /** Return the number of rules in this grammar */
  inline int nrules() { return _rules.size(); }
  /** Return the number of hyperactive rules in this grammar */
  int nhyperrules();
  
  /** Return the number of stem entries in the grammar */
  inline int nstems() { return _lexicon.size(); }
  /** return a pointer to the lex_stem with type id \a inst_key, or NULL if it
   *  does not exist.
   */
  lex_stem *find_stem(type_t inst_key);
  /** return all lex_stem pointers for the base form \a s. */
  list<lex_stem *> lookup_stem(string s);

#ifdef EXTDICT
  extDictionary *extDict() { return _extDict; }
  void clear_dynamic_stems();
#endif
  
  // _fix_me_ becomes obsolete when yy.cpp does
  //list<full_form> lookup_form(const string form);

  /** Return the list of generic lexicon entries in this grammar */
  list_int *generics() { return _generics; }

  /** \todo becomes obsolete when yy.cpp does */
  bool punctuationp(const string &s);

#if 0
  // obsolete
#ifdef ONLINEMORPH
  class tMorphAnalyzer *morph() { return _morph; }
#endif
#endif

  /** Return the statistic maxent model of this grammar */
  inline tSM *sm() { return _sm; }

  /** deactivate all rules */
  void deactivate_all_rules() {
    _rules.clear();
  }
  
  /** activate all (and only) inflection rules */
  void activate_infl_rules() {
    deactivate_all_rules();
    _rules = _infl_rules; 
  }

  /** activate all (and only) lexical and inflection rules */
  void activate_lex_rules() {
    activate_infl_rules();
    _rules.insert(_rules.end(), _lex_rules.begin(), _lex_rules.end());
  }

  /** activate syntactic rules only */
  void activate_syn_rules() {
    deactivate_all_rules();
    _rules = _syn_rules; 
  }

  /** activate all available rules */
  void activate_all_rules() {
    activate_lex_rules();
    _rules.insert(_rules.end(), _syn_rules.begin(), _syn_rules.end());
  }

  /** Return the grammar_rule pointer for type \a type or NULL, if not
   *  available.
   */
  grammar_rule * find_rule(type_t type) {
    map<type_t, grammar_rule *>::iterator it = _rule_dict.find(type);
    if(it != _rule_dict.end()) {
      return it->second;
    } else {
      return NULL;
    }
  }

 private:
  map<string, string> _properties;

#if 0
#ifndef HAVE_ICU
  string _punctuation_characters;
#else
  UnicodeString _punctuation_characters;
#endif
#endif

  map<type_t, lex_stem *> _lexicon;
  std::multimap<std::string, lex_stem *> _stemlexicon;

#ifdef EXTDICT
  extDictionary *_extDict;
  list<lex_stem *> _dynamicstems;
  int _extdict_discount;
#endif

#if 0
  typedef multimap<string, full_form *> ffdict;
  ffdict _fullforms;
#endif

#ifdef ONLINEMORPH
  class tMorphAnalyzer *_morph;
#endif

  /** The number of all rules (syntactic, lex, and infl) for rule filtering */
  int _nrules;
  /** The list of currently active rules.
   * If the parser is used to complete lexical processing first, only infl and
   * lex rules will be active in the first phase. Only syn rules will then be
   * active in the second phase.
   */
  list<grammar_rule *> _rules;

  /** The set of syntactic rules */
  list<grammar_rule *> _syn_rules;
  /** The set of lexical rules */
  list<grammar_rule *> _lex_rules;
  /** The set of inflectional rules */
  list<grammar_rule *> _infl_rules;
  /** Map the rule type back to the rule structure */
  map<type_t, grammar_rule *> _rule_dict;

  list_int *_root_insts;

  /** Generic lexicon entries, selected via POS tags in case no appropriate
   *  lexicon entry is found
   */
  list_int *_generics;
  
  char *_filter;
  char *_subsumption_filter;
  void initialize_filter();

  void init_rule_qc_unif();
  int _qc_inst_unif;
  int _qc_inst_subs;

  list_int *_deleted_daughters;
  class restrictor *_packing_restrictor;

  /// Stochastic model.
  class tSM *_sm;

  void undump_properties(dumper *f);
  void init_parameters();

  // friend class le_iter;
  friend class rule_iter;
};

/** An iterator to map through all rules of a grammar */
class rule_iter
{
 public:
  /** Initialize the iterator to the first rule of the grammar \a G */
  rule_iter(tGrammar *G)
    {
      _G = G; _curr = _G->_rules.begin();
    }

  /** Are there still rules? */
  inline bool valid()
    {
      return _curr != _G->_rules.end();
    }

  /** Return the pointer to the current rule */
  inline grammar_rule *current()
    {
      if(valid()) return *_curr; else return 0;
    }

  /** Go to next rule */
  rule_iter &operator++(int)
    {
      ++_curr; return *this;
    }

 private:
  list<grammar_rule *>::iterator _curr;
  tGrammar *_G;
};

#endif
