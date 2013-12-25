/* -*- Mode: C++ -*-
 * PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2003 Ulrich Callmeier uc@coli.uni-sb.de
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

/** \file item.h
 * Chart item hierarchy.
 */

#ifndef _ITEM_H_
#define _ITEM_H_

#include <limits.h>
#include "list-int.h"
#include "fs.h"
#include "fs-chart.h"
#include "grammar.h"
#include "paths.h"
#include "postags.h"
#include "hashing.h"
#include <functional>
#include <ios>

#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

class item_owner;
class tItem;
class tInputItem;
class tLexItem;
class grammar_rule;
class tGrammar;
class tAbstractItemPrinter;

/** this is a hoax to get the cfrom and cto values into the mrs */
void init_characterization();

/** Inhibit assignment operator and copy constructor(always throws an error) */
#define INHIBIT_COPY_ASSIGN(___Type) \
  virtual ___Type &operator=(const ___Type &i) { \
    throw tError("unexpected call to assignment of ___Type"); } \
  ___Type() { \
    throw tError("unexpected call to default constructor of ___Type"); }


/* Some typedef abbreviations for commonly used item container types */

/** A list of chart items */
typedef std::list< tItem * > item_list;
/** Iterator for item_list */
typedef item_list::iterator item_iter;
/** Iterator for const item list */
typedef item_list::const_iterator item_citer;
/** A list of input items */
typedef std::list< tInputItem * > inp_list;
/** Iterator for inp_list */
typedef inp_list::iterator inp_iterator;

/** Represent a possible (not necessarily valid) decomposition of an
    item. */
struct tDecomposition
{
public:
  std::set< std::vector<int> > indices;
  item_list rhs;
  tDecomposition(item_list rhs) {
    this->rhs = rhs;
  }
  tDecomposition(tItem* dtr) {
    this->rhs.push_back(dtr);
  }
};

/** Represent a hypothesis with which the item is constructed. */
struct tHypothesis
{
public:
  std::map<item_list,double> scores;
  tItem* edge;
  tItem* inst_edge;
  bool inst_failed;
  tDecomposition* decomposition;
  std::list<tHypothesis*> hypo_parents;
  std::list<tHypothesis*> hypo_dtrs;
  std::vector<int> indices;
  tHypothesis(tItem* e, tDecomposition* decomp, std::list<tHypothesis*> dtrs, std::vector<int> ind)
  {
    edge = e;
    inst_edge = NULL;
    inst_failed = false;
    decomposition = decomp;
    indices = ind;
    hypo_dtrs = dtrs;
    hypo_parents.clear();
    for (std::list<tHypothesis*>::iterator dtr = hypo_dtrs.begin();
         dtr != hypo_dtrs.end(); ++dtr) {
      (*dtr)->hypo_parents.push_back(this);
    }
  }
  tHypothesis(tItem* e)
  {
    edge = e;
    inst_edge = NULL;
    inst_failed = false;
    decomposition = NULL;
    hypo_parents.clear();
  }
  /** constructor for non-conventional top level usage only*/
  tHypothesis(tItem* e, tHypothesis *hdtr, int idx)
  {
    edge = e;
    inst_edge = NULL;
    inst_failed = false;
    scores = hdtr->scores;
    hypo_dtrs.push_back(hdtr);
    //hdtr->hypo_parents.push_back(this);
    indices.push_back(idx);
  }
};


// #define CFGAPPROX_LEXGEN 1

/** Represent an item in a chart. Conceptually there are input items,
 *  lexical items and phrasal items.
 */
class tItem {
public:
  /** Base constructor with feature structure
   * \param start start position in the chart (node number)
   * \param end end position in the chart (node number)
   * \param paths the set of word graph paths this item belongs to
   * \param f the items feature structure
   * \param printname a readable representation of this item
   */
  tItem(int start, int end, const tPaths &paths, const fs &f,
        const char *printname);

  /** Base constructor without feature structure
   * \param start start position in the chart
   * \param end start position in the chart
   * \param paths the set of word graph paths this item belongs to
   * \param printname a readable representation of this item
   */
  tItem(int start, int end, const tPaths &paths,
        const char *printname);

  virtual ~tItem();

  INHIBIT_COPY_ASSIGN(tItem);

  /** Set the owner of all created items to be able to handle destruction
   *  properly.
   */
  static void default_owner(item_owner *o) { _default_owner = o; }
  /** Return the owner of all created items. */
  static item_owner *default_owner() { return _default_owner; }

  /** Reset the global counter for the creation of unique internal item ids */
  static void reset_ids() { _next_id = 1; }

  /** Return the unique internal id of this item */
  inline int id() const { return _id; }
  /** Return the trait of this item, which may be:
      -- \c INPUT_TRAIT an input item, still without feature structure
      -- \c LEX_TRAIT a lexical item with all inflection rules applied
      -- \c SYNTAX_TRAIT a phrasal item
      \warning Note that \c INFL_TRAIT is not used in items.
  */
  inline rule_trait trait() const { return _trait; }

  /** Return \c true if there are no more pending inflectional rules
   *  for this item.
   */
  inline bool inflrs_complete_p() const { return _inflrs_todo == 0; }

  /** Return \c true if this item has all of its arguments filled. */
  inline bool passive() const { return _tofill == 0; }
  /** Start position (node number) in the chart */
  inline int start() const { return _start; }
  /** End position (node number) in the chart */
  inline int end() const { return _end; }
  /** return end() - start() */
  inline int span() const { return (_end - _start); }

  /** return the paths (ids) in the input graph this item belongs to */
  const tPaths &paths() const { return _paths; }

  /** return the list of still unsatisfied inflection rules (or \c NULL) */
  // THIS SHOULD REMAIN PRIVATE, IF POSSIBLE
  //const list_int *inflrs_todo() const { return _inflrs_todo; }

  /** Set the start node number of this item. */
  void set_start(int pos) { _start = pos ; }
  /** Set the end node number of this item. */
  void set_end(int pos) { _end = pos ; }

  /** Was this item produced by a rule that may only create items spanning the
   *  whole chart?
   */
  bool spanningonly() const { return _spanningonly; }

  /** Cheap compatibility tests of a passive item and a grammar rule.
   *
   * -- INFL items may only be combined with matching inflection or lexical
   *    rules
   * -- rules that may only create items spanning the whole chart check for
   *    appropriate start and end positions
   * -- passive items at the borders of the chart do not combine with rules
   *    that would try to extend them past the border
   */
  inline bool compatible(grammar_rule *R, int length) const {
    if(R->trait() == INFL_TRAIT) {
      if(inflrs_complete_p() || first(_inflrs_todo) != R->type())
        return false;
    }
#ifdef CFGAPPROX_LEXGEN
    else if((R->trait() == SYNTAX_TRAIT) ||
            ((R->trait() == LEX_TRAIT) && (inflrs_complete_p()))) {
      return false;
    }
#endif
    else if(R->trait() == SYNTAX_TRAIT) {
      if(! inflrs_complete_p())
        return false;
    }

    if(R->spanningonly()) {
      if(R->nextarg() == 1 && _start != 0)
        return false;

      if(R->nextarg() == R->arity() && _end != length)
        return false;
    }

    if(opt_shaping == false)
      return true;

    if(R->left_extending())
      return _end + R->arity() - 1 <= length;
    else
      return _start - (R->arity() - 1) >= 0;
  }

  /** Cheap compatibility tests of a passive and an active item.
   *
   * -- \c INFL and \c INPUT items do not combine with active items
   * -- rules that may only create items spanning the whole chart check for
   *    appropriate start and end positions
   * -- when doing lattice parsing, check the compatiblity of the path sets
   * \return \c false if not compatible
   */
  inline bool compatible(tItem *active, int length) const
  {
    if((_trait == INPUT_TRAIT) || !inflrs_complete_p())
      return false;

    if(active->spanningonly()) {
      if(active->nextarg() == 1) {  // is it the first arg?
        if(_start != 0)
          return false;
      } else
        if(active->nextarg() == active->arity() + active->nfilled()) {
          // or the last?
          if(_end != length)
            return false;
        }
    }

    if(opt_lattice && !_paths.compatible(active->_paths))
      return false;

    return true;
  }

  /** Compatibility test of a passive item and a PCFG rules */
  inline bool compatible_pcfg(grammar_rule *pcfg_rule, int length) {
    int arity = pcfg_rule->arity();
    if (pcfg_rule->nth_pcfg_arg(1) != identity())
        return false;
    return _end + arity - 1 <= length;
  }

  /** Compatibility test of a passive and an active item */
  inline bool compatible_pcfg(tItem* active, int length) {

    if ((_trait == INPUT_TRAIT) || !inflrs_complete_p())
      return false;

    // TODO more tricks to be done here?
    if (active->nextarg_pcfg() != identity())
      return false;
    int arity = active->arity();
    return _end + arity - 1 <= length;
  }

  /** Does this active item extend to the left or right?
   *  \pre assumes \c this is an active item.
   *  \todo Remove the current restriction to binary rules.
   */
  inline bool left_extending() const
  {
    return _tofill == 0 || first(_tofill) == 1;
  }

  /** Return \c true if the passive item is at the `open' end of this active
   *  item.
   * \pre assumes \c this is an active item.
   */
  inline bool adjacent(tItem *passive) const
  {
    return (left_extending() ? (_start == passive->_end)
            : (_end == passive->_start));
  }

  /** Check if this item is a parse result.
   *
   * \return \c true if the item is phrasal, spans the whole \a length and is
   * compatible with one of the root FS nodes. Furthermore, the root node is
   * returned in \a rule.
   */
  inline bool root(tGrammar *G, int length, type_t &rule) const {
    if(inflrs_complete_p() && (_start == 0) && (_end == length))
      if(_trait == PCFG_TRAIT) {
        bool result = G->root(identity());
        if(result) rule = identity();
        return result;
      }
      else
        return G->root(_fs, rule);
    else
      return false;
  }

  /** Return the feature structure of this item. */
  virtual fs get_fs(bool full = false) = 0;

  /** Return the root type of this item's feature structure */
  type_t type() const {
    // \todo this is not nice. Should be solved by looking into the
    // possibilities of get_fs()
    return const_cast<tItem *>(this)->get_fs().type();
  }

  /** Return the number of next argument to fill, ranging from one to
   *  arity().
   */
  inline int nextarg() const { return first(_tofill); }
  /** Return the substructure of this item's feature structure that represents
   *  the next argument to be filled.
   */
  inline fs nextarg(fs &f) const { return f.nth_arg(nextarg()); }
  /** Return the next PCFG argument */
  inline type_t nextarg_pcfg() { return rule()->nth_pcfg_arg(first(_tofill)); }

  /** Return the yet to fill arguments of this item, except for the current
   *  one
   */
  inline list_int *restargs() const { return rest(_tofill); }
  /** The number of arguments still to be filled */
  inline int arity() const { return length(_tofill); }
  /** The number of arguments that are already filled. */
  inline int nfilled() const { return _nfilled; }

  /** The external start position of that item */
  virtual int startposition() const { return _startposition; }
  /** The external end position of that item */
  virtual int endposition() const { return _endposition; }

  /** Print the yield of this item */
  virtual std::string get_yield() = 0;

  /** Function to enable printing through printer object \a ip via double
   *  dispatch. \see tAbstractItemPrinter class
   */
  virtual void print_gen(tAbstractItemPrinter *ip) const = 0;

  /** Collect the IDs of all daughters into \a ids */
  virtual void daughter_ids(std::list<int> &ids) = 0;

  /** \brief Collect all (transitive) children into \a result.
   * \attention Uses frosting mechanism \em outside the packing functionality
   * to avoid duplicates in \a result.
   */
  virtual void collect_children(item_list &result) = 0;

  /** Return \c true if this item contributes to a result */
  inline bool result_contrib() const { return _result_contrib; }
  /** This item contributes to a result */
  inline void set_result_contrib() { _result_contrib = true; }

  /** Return the root node type that licensed this item as result, or
   *  T_BOTTOM, if this item is not a result.
   */
  inline type_t result_root() const { return _result_root; }
  /** Set the root node licensing this item as result */
  virtual void set_result_root(type_t rule );

  /** Return the unification quick check vector of this item.
   * For active items, this is the quick check vector corresponding to the next
   * daughter that has to be filled, while for passive items, this is the qc
   * vector of the mother.
   */
  inline const qc_vec &qc_vector_unif() const { return _qc_vector_unif; }
  /** \brief Return the subsumption quick check vector of this item. Only
   *  passive items can have this qc vector.
   */
  inline const qc_vec &qc_vector_subs() const { return _qc_vector_subs; }

  /** \brief Return the rule this item was built from. This returns values
   *  different from \c NULL only for phrasal items.
   */
  virtual grammar_rule *rule() const = 0;

  /** Return node identity for this item, suitable for MEM features */
  virtual int identity() const = 0;

  /** Return the score for this item */
  double score() const { return _score; }

  /** Set the score for this item to \a score */
  void score(double score) { _score = score; }

  /** Return the GM score for this item */
  double gmscore() const { return _gmscore; }

  /** Set the GM score for this item to \a score */
  void gmscore(double gmscore) { _gmscore = gmscore; }

  /** @name Blocking Functions
   * Functions used to block items for packing.
   * See Oepen & Carroll 2000
   */
  /*@{*/
  /** Mark an item as temporarily blocked and optionally freeze all its parents.
   */
  inline void frost(bool freeze_parents = true) { block(1, freeze_parents); }
  /** Mark an item as permanently blocked and optionally freeze all its parents.
   */
  inline void freeze(bool freeze_parents = true) { block(2, freeze_parents); }
  /** \c true if an item is marked as blocked. */
  inline int blocked() const { return _blocked; }
  /** \c true if an item is marked as temporarily blocked. */
  inline bool frosted() const { return _blocked == 1; }
  /** \c true if an item is marked as permanently blocked. */
  inline bool frozen() const { return _blocked == 2; }

  item_list unpack(int limit);
  /*@}*/

  /** \brief Base selective unpacking function that unpacks \a n best
   *  readings of the item (including the readings for items packed
   *  into this item). Stops unpacking when \a upedgelimit edges have
   *  been unpacked.
   *
   *  \return the list of items represented by this item
   */
  //  virtual item_list selectively_unpack(int n, int upedgelimit) = 0;

  /** \brief Selective unpacking function that unpacks \a n best
   *   readings from the list of \a root items. Stop unpacking when \a
   *   upedgelimit edges have been unpacked. It also checks if the
   *   item's \a end to see if it is qualified to be a root.
   *
   *  \return the list of items represented by the list of \a roots
   */
  static item_list selectively_unpack(item_list roots, int n, int end,
      int upedgelimit, long memlimit);

  /** Return a meaningful external name. */
  inline const char *printname() const { return _printname.c_str(); }

  /** Return the list of daughters. */
  inline const item_list &daughters() const { return _daughters; }

  /** Return true if the given edge is a descendent of current edge */
  bool contains_p(const tItem *) const ;

  /** Return true if packing current item into \a it will create a
      cycle in the packed forest */
  bool cyclic_p(const tItem *it) const ;

  /** compare two items for linear precendece; used to sort YY tokens */
  struct precedes
    : public std::binary_function<bool, tItem *, tItem *> {
    bool operator() (tItem *foo, tItem *bar) const {
      return foo->startposition() < bar->startposition();
    }
  };

protected:
  /** Return the complete fs for this item. This may be more than a simple
   *  access, e.g. in cases of hyperactive parsing, temporary dags, or input
   *  feature structures.
   *  This function may only be called for input or phrasal items.
   */
  virtual void recreate_fs() = 0;

  /** \brief Base unpacking function called by unpack for each item. Stops
   *  unpacking when \a limit edges have been unpacked.
   *
   * \return the list of items represented by this item
   */
  virtual item_list unpack1(int limit) = 0;

  /** \brief Base function called by selectively_unpack to generate
   *   the \a i th best hypothesys with specific head \a path .
   *
   *  \return the \a i th best hypothesis of the item
   */
  virtual tHypothesis * hypothesize_edge(item_list path, unsigned int i) = 0;

  /** \brief Base function that instantiate the hypothesis (and
   *   recursively instantiate its sub-hypotheses) until \a upedgelimit
   *   edges have been exhausted.
   *
   *  \return the instantiated item from the hypothesis
   */
  virtual tItem * instantiate_hypothesis(item_list path, tHypothesis * hypo, int upedgelimit, long memlimit) = 0;

private:
  /**
   * Internal function for frosting/freezing
   * @param mark 1=frost, 2=freeze
   * @param freeze_parents frost/freeze all parents of this item
   */
  void block(int mark, bool freeze_parents);

  static item_owner *_default_owner;

  static int _next_id;

  static bool opt_shaping, opt_lattice;
  static bool init_item();

  int _id;

  rule_trait _trait;

  /** chart node positions */ /*@{*/
  int _start, _end;
  /*@}*/

  /** external positions */ /*@{*/
  int _startposition, _endposition;
  /*@}*/

  bool _spanningonly;

  tPaths _paths;

  /** the (possibly restricted) feature structure of this item */
  fs _fs;

  list_int *_tofill;

  int _nfilled;

  tLexItem *_key_item;

  /** List of inflection rules that must be applied to get a valid lex item */
  list_int *_inflrs_todo;

  type_t _result_root;

  /** if \c true, this item contributes to some result tree
   * \todo check if this description is correct
   */
  bool _result_contrib;

  qc_vec _qc_vector_unif;
  qc_vec _qc_vector_subs;

  double _score;
  double _gmscore;

  const std::string _printname;

  item_list _daughters;

  int _blocked;
  item_list *_unpack_cache;

public:
  /** The parents of this node */
  item_list parents;

  /** If this list is not empty, this item is a representative of a class of
   *  packed items.
   */
  item_list packed;

  friend class tInputItem;
  friend class tLexItem;
  friend class tPhrasalItem;

  friend class tAbstractItemPrinter;

  /**
   * \name Generalized chart interface
   * At the moment, tItem objects can be added to the old chart implementation
   * or to the new generalised tChart. The start(), end(), startposition()
   * and endposition() methods as well as the corresponding parameters in the
   * constructors of tItem and its derived classes refer to the old chart
   * implementation. If you want to use tItem objects with the new tChart,
   * you have to use the following methods instead. The tChart implementation
   * will eventually replace the old chart implementation.
   * \see tChart::add_item()
   */
  //@{
private:
  /**
   * Pointer to the tChart object to which this item has been added,
   * otherwise 0.
   */
  tChart *_chart;
  /**
   * Informs this item that it has been added to or removed from the specified
   * chart. This method is meant to be called by the chart itself.
   */
  void notify_chart_changed(tChart *chart);
public:
  /** Gets the vertex preceding this item. */
  tChartVertex* prec_vertex();
  /** Gets the vertex preceding this item. */
  const tChartVertex* prec_vertex() const;
  /** Gets the vertex succeeding this item. */
  tChartVertex* succ_vertex();
  /** Gets the vertex succeeding this item. */
  const tChartVertex* succ_vertex() const;
  //@}
  friend class tChart; // Doxygen doesn't understand this in the member group

};

/** Token classes of an input item.
 * - WORD_TOKEN_CLASS: analyze the surface form of this token
 *              morpologically to get all appropriate lexicon entries.
 * - STEM_TOKEN_CLASS: this item has already been analyzed morphologically,
 *              use the given stem to do lexicon access.
 * - SKIP_TOKEN_CLASS: ignore this token (e.g., punctuation).
 */
enum tok_class { SKIP_TOKEN_CLASS = -3, WORD_TOKEN_CLASS, STEM_TOKEN_CLASS };

/**
 * tInputItems represent items from the tokenization/input format reading
 * process.
 *
 * tInputItems can only have daughters that are tInputItems. That could
 * represent a named entity consisting of several other input tokens.
 * A tInputItem may never be a daughter of a tPhrasalItem, but only of a
 * tLexItem.
 * tItem must be the superclass to be able to add these items to the chart.
 * These items are the results of tokenization and NE recognition, possibly
 * with different slots filled
 */
class tInputItem : public tItem {
public:
  /**
   * \name Constructors
   * Create a new input item.
   * \param id A unique external id
   * \param start The start position (node number, -1 if not yet determined)
   * \param end The end position (node number, -1 if not yet determined)
   * \param startposition The external start position
   * \param endposition The external end position
   * \param surface The surface form for this entry.
   * \param stem The base form for this entry (only used if \a token_class is
   *             \c STEM_TOKEN_CLASS)
   * \param paths The paths (ids) in the input graph this item belongs to.
   * \param token_class One of \c SKIP_TOKEN_CLASS, \c WORD_TOKEN_CLASS
   *                    (perform morphological analysis and lexicon lookup;
   *                    the default), \c STEM_TOKEN_CLASS (lookup lexical item
   *                    by parameter \a stem) or a valid HPSG type (direct
   *                    lexical lookup with \a token_class as the type).
   * \param fsmods A list of feature structure modifications which are applied
   *               to the feature structure of the lexical item (default: no
   *               modifications). DEPRECATED
   * \param token_fs A feature structure describing the input item, as needed
   *               for token mapping. (default: invalid feature structure)
   *               If an invalid token fs is provided, it will get recreated
   *               from the item properties.
   */
  //@{
  /** Create a new input item with internal and external start/end positions. */
  tInputItem(std::string id
             , int start, int end, int startposition, int endposition
             , std::string surface, std::string stem
             , const tPaths &paths = tPaths()
             , int token_class = WORD_TOKEN_CLASS
             , const std::list<int> &infl_rules = std::list<int>()
             , const postags &pos = postags()
             , modlist fsmods = modlist()
             , const fs &token_fs = fs());
  /** Create a new input item with external start/end positions only. */
  tInputItem(std::string id, int startposition, int endposition
             , std::string surface, std::string stem
             , const tPaths &paths = tPaths()
             , int token_class = WORD_TOKEN_CLASS
             , const std::list<int> &infl_rules = std::list<int>()
             , const postags &pos = postags()
             , modlist fsmods = modlist());
  /**
   * Create a new complex input item (an input item with input item
   * daughters as it can be defined in PIC).
   * \deprecated This feature is deprecated since additional structure are
   *             better represented with token feature structures.
   */
  tInputItem(std::string id, const inp_list &dtrs
             , std::string stem
             , int token_class = WORD_TOKEN_CLASS
             , const std::list<int> &infl_rules = std::list<int>()
             , const postags &pos = postags()
             , modlist fsmods = modlist());
  //@}

  ~tInputItem() {
    free_list(_inflrs_todo);
  }

  INHIBIT_COPY_ASSIGN(tInputItem);

  /** Print the yield of this item */
  virtual std::string get_yield();

  /** Function to enable printing through printer object \a ip via double
   *  dispatch. \see tAbstractItemPrinter class
   */
  virtual void print_gen(tAbstractItemPrinter *ip) const ;

  /** Collect the IDs of all daughters into \a ids */
  virtual void daughter_ids(std::list<int> &ids);
  /** \brief Collect all (transitive) children into \a result.
   * \attention Uses frosting mechanism \em outside the packing functionality
   * to avoid duplicates in \a result.
   */
  virtual void collect_children(item_list &result);

  /** Always returns \c NULL */
  virtual grammar_rule *rule() const;

  /** I've got no clue.
   * \todo i will fix this when i know what "description" ought to do
   */
  std::string description() const { return _surface; }

  /** Return the string(s) that is (are) the input to this item */
  std::string orth() const;

  /** Tell me if this token should be skipped before chart positions are
   *  computed.
   */
  bool is_skip_token() const { return (_class == SKIP_TOKEN_CLASS); }

  /** Tell me if morphology and lexicon access should be called for this token.
   */
  bool is_word_token() const { return (_class == WORD_TOKEN_CLASS); }

  /** Tell me if lexicon access should be called for this token (morphological
   *  analysis is already included
   */
  bool is_stem_token() const { return (_class == STEM_TOKEN_CLASS); }

  /** Return the class (hopefully the type id of a valid HPSG lexical type,
   *  i.e., \c lex_stem)
   */
  type_t tclass() const { return _class ; }

  /** Return the list of feature structure modifications. */
  modlist &mods() { return _fsmods ; }

  /**
   * Set the list of feature structure modifications.
   * This will recreate the token fs.
   * \deprecated 1) Pass the fsmods to the constructor.
   *             2) Use token feature structures in combination with chart
   *                mapping to import information into lexical items.
   */
  void set_mods(modlist &mods) {
    _fsmods = mods;
    recreate_fs();
  }

  /**
   * Set the postags coming from the input
   * This will recreate the token fs.
   * \deprecated 1) Pass the pos tags to the constructor.
   *             2) Use token feature structures in combination with chart
   *                mapping to import information into lexical items.
   */
  void set_in_postags(const postags &p) {
    _postags = p;
    recreate_fs();
  }
  /** Get the postags coming from the input */
  const postags & get_in_postags() const { return _postags; }
  /** I've got no clue.
   * \todo I'll fix this when i have an idea where supplied postags come from
   */
  const postags &get_supplied_postags() const { return _postags; }

  /** Get the inflrs_todo (inflection rules <-> morphology) */
  list_int *inflrs() const { return _inflrs_todo; }

  /**
   * Set the inflrs_todo (inflection rules <-> morphology) of the item.
   * This will recreate the token fs.
   * \deprecated 1) Pass the infl_rules to the constructor.
   *             2) Use token feature structures in combination with chart
   *                mapping to import information into lexical items.
   */
  void set_inflrs(const std::list<int> &infl_rules) {
    free_list(_inflrs_todo);
    _inflrs_todo = copy_list(infl_rules);
    recreate_fs();
  }

  /** The surface string (if available).
   * If this input item represents a named entity, this string may be empty,
   * although the item represents a nonempty string.
   */
  std::string form() const { return _surface; }

  /** The base (uninflected) form of this item (if available).
   */
  std::string stem() const { return _stem; }

  /** Return the feature structure of this item. */
  virtual fs get_fs(bool full = false) { return _fs; }

  /** Return generic lexical entries for this input token. If \a onlyfor is
   * non-empty, only those generic entries corresponding to one of those
   * POS tags are postulated. The correspondence is defined in posmapping.
   */
  std::list<lex_stem *> generics(postags onlyfor = postags());

  /** Return node identity for this item, suitable for MEM features */
  virtual int identity() const { return _class; }

  /** \brief Since a tInputItem does not have a feature structure, and can thus
   * have no other items packed into them, they need not be unpacked. Unpacking
   * does not proceed past tLexItem.
   */
  virtual item_list unpack1(int limit);

  /** \brief tInputItem will not have items packed into them. They
      need not be unpacked. */
  virtual tHypothesis * hypothesize_edge(std::list<tItem*> path, unsigned int i);
  virtual tItem * instantiate_hypothesis(std::list<tItem*> path, tHypothesis * hypo, int upedgelimit, long memlimit);
  //  virtual item_list selectively_unpack(int n, int upedgelimit);

  /** Return the external id associated with this item */
  const std::string &external_id() const { return _input_id; }

protected:
  /**
   * (Re)creates an input feature structure from the properties of this item.
   * This is necessary if the tokenizer did not already provide an input fs.
   * In this case, this method will be called by the constructor of the item.
   */
  virtual void recreate_fs();

private:
  std::string _input_id; /// external ID

  int _class; /// token or NE-class (an HPSG type code), or one of tok_class

  /** The surface form, as delivered by the tokenizer. May possibly be put into
   *  tItem's printname
   */
  std::string _surface;

  /** for morphologized items, to be able to do lexicon access */
  std::string _stem;

  /// Additional FS modifiers: (path . value)
  modlist _fsmods;

  /** The postags from the input */
  postags _postags;

  // inflrs_todo und score aus tItem
  friend class tAbstractItemPrinter;
};

/** An item created from an input item with a corresponding lexicon
 *  entry or a lexical or morphological rule
 *  This contains also morphological and part of speech information. The
 *  \c tLexItems with only tInputItems as daughters are there to lexical
 *  entries, maybe for multi word expressions. Otherwise, \c tLexItems are the
 *  results of applications of morphological or lexical rules and have
 *  \c tLexItems as daughters. A \c tLexItem can also be a daughter of a
 *  \c tPhrasalItem.
 */
class tLexItem : public tItem
{
public:
  /** Build a new tLexitem from \a stem and morph info in \a inflrs_todo,
   *  together with the first daughter \a first_dtr.
   */
  tLexItem(lex_stem *stem, tInputItem *first_dtr
           , fs &f, const list_int *inflrs_todo);

  /** Build a new tLexItem from an active tLexItem and another tInputItem (a
   *  new daughter), plus (optionally) the fs for the new item, typically the
   *  result of unifying in input item feature structures.
   */
  tLexItem(tLexItem *from, tInputItem *new_dtr, fs f = fs());

  ~tLexItem() {
    free_list(_inflrs_todo);
    delete _hypo;
  }

  INHIBIT_COPY_ASSIGN(tLexItem);

  /** Print the yield of this item */
  virtual std::string get_yield();

  /** Function to enable printing through printer object \a ip via double
   *  dispatch. \see tAbstractItemPrinter class
   */
  virtual void print_gen(tAbstractItemPrinter *ip) const ;

  /** Collect the IDs of all daughters into \a ids */
  virtual void daughter_ids(std::list<int> &ids);

  /** \brief Collect all (transitive) children into \a result. Uses frosting
   * mechanism.
   */
  virtual void collect_children(item_list &result);

  /** Always return NULL */
  virtual grammar_rule *rule() const ;

  /** Return either the \a full or the restricted feature structure */
  virtual fs get_fs(bool full = false) {
    return full ? _fs_full : _fs;
  }

  /** \todo what is this function good for? */
  std::string description() const;
  /** Return the surface string(s) this item originates from */
  std::string orth() const;

  /** Is this item passive or not? */
  bool passive() const {
    return (_ldot == 0) && (_rdot == _stem->length());
  }

  // *Is this item almost passive, i.e. missing exactly one argument */
  bool near_passive() const {
    return (_ldot == 1 || _rdot == _stem->length() - 1);
  }

  /** Is this item active and extends to the left? */
  bool left_extending() const {
    return _ldot > 0;
  }

  /** Return the next argument position to fill */
  int nextarg() const {
    return left_extending() ? _ldot - 1 : _rdot;
  }

  /** The surface string of this item: the string encoded in its lex_entry.
   */
  const char *form(int pos) const {
    return _stem->orth(pos);
  }

  /** The lexical stem */
  const lex_stem *stem() const {
    return _stem;
  }

  /** Returns the POS tags coming from the input */
  inline const postags &get_in_postags() const {
    return _keydaughter->get_in_postags();
  }
  /** Returns the POS tags given by the lexical stem of this item */
  inline const postags &get_supplied_postags() const {
    return _supplied_pos;
  }

  /** Return node identity for this item, suitable for MEM features */
  virtual int identity() const { return _fs.type(); }

  /** Cheap compatibility tests of an active tLexItem and a tInputItem.
   *  -- The input items surface string must match the string of the next
   *     argument.
   *  -- This item may not have produced an item with the same start and end
   *     position as the new potential result.
   */
  bool compatible(tInputItem *inp) const {
    if (form(nextarg()) != inp->form()) return false;

    // check if we already did this combination before: is the new start
    // resp. end point registered?
    int pos = (left_extending() ? inp->start() : inp->end());
    return (find(_expanded.begin(), _expanded.end(), pos) == _expanded.end());
  }

protected:
  /** Always throws an error */
  virtual void recreate_fs();

  /** \brief Return a list of items that is represented by this item. For this
   *  class of items, the list always contains only the item itself
   */
  virtual item_list unpack1(int limit);

  /** \brief Return the \a i th best hypothesis. For tLexItem, there
   *   is always only one hypothesis, for a given \a path .
   */
  virtual tHypothesis * hypothesize_edge(std::list<tItem*> path, unsigned int i);
  virtual tItem * instantiate_hypothesis(std::list<tItem*> path, tHypothesis * hypo, int upedgelimit, long memlimit);
  //  virtual item_list selectively_unpack(int n, int upedgelimit);

private:
  void init();

  int _ldot, _rdot;
  tInputItem *_keydaughter;

  lex_stem * _stem;

  fs _mod_form_fs, _mod_stem_fs;

  /** This list registers all immediate expansions of this items to avoid
   *  duplicate multi word entries.
   *
   * Since different active items emerging from a tLexItem can only differ in
   * start resp. end position, we only need to register the new start/endpoint.
   */
  std::list< int > _expanded;

  postags _supplied_pos;

  fs _fs_full; // unrestricted (packing) structure

  /** Cache for the hypothesis */
  tHypothesis* _hypo;

  friend class tPhrasalItem; // to get access to the _mod...fs
  friend class tAbstractItemPrinter;
};

/** An item build from a grammar rule and arguments (tPhrasalItem or tLexItem).
 *  May be active or passive.
 */
class tPhrasalItem : public tItem {
public:
  /** Build a new phrasal item from the (successful) combination of \a rule and
   *  \a passive, which already produced \a newfs
   */
  tPhrasalItem(grammar_rule *rule, tItem *passive, fs &newfs);
  /** Build a new phrasal item from the (successful) combination of \a active
   *  and \a passive, which already produced \a newfs
   */
  tPhrasalItem(tPhrasalItem *active, tItem *passive, fs &newfs);
  /** Constructor for unpacking: Build a new passive phrasal item from the
   *  \a representative with the daughters \a dtrs and the new feature
   *  structure \a newfs.
   */
  tPhrasalItem(tPhrasalItem *representative, std::vector<tItem *> &dtrs, fs &newfs);

  /*@{*/
  /** Constructors for PCFG items */
  tPhrasalItem(grammar_rule *rule, class tItem *passive);
  tPhrasalItem(tPhrasalItem *active, class tItem *passive);
  tPhrasalItem(tPhrasalItem *representative, std::vector<tItem *> &dtrs);
  /*@}*/

  virtual ~tPhrasalItem() {
    // clear the hypotheses cache
    for (std::vector<tHypothesis*>::iterator h = _hypotheses.begin();
         h != _hypotheses.end(); ++h)
      delete *h;

    // clear the decomposition cache
    for (std::list<tDecomposition*>::iterator d = decompositions.begin();
         d != decompositions.end(); ++d)
      delete *d;
  }

  INHIBIT_COPY_ASSIGN(tPhrasalItem);

  /** get the yield of this item */
  virtual std::string get_yield();

  /** Function to enable printing through printer object \a ip via double
   *  dispatch. \see tAbstractItemPrinter class
   */
  virtual void print_gen(tAbstractItemPrinter *ip) const ;

  /** Collect the IDs of all daughters into \a ids */
  virtual void daughter_ids(std::list<int> &ids);

  /** \brief Collect all (transitive) children into \a result.
   * \attention Uses frosting mechanism \em outside the packing functionality
   * to avoid duplicates in \a result.
   */
  virtual void collect_children(item_list &result);

  /** Set the root node licensing this item as result */
  virtual void set_result_root(type_t rule);

  /** \brief Return the rule this item was built from. This returns values
   *  different from \c NULL only for phrasal items.
   */
  virtual grammar_rule *rule() const ;

  /** Return the feature structure of this item.
   * \todo This function is only needed because some items may have temporary
   * dags, which have not been copied after unification. This functionality is
   * rather messy and should be cleaned up.
   */
  virtual fs get_fs(bool full = false) {
    if(_fs.temp() && _fs.temp() != unify_generation)
      recreate_fs();
    return _fs;
  }

  /** Return node identity for this item, suitable for MEM features */
  virtual int identity() const {
    if(_rule)
      return _rule->type();
    else
      return 0;
  }

protected:
  /** Return the complete fs for this item. This may be more than a simple
   *  access, e.g. in cases of hyperactive parsing and temporary dags.
   *  This function may only be called for phrasal items.
   */
  virtual void recreate_fs();

  /** @name Unpacking Functions
   * This is the complex case where daughter combinations have to be
   * considered.
   */
  /*@{*/
  /** Return a list of items that is represented by this item.
   * This requires first the unpacking of all daughters, and then generate all
   * possible combinations to compute the unpacked items represented here.
   */
  virtual item_list unpack1(int limit);

  /** Apply the rule that built this item to all combinations of the daughter
   *  items in \a dtrs and collect the results in \a res.
   *  \param dtrs The unpacked daughter items for each argument position
   *  \param index The argument position that is considered in this recursive
   *               call to unpack_cross. If \a index is equal to the rule
   *               arity, \a config has been filled completely and this
   *               configuration is applied to the rule to (potentially)
   *               generate a new unpacked item.
   *  \param config Contains a (possibly partial) combination of daughters from
   *                \a dtrs
   *  \param res contains all successfully built items.
   */
  void unpack_cross(std::vector<item_list> &dtrs,
                    int index, std::vector<tItem *> &config,
                    item_list &res);

  /** Try to fill the rule of this item with the arguments in \a config. */
  tItem *unpack_combine(std::vector<tItem *> &config);
  /*@}*/

  /** \brief Selectively unpack the n-best results of this item.
   *
   * \return The list of unpacked results (up to \a n items)
   */
  //  virtual item_list selectively_unpack(int n, int upedgelimit);

  /** Get the \i th best hypothesis of the item with \a path to root. */
  virtual tHypothesis * hypothesize_edge(std::list<tItem*> path, unsigned int i);

  /** Instantiatve the hypothesis */
  virtual tItem * instantiate_hypothesis(std::list<tItem*> path, tHypothesis * hypo, int upedgelimit, long memlimit);

  /** Decompose edge and return the number of decompositions
   * All the decompositions are recorded in this->decompositions .
   */
  virtual int decompose_edge();

  /** Generate a new hypothesis and add it onto the local agendas. */
  virtual void new_hypothesis(tDecomposition* decomposition, std::list<tHypothesis*> dtrs, std::vector<int> indices);


private:
  /** The active item that created this item */
  tItem * _adaughter;
  grammar_rule *_rule;

  /** A vector of hypotheses*/
  std::vector<tHypothesis*> _hypotheses;
  /** a map from path to corresponding cached hypotheses */
  std::map<std::list<tItem*>,std::vector<tHypothesis*> > _hypotheses_path;
  /** a map from path to corresponding max number of hypothese */
  std::map<std::list<tItem*>,unsigned int> _hypo_path_max;

  /** Hypothesis agenda*/
  std::map<std::list<tItem*>,std::list<tHypothesis*> > _hypo_agendas;

  /** A list of decompositions */
  std::list<tDecomposition*> decompositions;

  friend class active_and_passive_task;
  friend class tAbstractItemPrinter;
};

/** Recursively propagate instantiation failure of the hypothesis to
 * parents.
 */
void propagate_failure(tHypothesis *hypo);

/** Advance the indices vector.
 * e.g. <0 2 1> -> {<1 2 1> <0 3 1> <0 2 2>}
 */
std::list<std::vector<int> > advance_indices(std::vector<int> indices);

/** Insert hypothesis into agenda. Agenda is sorted descendingly
 *
 */
void hagenda_insert(std::list<tHypothesis*> &agenda, tHypothesis* hypo, std::list<tItem*> path);

// \todo _fix_me_
#if 0
class greater_than_score {
public:
  greater_than_score(tSM *sm)
    : _sm(sm) {}

  bool operator() (tItem *i, tItem *j) {
      return i->score(_sm) > j->score(_sm);
  }
private:
  tSM *_sm;
};
#endif


/** Manage proper release of chart items.
 * Destruction of the owner will destroy all items owned by it.
 */
class item_owner
{
 public:
  item_owner() {}
  ~item_owner() {
    for(item_iter curr = _list.begin(); curr != _list.end(); ++curr)
      delete *curr;
    tItem::reset_ids();
  }
  void add(tItem *it) { _list.push_back(it); }
  void print(std::ostream &stream) {
    for(item_iter it = _list.begin(); it != _list.end(); ++it)
      if(!(*it)->frozen()) {
        stream << *it << std::endl;
      } // if
  } // print()
 private:
  item_list _list;
};

namespace HASH_SPACE {
  /** hash function for pointer that just looks at the pointer content */
  template<> struct hash< tItem * > {
    /** \return A hash code for a pointer */
    inline size_t operator()(tItem *key) const {
      return (size_t) key;
    }
  };
}

/** a function type for functions returning true for wanted and false for
 *  unwanted items
 */
typedef bool (*item_predicate)(const class tItem *);

/** Default predicate, selecting all items. */
inline bool alltrue(const class tItem *it) { return true; }

/** Item predicate selecting all passive items. */
inline bool onlypassives(const tItem *item) { return item->passive(); }

/** Item predicate selecting all passive non-blocked items. */
inline bool passive_unblocked(const class tItem *item) {
  return item->passive() && !item->blocked();
}

/** Item predicate selecting all passive non-blocked, non-input items. */
inline bool passive_unblocked_non_input(const class tItem *item) {
  return item->passive() && !item->blocked() && item->trait() != INPUT_TRAIT;
}

/** Item predicate selecting all passive items without pending morphographemic
 * rules. */
inline bool passive_no_inflrs(const tItem *item) {
  return item->passive() && item->inflrs_complete_p();
}

/** \brief This predicate should be used in find_unexpanded if lexical
 *  processing is not exhaustive. All non-input items are valid.
 */
inline bool non_input(const tItem *item) {
  return item->trait() != INPUT_TRAIT;
}

/** \brief This predicate should be used in find_unexpanded if lexical
 *  processing is exhaustive. All items that are not input items and have
 *  satisified all inflection rules are valid.
 */
inline bool lex_complete(const tItem *item) {
  return (item->trait() != INPUT_TRAIT) && (item->inflrs_complete_p());
}

/** A function object comparing two items based on their score */
struct item_greater_than_score :
  public std::binary_function<bool, tItem*, tItem*> {
  bool operator()(tItem *a, tItem *b) const {
    return a->score() > b->score();
  }
};


// ========================================================================
// INLINE DEFINITIONS
// ========================================================================

// =====================================================
// class tItem
// =====================================================

inline void
tItem::notify_chart_changed(tChart *chart)
{
  _chart = chart;
}

inline tChartVertex*
tItem::prec_vertex()
{
  assert(_chart);
  return (_chart->_item_to_prec_vertex.find(this))->second;
}

inline const tChartVertex*
tItem::prec_vertex() const
{
  assert(_chart);
  // "this" is const in const member functions, but since the keys in
  // the map are not declared as const, we have to cast "this":
  tItem* key = const_cast<tItem*>(this);
  return (_chart->_item_to_prec_vertex.find(key))->second;
}

inline tChartVertex*
tItem::succ_vertex()
{
  assert(_chart);
  return (_chart->_item_to_succ_vertex.find(this))->second;
}

inline const tChartVertex*
tItem::succ_vertex() const
{
  assert(_chart);
  // "this" is const in const member functions, but since the keys in
  // the map are not declared as const, we have to cast "this":
  tItem* key = const_cast<tItem*>(this);
  return (_chart->_item_to_succ_vertex.find(key))->second;
}

#endif
