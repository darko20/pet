/* PET
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
#include "options.h"
#include "grammar.h"
#include "paths.h"
#include "postags.h"

/** this is a hoax to get the cfrom and cto values into the mrs */
void init_characterization();

/** Represent an item in a chart. Conceptually there are input items,
 *  morphological items, lexical items and phrasal items. 
 */
class tItem
{
 public:
  /** Base constructor with feature structure
   * \param start start position in the chart
   * \param end start position in the chart
   * \param paths the set of word graph paths this item belongs to
   * \param f the items feature structure
   * \param printname a readable representation of this item
   */
  tItem(int start, int end, const tPaths &paths, const fs &f,
       const char *printname);
  /** Base constructor
   * \param start start position in the chart
   * \param end start position in the chart
   * \param paths the set of word graph paths this item belongs to
   * \param printname a readable representation of this item
   */
  tItem(int start, int end, const tPaths &paths, 
       const char *printname);

  virtual ~tItem();

  /** Inhibited assignment operator (always throws an error) */
  virtual tItem &operator=(const tItem &li)
  {
    throw tError("unexpected call to copy constructor of tItem");
  }

  /** Inhibited copy constructor (always throws an error) */
  tItem()
  {
    throw tError("unexpected call to constructor of tItem");
  }

  /** Set the owner of all created items to be able to handle destruction
   *  properly.
   */
  static void default_owner(class item_owner *o) { _default_owner = o; }
  /** Return the owner of all created items. */
  static class item_owner *default_owner() { return _default_owner; }

  /** Reset the global counter for the creation of unique internal item ids */
  static void reset_ids() { _next_id = 1; }

  /** Return the unique internal id of this item */
  inline int id() const { return _id; }
  /** Return the trait of this item, which may be:
      -- \c INPUT_TRAIT an input item, still without feature structure
      -- \c INFL_TRAIT an incomplete lexical item that needs application of
                    inflection rules to get complete
      -- \c LEX_TRAIT a lexical item with all inflection rules applied
      -- \c SYNTAX_TRAIT a phrasal item
  */
  inline rule_trait trait() { return _trait; }

  inline bool completep() { return _inflrs_todo == 0; }

  /** Return \c true if this item has all of its arguments filled. */
  inline bool passive() const { return _tofill == 0; }
  /** Start position in the chart */
  inline int start() const { return _start; }
  /** End position in the chart */
  inline int end() const { return _end; }
  /** return end() - start() */
  inline int span() const { return (_end - _start); }

  /** Was this item produced by a rule that may only create items spanning the
   *  whole chart?
   */
  bool spanningonly() { return _spanningonly; }

  /** Cheap compatibility tests of a passive item and a grammar rule.
   *
   * -- INFL items may only be combined with matching inflection or lexical
   *    rules
   * -- rules that may only create items spanning the whole chart check for
   *    appropriate start and end positions
   * -- passive items at the borders of the chart do not combine with rules
   *    that would try to extend them past the border
   */
  inline bool compatible(class grammar_rule *R, int length)
  {
      if(R->trait() == INFL_TRAIT)
      {
        if(_inflrs_todo == 0 || first(_inflrs_todo) != R->type())
          return false;
      }
      else if(R->trait() == LEX_TRAIT)
      {
        if(_trait == SYNTAX_TRAIT) 
          return false;
      }
      else if(R->trait() == SYNTAX_TRAIT)
      {
        if(_inflrs_todo != 0) return false;
      }
      
      if(R->spanningonly())
      {
          if(R->arity() == 1)
          {
              if(span() != length)
                  return false;
          }
          else if(R->nextarg() == 1)
          {
              if(_start != 0)
                  return false;
          }
          else if(R->nextarg() == R->arity())
          {
              if(_end != length)
                  return false;
          }
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
   */
  inline bool compatible(tItem *active, int length)
  {
    if((_trait == INPUT_TRAIT) || !completep())
          return false;
      
      if(active->spanningonly())
      {
          if(active->nextarg() == 1)
          {
              if(_start != 0)
                  return false;
          }
          else if(active->nextarg() == active->arity() + active->nfilled())
          {
              if(_end != length)
                  return false;
          }
      }
  
      if(!opt_lattice && !_paths.compatible(active->_paths))
          return false;
    
      return true;
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
  inline bool adjacent(class tItem *passive)
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
  inline bool root(class tGrammar *G, int length, type_t &rule)
  {
      if(_trait == INFL_TRAIT)
          return false;
      
      if(_start == 0 && _end == length)
          return G->root(_fs, rule);
      else
          return false;
  }
  
  /** Return the feature structure of this item.
   * \todo This function is only needed because some items may have temporary
   * dags, which have not been copied after unification. This functionality is
   * rather messy and should be cleaned up.
   */
  virtual fs get_fs(bool full = false)
  {
      if(_fs.temp() && _fs.temp() != unify_generation)
          recreate_fs();
      return _fs;
  }

  /** Return the root type of this item's feature structure */
  type_t type()
  {
      return get_fs().type();
  }
  
  /** Return the number of next argument to fill, ranging from zero to
   *  arity() - 1.
   */
  inline int nextarg() { return first(_tofill); }
  /** Return the substructure of this item's feature structure that represents
   *  the next argument to be filled.
   */
  inline fs nextarg(fs &f) { return f.nth_arg(nextarg()); }
  /** Return the yet to fill arguments of this item, except for the current
   *  one 
   */
  inline list_int *restargs() { return rest(_tofill); }
  /** The number of arguments yet to be filled */
  inline int arity() { return length(_tofill); }
  /** The number of arguments that are already filled. */
  inline int nfilled() { return _nfilled; }

  /** The external start position of that item */
  virtual int startposition() const { return _startposition; }
  /** The external end position of that item */
  virtual int endposition() const { return _endposition; }

  /** \brief Print item readably to \a f. Don't be too verbose if \a compact is
   * false.
   */
  virtual void print(FILE *f, bool compact = false);
  /** Print the ID's of daughters and parents of this item to \a f */
  void print_family(FILE *f);
  /** Print the ID's of all items packed into this item to \a f */
  virtual void print_packed(FILE *f);
  /** \brief Print the whole derivation of this item to \a f. Escape double
   *  quotes with backslashes if \a quoted is \c true.
   */
  virtual void print_derivation(FILE *f, bool quoted) = 0;
  
  /** Print the yield of this item */
  virtual void print_yield(FILE *f) = 0;

  /** Dump item in a format feasible for LUI (?) into \a directory */
  void lui_dump(const char* directory = "/tmp");

  /** Print the derivation of this item in incr[tsdb()] compatible form,
   *  according to \a protocolversion.
   */
  virtual string tsdb_derivation(int protocolversion) = 0;

  /** Function to enable printing through printer object \a ip via double
   *  dispatch. \see tItemPrinter class
   */
  virtual void print_gen(class tItemPrinter *ip) const = 0;

  /** Collect the IDs of all daughters into \a ids */
  virtual void daughter_ids(list<int> &ids) = 0;

  /** \brief Collect all (transitive) children into \a result.
   * \attention Uses frosting mechanism \em outside the packing functionality
   * to avoid duplicates in \a result.
   */
  virtual void collect_children(list<tItem *> &result) = 0;

  /** Return the root node type that licensed this item as result, or -1, if
   *  this item is not a result.
   */
  inline type_t result_root() { return _result_root; }
  /** Return \c true if this item contributes to a result */
  inline bool result_contrib() { return _result_contrib; }

  /** Set the root node licensing this item as result */
  virtual void set_result_root(type_t rule ) = 0;
  /** This item contributes to a result */
  virtual void set_result_contrib() = 0;
  
  /** Return the unification quick check vector of this item.
   * For active items, this is the quick check vector corresponding to the next
   * daughter that has to be filled, while for passive items, this is the qc
   * vector of the mother.
   */
  inline type_t *qc_vector_unif() { return _qc_vector_unif; }
  /** \brief Return the subsumption quick check vector of this item. Only
   *  passive items can have this qc vector.
   */
  inline type_t *qc_vector_subs() { return _qc_vector_subs; }

  /** \brief Return the rule this item was built from. This returns values
   *  different from \c NULL only for phrasal items.
   */
  virtual grammar_rule *rule() = 0;

  /** Return the complete fs for this item. This may be more than a simple
   *  access, e.g. in cases of hyperactive parsing and temporary dags.
   *  This function may only be called for phrasal items.
   */
  virtual void recreate_fs() = 0;

  /** Return the HPSG type this item stems from */
  virtual int identity() const = 0;

  /** Return the score for this item */
  double score() const { return _score; }

  /** Set the score for this item to \a score */
  void score(double score) { _score = score; }

  /** @name Blocking Functions
   * Functions used to block items for packing
   * \todo needs a description of the packing functionality. oe, could you do
   * this pleeaaazze :-)
   */
  /*@{*/
  inline void frost() { block(1); }
  inline void freeze() { block(2); }
  inline bool blocked() { return _blocked != 0; }
  inline bool frosted() { return _blocked == 1; }
  inline bool frozen() { return _blocked == 2; }

  list<tItem *> unpack(int limit);
  /*@}*/

  /** Return a meaningful external name. */
  inline const char *printname() const { return _printname.c_str(); }

  /** Return the list of daughters. */
  inline const list<tItem *> &daughters() const { return _daughters; }

 protected:
  /** \brief Base unpacking function called by unpack for each item. Stops
   *  unpacking when \a limit edges have been unpacked.
   *
   * \return the list of items represented by this item
   */
  virtual list<tItem *> unpack1(int limit) = 0;

 private:
  // Internal function for packing/frosting
  void block(int mark);

  static class item_owner *_default_owner;

  static int _next_id;

  int _id;

  rule_trait _trait;

  int _start, _end;
  
  // external position
  int _startposition, _endposition;

  bool _spanningonly;

  tPaths _paths;
  
  fs _fs;

  list_int *_tofill;

  int _nfilled;

  tLexItem *_key_item;

  list_int *_inflrs_todo;

  type_t _result_root;
  bool _result_contrib;

  type_t *_qc_vector_unif;
  type_t *_qc_vector_subs;

  double _score;

  const string _printname;

  list<tItem *> _daughters;

  int _blocked;
  list<tItem *> *_unpack_cache;

 public:
  /** The parents of this node */
  list<tItem *> parents;

  /** If this list is not empty, this item is a representative of a class of
   *  packed items.
   */
  list<tItem *> packed;

  friend class tInputItem;
  friend class tLexItem;
  friend class tPhrasalItem;
  
  friend class tItemPrinter;
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
 * tItem must be the superclass to be able to add these items to the chart.
 * These items are the results of tokenization and NE recognition, possibly
 * with different slots filled
 */
class tInputItem : public tItem {
public:
  /** Create a new input item.
   * \param id A unique external id
   * \param start The external start position
   * \param end The external end position
   * \param surface The surface form for this entry.
   * \param stem The base form for this entry. See remark at the end of this
   *             comment.
   * \param paths The paths (ids) in the input graph this item belongs to.
   * \param token_class One of \c SKIP_TOKEN_CLASS, \c WORD_TOKEN_CLASS (the
   *                    default), \c STEM_TOKEN_CLASS or a valid HPSG type.
   * \param fsmods A list of feature structure modifications (default: no
   *               modifications).
   * The \a stem argument is ignored if \a token_class is not \c
   * STEM_TOKEN_CLASS. If \a token_class is \c WORD_TOKEN_CLASS, it is produced
   * by calling morphological analysis, if it is a HPSG type, the lexicon entry
   * in accessed directly using this type.
   */
  tInputItem(string id, int start, int end, string surface, string stem
             , const tPaths &paths = tPaths()
             , int token_class = WORD_TOKEN_CLASS
             , modlist fsmods = modlist());
  
  /** Create a new complex input item (an input item with input item
   *  daughters). 
   * \param id A unique external id
   * \param dtrs The daughters of this node.
   * \param stem The base form for this entry. See remark at the end of this
   *             comment.
   * \param token_class One of \c SKIP_TOKEN_CLASS, \c WORD_TOKEN_CLASS (the
   *                    default), \c STEM_TOKEN_CLASS or a valid HPSG type.
   * \param fsmods A list of feature structure modifications (default: no
   *               modifications) .
   * The \a stem argument is ignored if \a token_class is not \c
   * STEM_TOKEN_CLASS. If \a token_class is \c WORD_TOKEN_CLASS, it is produced
   * by calling morphological analysis, if it is a HPSG type, the lexicon entry
   * in accessed directly using this type.
   */
  tInputItem(string id, const list< tInputItem * > &dtrs, string stem
             , int token_class = WORD_TOKEN_CLASS
             , modlist fsmods = modlist());
  
  ~tInputItem() {
    free_list(_inflrs_todo); 
  }

  /** Inhibited assignment operator (always throws an error) */
  virtual tInputItem &operator=(const tItem &li)
  {
    throw tError("unexpected call to assignment operator of tLexItem");
  }
 
  /** Inhibited copy constructor (always throws an error) */
  tInputItem(const tInputItem &li)
  {
    throw tError("unexpected call to copy constructor of tLexItem");
  }

  /** \brief Print item readably to \a f. \a compact is ignored here */
  virtual void print(FILE *f, bool compact=true);
  /** Print the yield of this item */
  virtual void print_yield(FILE *f);
  /** Print a readable description of the derivation tree encoded in this item.
   */
  virtual void print_derivation(FILE *f, bool quoted);

  /** Print a machine readable description of the derivation tree encoded in
   *  this item for use with the incr[tsdb] system.
   */
  virtual string tsdb_derivation(int protocolversion);

  /** Function to enable printing through printer object \a ip via double
   *  dispatch. \see tItemPrinter class
   */
  virtual void print_gen(class tItemPrinter *ip) const ;

  /** Collect the IDs of all daughters into \a ids */
  virtual void daughter_ids(list<int> &ids);
  /** \brief Collect all (transitive) children into \a result.
   * \attention Uses frosting mechanism \em outside the packing functionality
   * to avoid duplicates in \a result.
   */
  virtual void collect_children(list<tItem *> &result);

  /** Set the root node licensing this item as result */
  virtual void set_result_root(type_t rule );
  /** This item contributes to a result */
  virtual void set_result_contrib() { _result_contrib = true; }

  /** Always returns \c NULL */
  virtual grammar_rule *rule();

  /** This method always throws an error */
  virtual void recreate_fs();

  /** I've got no clue.
   * \todo i will fix this when i know what "description" ought to do
   */
  string description() { return _surface; }
  
  /** Return the string(s) that is (are) the input to this item */
  string orth() const;

  /** @name External Positions
   * Return the external positions of this item
   */
  /*@{*/
  virtual int startposition() const { return _startposition ; }
  virtual int endposition() const { return _endposition ; }
  /*@}*/

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

  /** Set the list of feature structure modifications. */
  void set_mods(modlist &mods) { _fsmods = mods; }

  /** Set the postags coming from the input */
  void set_in_postags(const postags &p) { _postags = p; }
  /** Get the postags coming from the input */
  const postags &get_in_postags() { return _postags; }
  /** I've got no clue. 
   * \todo I'll fix this when i have an idea where supplied postags come from 
   */
  const postags &get_supplied_postags() { return _postags; }

  /** Get the inflrs_todo (inflection rules <-> morphology) */
  list_int *inflrs() { return _inflrs_todo; }

  /** Set the inflrs_todo (inflection rules <-> morphology) */
  void set_inflrs(const list<int> &infl_rules) {
    free_list(_inflrs_todo);
    _inflrs_todo = copy_list(infl_rules);
  }

  /** The surface string (if available).
   * If this input item represents a named entity, this string may be empty,
   * although the item represents a nonempty string.
   */
  string form() const { return _surface; }

  /** The base (uninflected) form of this item (if available).
   */
  string stem() const { return _stem; }

  /** Return generic lexical entries for this input token. If \a onlyfor is 
   * non-empty, only those generic entries corresponding to one of those
   * POS tags are postulated. The correspondence is defined in posmapping.
   */
  list<lex_stem *> generics(postags onlyfor = postags());

  /** @name Set Internal Positions
   * Set the start resp. end node number of this item
   */
  /*@{*/
  void set_start(int pos) { _start = pos ; }
  void set_end(int pos) { _end = pos ; }
  /*@}*/

  /** Return the HPSG type this item stems from */
  virtual int identity() const { return _class; }

  /** \brief Since a tInputItem do not have a feature structure, and can thus
   * have no other items packed into them, they need not be unpacked. Unpacking
   * does not proceed past tLexItem.
   */
  virtual list<tItem *> unpack1(int limit);

  /** Return the external id associated with this item */
  const string &external_id() { return _input_id; }
  
private:  
  string _input_id; /// external ID

  int _class; /// token or NE-class (an HPSG type code), or one of tok_class

  /** The surface form, as delivered by the tokenizer. May possibly be put into
   *  tItem's printname 
   */
  string _surface;

  /** for morphologized items, to be able to do lexicon access */
  string _stem;

  /// Additional FS modifiers: (path . value)
  modlist _fsmods;

  /** The postags from the input */
  postags _postags;

  // inflrs_todo und score aus tItem
  friend class tItemPrinter;
};

/** An item created from an input item with a corresponding lexicon
 *  entry.
 *  Contains also morphological and part of speech information
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
   *  new daughter).
   */
  tLexItem(tLexItem *from, tInputItem *new_dtr);

  ~tLexItem() {
    free_list(_inflrs_todo); 
  }

  /** Inhibited assignment operator (always throws an error) */
  virtual tLexItem &operator=(const tItem &li)
  {
    throw tError("unexpected call to assignment operator of tLexItem");
  }

  /** Inhibited copy constructor (always throws an error) */
  tLexItem(const tLexItem &li)
  {
    throw tError("unexpected call to copy constructor of tLexItem");
  }

  /** \brief Print item readably to \a f. Don't be too verbose if \a compact is
   * false.
   */
  virtual void print(FILE *f, bool compact = false);

  /** \brief Print the whole derivation of this item to \a f. Escape double
   *  quotes with backslashes if \a quoted is \c true.
   */
  virtual void print_derivation(FILE *f, bool quoted);
  /** Print the yield of this item */
  virtual void print_yield(FILE *f);
  /** Print the derivation of this item in incr[tsdb()] compatible form,
   *  according to \a protocolversion.
   */
  virtual string tsdb_derivation(int protocolversion);

  /** Function to enable printing through printer object \a ip via double
   *  dispatch. \see tItemPrinter class
   */
  virtual void print_gen(class tItemPrinter *ip) const ;

  /** Collect the IDs of all daughters into \a ids */
  virtual void daughter_ids(list<int> &ids);

  /** \brief Collect all (transitive) children into \a result. Uses frosting
   * mechanism.
   */
  virtual void collect_children(list<tItem *> &result);

  /** Set the root node licensing this item as result */
  virtual void set_result_root(type_t rule);
  /** This item contributes to a result */
  virtual void set_result_contrib() { _result_contrib = true; }

  /** Always return NULL */
  virtual grammar_rule *rule();

  /** Return either the \a full or the restricted feature structure */
  virtual fs get_fs(bool full = false)
  {
      return full ? _fs_full : _fs;
  }

  /** Always throws an error */
  virtual void recreate_fs();

  /** \todo what is this function good for? */
  string description();
  /** Return the surface string(s) this item originates from */
  string orth();

  /** Is this item passive or not? */
  bool passive() {
    return (_ldot == 0) && (_rdot == _stem->length());
  }

  /** Is this item active and extends to the left? */
  bool left_extending() {
    return _ldot > 0;
  }
  
  /** Return the next argument position to fill */
  int nextarg() {
    return left_extending() ? _ldot - 1 : _rdot;
  }

  /** The surface string of this item: the string encoded in its lex_entry.
   */
  const char *form(int pos) {
    return _stem->orth(pos);
  }

  /** Returns the POS tags coming from the input */
  inline const postags &get_in_postags()
  { 
    return _keydaughter->get_in_postags();
  }
  /** Returns the POS tags given by the lexical stem of this item */
  inline const postags &get_supplied_postags()
  { 
    return _supplied_pos;
  }

  /** Return the HPSG type this item stems from */
  virtual int identity() const
  {
    return _stem->type(); // _dtrs[_keydtr]->identity();
  }

  /** Cheap compatibility tests of an active tLexItem and a tInputItem.
   *  -- The input items surface string must match the string of the next
   *     argument.
   *  -- This item may not have produced an item with the same start and end
   *     position as the new potential result.
   */
  bool compatible(tInputItem *inp) {
    if (form(nextarg()) != inp->form()) return false;

    // check if we already did this combination before: is the new start
    // resp. end point registered?
    int pos = (left_extending() ? inp->start() : inp->end());
    return (find(_expanded.begin(), _expanded.end(), pos) == _expanded.end());
  }

 protected:
  /** \brief Return a list of items that is represented by this item. For this
   *  class of items, the list always contains only the item itself
   */
  virtual list<tItem *> unpack1(int limit);

 private:
  void init(fs &fs);

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
  list< int > _expanded;

  postags _supplied_pos;

  fs _fs_full; // unrestricted (packing) structure

  friend class tPhrasalItem; // to get access to the _mod...fs
  friend class tItemPrinter;
};

/** An item build from a grammar rule and arguments (tPhrasalItem or tLexItem).
 *  May be active or passive.
 */
class tPhrasalItem : public tItem
{
 public:
  /** Build a new phrasal item from the (successful) combination of \a rule and
   *  \a passive, which already produced \a newfs
   */
  tPhrasalItem(grammar_rule *rule, class tItem *passive, fs &newfs);
  /** Build a new phrasal item from the (successful) combination of \a active
   *  and \a passive, which already produced \a newfs
   */
  tPhrasalItem(tPhrasalItem *active, class tItem *passive, fs &newfs);
  /** Constructor for unpacking: Build a new passive phrasal item from the
   *  \a representative with the daughters \a dtrs and the new feature
   *  structure \a newfs.
   */
  tPhrasalItem(tPhrasalItem *representative, vector<tItem *> &dtrs, fs &newfs);

  /** \brief Print item readably to \a f. Don't be too verbose if \a compact is
   * false.
   */
  virtual void print(FILE *f, bool compact = false);
  /** \brief Print the whole derivation of this item to \a f. Escape double
   *  quotes with backslashes if \a quoted is \c true.
   */
  virtual void print_derivation(FILE *f, bool quoted);
  /** Print the yield of this item */
  virtual void print_yield(FILE *f);
  /** Print the derivation of this item in incr[tsdb()] compatible form,
   *  according to \a protocolversion.
   */
  virtual string tsdb_derivation(int protocolversion);

  /** Function to enable printing through printer object \a ip via double
   *  dispatch. \see tItemPrinter class
   */
  virtual void print_gen(class tItemPrinter *ip) const ;

  /** Collect the IDs of all daughters into \a ids */
  virtual void daughter_ids(list<int> &ids);

  /** \brief Collect all (transitive) children into \a result.
   * \attention Uses frosting mechanism \em outside the packing functionality
   * to avoid duplicates in \a result.
   */
  virtual void collect_children(list<tItem *> &result);

  /** Set the root node licensing this item as result */
  virtual void set_result_root(type_t rule);
  /** This item contributes to a result */
  virtual void set_result_contrib() { _result_contrib = true; }

  /** \brief Return the rule this item was built from. This returns values
   *  different from \c NULL only for phrasal items.
   */
  virtual grammar_rule *rule();

  /** Return the complete fs for this item. This may be more than a simple
   *  access, e.g. in cases of hyperactive parsing and temporary dags.
   *  This function may only be called for phrasal items.
   */
  virtual void recreate_fs();

  /** Return the HPSG type this item stems from */
  virtual int identity() const
  {
      if(_rule)
          return _rule->type();
      else
          return 0;
  }

 protected:
  /** @name Unpacking Functions
   * This is the complex case where daughter combinations have to be 
   * considered.
   */
  /*@{*/
  /** Return a list of items that is represented by this item. 
   * This requires first the unpacking of all daughters, and then generate all
   * possible combinations to compute the unpacked items represented here.
   */
  virtual list<tItem *> unpack1(int limit);

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
  void unpack_cross(vector<list<tItem *> > &dtrs,
                    int index, vector<tItem *> &config,
                    list<tItem *> &res);

  /** Try to fill the rule of this item with the arguments in \a config. */
  tItem *unpack_combine(vector<tItem *> &config);
  /*@}*/

 private:
  /** The active item that created this item */
  tItem * _adaughter;
  grammar_rule *_rule;

  friend class active_and_passive_task;
  friend class tItemPrinter;
};

// _fix_me_
#if 0
class greater_than_score
{
public:
    greater_than_score(tSM *sm)
        : _sm(sm) {}

    bool operator() (tItem *i, tItem *j)
    {
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
  ~item_owner()
    {
      for(list<tItem *>::iterator curr = _list.begin(); 
	  curr != _list.end(); 
	  ++curr)
	delete *curr;
      tItem::reset_ids();
    }
  void add(tItem *it) { _list.push_back(it); }
  void print(FILE *stream) {
    for(list<tItem *>::iterator item = _list.begin(); 
        item != _list.end(); 
        ++item)
      if(!(*item)->frozen()) {
        (*item)->print(stream);
        fprintf(stream, "\n");
      } // if
  } // print()
 private:
  list<tItem *> _list;
};

namespace HASH_SPACE {
  /** hash function for pointer that just looks at the pointer content */
  template<> struct hash< tItem * >
  {
    /** \return A hash code for a pointer */
    inline size_t operator()(tItem *key) const
    {
      return (size_t) key;
    }
  };
}

/** A list of chart items */
typedef list< tItem * > item_list;
/** Iterator for item_list */
typedef list< tItem * >::iterator item_iter;
/** Iterator for const item list */
typedef list< tItem * >::const_iterator item_citer;
/** A list of input items */
typedef list< tInputItem * > inp_list;
/** Iterator for inp_list */
typedef list< tInputItem * >::iterator inp_iterator;

/** A virtual base class for predicates on items */
struct item_predicate : public unary_function<bool, tItem *> {
  virtual ~item_predicate() {}
  virtual bool operator()(tItem *item) = 0;
};

/** A function object comparing two items based on their score */
struct item_greater_than_score : public binary_function<bool, tItem*, tItem*> {
  bool operator()(tItem *a, tItem *b) const {
    return a->score() > b->score();
  }
};

#endif
