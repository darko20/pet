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

/* Chart item hierarchy */

#ifndef _ITEM_H_
#define _ITEM_H_

#include <limits.h>
#include "list-int.h"
#include "fs.h"
#include "options.h"
#include "grammar.h"
#include "paths.h"
#include "postags.h"
#include "item-printer.h"

/** Represent an item in a chart. Conceptually there are input items,
 *  morphological items, lexical items and phrasal items. 
 */

class tItem
{
 public:
  tItem(int start, int end, const tPaths &paths, const fs &f,
       const char *printname);
  tItem(int start, int end, const tPaths &paths, 
       const char *printname);

  virtual ~tItem();

  virtual tItem &operator=(const tItem &li)
  {
    throw tError("unexpected call to copy constructor of tItem");
  }

  tItem()
  {
    throw tError("unexpected call to constructor of tItem");
  }

  static void default_owner(class item_owner *o) { _default_owner = o; }
  static class item_owner *default_owner() { return _default_owner; }

  static void reset_ids() { _next_id = 1; }

  inline int id() { return _id; }
  inline rule_trait trait() { return _trait; }

  inline bool passive() const { return _tofill == 0; }
  inline int start() const { return _start; }
  inline int end() const { return _end; }
  inline int span() const { return (_end - _start); }

  bool spanningonly() { return _spanningonly; }

  inline bool compatible(class grammar_rule *R, int length)
  {
      if(R->trait() == INFL_TRAIT)
      {
          if(_trait != INFL_TRAIT)
              return false;
          
          if(first(_inflrs_todo) != R->type())
              return false;
      }
      else if(R->trait() == LEX_TRAIT)
      {
          if(_trait == INFL_TRAIT && first(_inflrs_todo) != R->type())
              return false;
      }
      else if(R->trait() == SYNTAX_TRAIT)
      {
          if(_trait == INFL_TRAIT)
              return false;
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
  
  inline bool compatible(tItem *active, int length)
  {
    if((_trait == INFL_TRAIT) || (_trait == INPUT_TRAIT))
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
  
  inline bool left_extending()
  {
      return _tofill == 0 || first(_tofill) == 1;
  }

  inline bool adjacent(class tItem *passive) // assumes `this' is an active item
  {
      return (left_extending() ? (_start == passive->_end)
              : (_end == passive->_start));
  }

  inline bool root(class tGrammar *G, int length, type_t &rule)
  {
      if(_trait == INFL_TRAIT)
          return false;
      
      if(_start == 0 && _end == length)
          return G->root(_fs, rule);
      else
          return false;
  }
  
  virtual fs get_fs(bool full = false)
  {
      if(_fs.temp() && _fs.temp() != unify_generation)
          recreate_fs();
      return _fs;
  }

  type_t type()
  {
      return get_fs().type();
  }
  
  inline int nextarg() { return first(_tofill); }
  inline fs nextarg(fs &f) { return f.nth_arg(nextarg()); }
  inline list_int *restargs() { return rest(_tofill); }
  inline int arity() { return length(_tofill); }
  inline int nfilled() { return _nfilled; }

  virtual int startposition() const {
    return _daughters.front()->startposition() ;
  }
  virtual int endposition() const {
    return _daughters.back()->endposition() ;
  }

  virtual void print(FILE *f, bool compact = false);
  virtual void print_family(FILE *f) = 0;
  virtual void print_packed(FILE *f);
  virtual void print_derivation(FILE *f, bool quoted) = 0;
  
  virtual void print_yield(FILE *f) = 0;

  virtual string tsdb_derivation(int protocolversion) = 0;

  virtual void print_gen(class tItemPrinter *ip) = 0;

  virtual void daughter_ids(list<int> &ids) = 0;
  // Collect all (transitive) children. Uses frosting mechanism.
  virtual void collect_children(list<tItem *> &result) = 0;

  inline type_t result_root() { return _result_root; }
  inline bool result_contrib() { return _result_contrib; }

  virtual void set_result_root(type_t rule ) = 0;
  virtual void set_result_contrib() = 0;
  
  inline type_t *qc_vector_unif() { return _qc_vector_unif; }
  inline type_t *qc_vector_subs() { return _qc_vector_subs; }

  virtual grammar_rule *rule() = 0;

  virtual void recreate_fs() = 0;

  virtual int identity() = 0;

  double
  score()
  {
      return _score;
  }

  void
  score(double score)
  {
      _score = score; 
  }

  void block(int mark);
  inline void frost() { block(1); }
  inline void freeze() { block(2); }
  inline bool blocked() { return _blocked != 0; }
  inline bool frosted() { return _blocked == 1; }
  inline bool frozen() { return _blocked == 2; }

  list<tItem *> unpack(int limit);
  virtual list<tItem *> unpack1(int limit) = 0;

  inline const char *printname() { return _printname.c_str(); }

  inline list<tItem *> &daughters() { return _daughters; }

 private:
  static class item_owner *_default_owner;

  static int _next_id;

  int _id;

  rule_trait _trait;

  int _start, _end;

  bool _spanningonly;

  tPaths _paths;
  
  fs _fs;

  list_int *_tofill;

  int _nfilled;

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
  list<tItem *> parents;
  list<tItem *> packed;

  friend class tInputItem;
  friend class tLexItem;
  friend class tPhrasalItem;
};

enum tok_class { SKIP_TOKEN_CLASS = -3, WORD_TOKEN_CLASS, STEM_TOKEN_CLASS };

/**
 * tItem must be the superclass to be able to add these items to the chart.
 * These items are the results of tokenization and NE recognition, possibly
 * with different slots filled
 */
class tInputItem : public tItem {
public:
  tInputItem(int id, int start, int end, string surface, string stem
             , const tPaths &paths, tok_class token_class = WORD_TOKEN_CLASS);
  
  ~tInputItem() {}

  virtual tInputItem &operator=(const tItem &li)
  {
    throw tError("unexpected call to assignment operator of tLexItem");
  }

  tInputItem(const tInputItem &li)
  {
    throw tError("unexpected call to copy constructor of tLexItem");
  }

  virtual void print(FILE *f, bool compact=true);
  virtual void print_family(FILE *f) {}
  virtual void print_yield(FILE *f);
  /** Print a readable description of the derivation tree encoded in this item.
   */
  virtual void print_derivation(FILE *f, bool quoted);

  /** Print a machine readable description of the derivation tree encoded in
   *  this item for use with the incr[tsdb] system.
   */
  virtual string tsdb_derivation(int protocolversion);

  virtual void print_gen(class tItemPrinter *ip) ;

  virtual void daughter_ids(list<int> &ids);
  // Collect all (transitive) children. Uses frosting mechanism.
  virtual void collect_children(list<tItem *> &result);

  virtual void set_result_root(type_t rule );
  virtual void set_result_contrib() { _result_contrib = true; }

  virtual grammar_rule *rule();

  virtual void recreate_fs();

  string description() {
    // _fix_me_ XXX i will fix this when i know what "description" ought to do
    return _surface;
  }
  
  string orth();

  /** Return the external positions of this item */
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
  bool is_stem_token() const { return (_class == WORD_TOKEN_CLASS); }

  /** Return the class (hopefully the type id of a valid HPSG lexical type,
   *  i.e., \c lex_stem)
   */
  type_t tclass() const { return _class ; }

  /** Return the list of feature structure modifications. */
  modlist &mods() { return _fsmods ; }

  /** Set the inflrs_todo (inflection rules <-> morphology) */
  void inflrs(const list<int> &infl_rules) {
    free_list(_inflrs_todo);
    _inflrs_todo = copy_list(infl_rules);
  }

  void set_in_postags(const postags &p) { _postags = p; }
  const postags &get_in_postags() { return _postags; }
  /** XXX _fix_me_ I'll fix this when i have a model for postags */
  const postags &get_supplied_postags() { return _postags; }

  /** Get the inflrs_todo (inflection rules <-> morphology) */
  list_int *inflrs() { return _inflrs_todo; }

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


  /** Set the start resp. end node number of this item */
   /*@{*/
  void set_start(int pos) { _start = pos ; }
  void set_end(int pos) { _end = pos ; }
  /*@}*/

  virtual int identity() { return _class; }

  virtual list<tItem *> unpack1(int limit);

private:  
  int _input_id; // external ID

  int _startposition, _endposition;  // external position

  int _class; // token or NE-class (an HPSG type code), or one of tok_class

  /** The surface form, as delivered by the tokenizer. May possibly be put into
   *  tItem's printname 
   */
  string _surface;

  /** for morphologized items, to be able to do lexicon access */
  string _stem;

  // Additional FS modifiers: (path . value)
  modlist _fsmods;

  postags _postags;

  // inflrs_todo und score aus tItem
  friend class tItemPrinter;
};

class tLexItem : public tItem
{
 public:
  //tLexItem(int start, int end, const tPaths &paths,
  //         int ndtrs, int keydtr, class input_token **dtrs,
  //         fs &f, const char *name);

  /** Build a new tLexitem from stem and morph info, together with the first
   *  daughter.
   */
  tLexItem(lex_stem *stem, tInputItem *first_dtr
           , fs &f, const list_int *inflrs_todo);

  /** Build a new tLexItem from an active tLexItem and another tInputItem (a
   *  new daughter).
   */
  tLexItem(tLexItem *from, tInputItem *new_dtr);

  ~tLexItem() { }

  virtual tLexItem &operator=(const tItem &li)
  {
    throw tError("unexpected call to assignment operator of tLexItem");
  }

  tLexItem(const tLexItem &li)
  {
    throw tError("unexpected call to copy constructor of tLexItem");
  }

  virtual void print(FILE *f, bool compact = false);
  virtual void print_family(FILE *f) {}

  virtual void print_derivation(FILE *f, bool quoted);
  virtual void print_yield(FILE *f);
  virtual string tsdb_derivation(int protocolversion);

  virtual void print_gen(class tItemPrinter *ip) ;

  virtual void daughter_ids(list<int> &ids);
  // Collect all (transitive) children. Uses frosting mechanism.
  virtual void collect_children(list<tItem *> &result);

  virtual void set_result_root(type_t rule);
  virtual void set_result_contrib() { _result_contrib = true; }

  virtual grammar_rule *rule();

  virtual fs get_fs(bool full = false)
  {
      return full ? _fs_full : _fs;
  }

  virtual void recreate_fs();

  string description();
  string orth();

  bool passive() {
    return (_ldot == 0) && (_rdot == _stem->length());
  }

  bool left_extending() {
    return _ldot > 0;
  }
  
  int nextarg() {
    return left_extending() ? _ldot - 1 : _rdot;
  }

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

  // bool synthesized() { return _dtrs[_keydtr]->synthesized(); }

  // friend bool same_lexitems(const tLexItem &a, const tLexItem &b);

  virtual int identity()
  {
    return leaftype_parent(_stem->type()); // _dtrs[_keydtr]->identity();
  }

  virtual list<tItem *> unpack1(int limit);

  bool compatible(tInputItem *inp) {
    if (form(nextarg()) != inp->form()) return false;

    // check if we already did this combination before: is the new start
    // resp. end point registered?
    int pos = (left_extending() ? inp->start() : inp->end());
    return (find(_expanded.begin(), _expanded.end(), pos) == _expanded.end());
  }

 private:
  void init(const fs &fs);

  int _ldot, _rdot;
  tInputItem *_keydaughter;
  lex_stem * _stem;

  /** This list registers all immediate expansions of this items to avoid
   *  duplicate multi word entries.
   *
   * Since different active items emerging from a tLexItem can only differ in
   * start resp. end position, we only need to register the new start/endpoint.
   */
  list< int > _expanded;

  postags _supplied_pos;

  fs _fs_full; // unrestricted (packing) structure

  friend class tItemPrinter;
};

class tPhrasalItem : public tItem
{
 public:
  tPhrasalItem(class grammar_rule *, class tItem *, fs &);
  tPhrasalItem(class tPhrasalItem *, class tItem *, fs &);
  tPhrasalItem(class tPhrasalItem *, vector<class tItem *> &, fs &);

  virtual void print(FILE *f, bool compact = false);
  virtual void print_family(FILE *f);
  virtual void print_derivation(FILE *f, bool quoted);
  virtual void print_yield(FILE *f);
  virtual string tsdb_derivation(int protocolversion);

  virtual void print_gen(class tItemPrinter *ip) ;

  virtual void daughter_ids(list<int> &ids);
  // Collect all (transitive) children. Uses frosting mechanism.
  virtual void collect_children(list<tItem *> &result);

  virtual void set_result_root(type_t rule);
  virtual void set_result_contrib() { _result_contrib = true; }

  virtual grammar_rule *rule();

  virtual void recreate_fs();

  virtual int identity()
  {
      if(_rule)
          return _rule->type();
      else
          return 0;
  }
  virtual list<tItem *> unpack1(int limit);
  void unpack_cross(vector<list<tItem *> > &dtrs,
                    int index, vector<tItem *> &config,
                    list<tItem *> &res);
  tItem *unpack_combine(vector<tItem *> &config);

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

class item_owner
// allow proper release of memory
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

typedef list< tItem * > item_list;
typedef list< tItem * >::iterator item_iter;
typedef list< tInputItem * > inp_list;
typedef list< tInputItem * >::iterator inp_iterator;

#endif
