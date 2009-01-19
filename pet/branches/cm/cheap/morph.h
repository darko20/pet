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

/** \file morph.h
 * Implementation of LKB style morphological analysis and generation.
 */

#ifndef _MORPH_H_
#define _MORPH_H_

#include "hashing.h"
#include "types.h"
#include "input-modules.h"
#include "morph-inner.h"

#include <string>
#include <list>
#include <map>
#include <vector>

/** An object representing the result of a morpological analysis extending over
 *  multiple stages.
 */
class tMorphAnalysis
{
 public:
  tMorphAnalysis() {}

  /** Build a tMorphAnalysis from available data */
  tMorphAnalysis(std::list<std::string> forms, std::list<grammar_rule *> rules)
    : _forms(forms), _rules(rules)
    {}

  /** A list of strings, from the most analyzed to the surface form */
  std::list<std::string> &forms() { return _forms; }

  /** The base form of the string, without inflection */
  std::string base() const { return _forms.front(); }
  /** The surface form (the input of the analysis) */
  std::string complex() const { return _forms.back(); }

  /** Return the inflection rules that have to be applied to perform this
   *  morphological analysis.
   */
  const std::list<grammar_rule *> &rules() { return _rules; }

  /** Print readably for debugging purposes */
  void print(FILE *f) const;
  /** Print in LKB format */
  void print_lkb(FILE *f);

  /** Two analyses are equal if the base form and the inflection derivation are
   *  equal
   */
  friend bool operator==(const tMorphAnalysis &a, const tMorphAnalysis &b);

  /** Destructively erase duplicate entries from a list of tMorphAnalyses */
  friend void erase_duplicates(std::list<tMorphAnalysis> &li) ;

 private:
  /** Two analyses are equal if the base form and the inflection derivation are
   *  equal
   */
  struct equal :
    public std::binary_function<bool, tMorphAnalysis, tMorphAnalysis> {
    bool operator()(tMorphAnalysis &a, tMorphAnalysis &b) {
      return a == b;
    }
  };

  std::list<std::string> _forms;
  rulelist _rules;

  struct less_than 
    : public std::binary_function<bool, tMorphAnalysis, tMorphAnalysis> {
    bool operator()(tMorphAnalysis &a, tMorphAnalysis &b) {
      if (a.base() < b.base()) return true;
      if (a.base() == b.base()) {
        // "lexicographic" ordering on infl-rules
        ruleiter ar, ae, br, be;
        ar = a._rules.begin();
        ae = a._rules.end();
        br = b._rules.begin();
        be = b._rules.end();
        while ((ar != ae) && (br != be) && (*ar == *br)) {
          ar++; 
          br++;
        }
        if (ar == ae)
          // shorter list (alist prefix blist) is smaller
          return !(br == be) ;
        else
          if (br == be)
            // longer list (blist prefix alist) is greater
            return false;
          else
            // elements are not equal, smaller element loses
            return (*ar < *br);
      }
      else 
        // (a.base() > b.base())
        return false;
    }
  };
};

bool operator==(const tMorphAnalysis &a, const tMorphAnalysis &b);

void erase_duplicates(std::list<tMorphAnalysis> &li) ;

/** Morphological analyzer LKB style. Implements transformation rules similar
 *  to regular expressions.
 */
class tMorphAnalyzer
{
 public:
  tMorphAnalyzer();
  ~tMorphAnalyzer();

  /** Add a global morphological rule, i.e., a rule that does not have an HPSG
   *  counterpart.
   * \todo Check the documentation of this function.
   */
  void add_global(std::string rule);
  /** Add a morpological rule with its corresponding HPSG rule, encoded in the
   *  typedag of \a t.
   */
  void add_rule(grammar_rule *, std::string rule);
  /** Add an irregular form entry.
   *
   * These entries map the surface form to the base form directly, additionally
   * specifying the HPSG inflection rule to apply. Actually, they are very
   * similar to morphological rules, in that they are applied in the same way,
   * with the restriction that the remaining string in the analysis must match
   * the complete surface form.
   * \param stem the base form
   * \param t the type id of the HPSG inflection rule.
   * \param form the surface form.
   */
  void add_irreg(std::string stem, type_t t, std::string form);

  /** Return \c true if no rules or irregular forms are in this analyzer. */
  bool empty();  

  /** \brief Use competing irregular analyses only if \a b is \c
   *  true. Otherwise, simply add all irregular analyses without removing
   *  competing regular ones.
   */
  void set_irregular_only(bool b) { _irregs_only = b; }

  /** Return the letterset named \a name (an alias for a character class). */
  class morph_letterset *letterset(std::string name) const;
  /** \brief Lettersets can imply co-occurence restrictions that are enforced
   *  via bindings. Reset the bindings of all lettersets.
   */
  void undo_letterset_bindings();

  /** Take a surface form and return a list of morphological analyses */
  std::list<tMorphAnalysis> analyze(std::string form);
  /** Generate a surface form from a morphological analysis */
  std::string generate(tMorphAnalysis);

  /** Print the contents of this analyzer for debugging purposes */
  void print(FILE *f);

  /** Initialize the special filter for morphology analysis */
  void initialize_lexrule_filter();

  /** check if to infl rules can feed each other (even through a list of
   *  non-affixation rules) 
   *
   *  this is a weaker version of the rule filter for affixation rules for the
   *  purpose of doing correct filtering during morphological analysis.
   */
  inline bool
  lexfilter_compatible(grammar_rule *mother, grammar_rule *daughter) {
    return _rulefilter.get(mother, daughter);
  }

 private:

  void parse_rule(grammar_rule *, std::string rule, bool suffix);

  void analyze1(tMorphAnalysis form, std::list<tMorphAnalysis> &result);
  bool matching_irreg_form(tMorphAnalysis a);

  void add_subrule(class morph_subrule *sr) 
    { _subrules.push_back(sr); }

  class morph_lettersets _lettersets;
  class morph_trie _suffixrules;
  class morph_trie _prefixrules;

  std::vector<class morph_subrule *> _subrules;

  rulefilter _rulefilter;

  bool _irregs_only;

  /** Check that no rule gets applied twice in a chain
   *  Gets the value of the setting \c orthographemics-duplicate-filter.
   */
  bool _duplicate_filter_p;
  /** The maximal depth of inflection rules (either prefix or suffix).
   *  Gets the value of the setting \c orthographemics-maximum-chain-depth.
   */
  unsigned int _maximal_depth;
  /** Make sure that the length of the base form string does not become smaller
   *  than the value of this variable (in case it is > 0).
   *  Gets the value of the setting \c orthographemics-minimum-stem-length
   */
  unsigned int _minimal_stem_length;

  std::multimap<std::string, tMorphAnalysis *> _irregs_by_stem;
  std::multimap<std::string, tMorphAnalysis *> _irregs_by_form;

  
  friend class morph_trie;
};

/** LKB style online morphology with regexps for suffixes and prefixes and a
 *  table for irregular forms.
 */
class tLKBMorphology : public tMorphology {
public:
  static tLKBMorphology *create(class dumper &dmp);

  virtual ~tLKBMorphology() {}

  /** Compute morphological results for \a form. */
  virtual std::list<tMorphAnalysis> operator()(const myString &form) {
    return _morph.analyze(form);
  }

  virtual std::string description() { return "LKB style morphology"; }

private:
  tLKBMorphology() {}
  void undump_inflrs(class dumper &dmp);
  void undump_irregs(class dumper &dmp);

  tMorphAnalyzer _morph;
};

/** Take an input token and compute a list of input tokens with morphological
 *  information stored in the \c _inflrs_todo and \c _stem fields.
 */
class tFullformMorphology : public tMorphology {
private:
  
  typedef HASH_SPACE::hash_map<std::string, std::list<tMorphAnalysis>,
                               standard_string_hash > ffdict ;

  /** Prerequisite: The dumper must be set to the beginning of the (existing)
   *  fullform section
   */
  tFullformMorphology(class dumper &dmp);

public:
  /** \brief Create a new full form morphology, if available. Return NULL
   *  otherwise 
   */
  static tFullformMorphology *create(class dumper &dmp);

  virtual ~tFullformMorphology() {}
  
  /** Compute morphological results for \c token. */
  virtual std::list<tMorphAnalysis> operator()(const myString &form);

  virtual std::string description() { return "full form table morphology"; }

  /** Print the table in .voc format for debugging */
  void print(FILE *);
private:
  ffdict _fullforms;
  std::list< tMorphAnalysis > _emptyresult;
};

#endif
