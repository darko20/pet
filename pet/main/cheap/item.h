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
#include "inputtoken.h"

/** Represent an item in a chart. Conceptually there are input items,
 *  morphological items, lexical items and phrasal items. 
 */

class tItem
{
 public:

    /** Does this item have any further completion requirements? */
    virtual bool
    passive() = 0;

    /** If this item is not passive(), i.e. it has further completion
     *  requirements, does it extend to the left? If the item is
     *  passive(), returns true. */
    virtual bool
    leftExtending() = 0;

    virtual int
    arity() = 0;

    /** _fix_me_ length shouldn't be passed here; also it should do
     *  a more complete job */
    virtual bool
    compatible(tItem *passive, int length) = 0;
    
  tItem(int start, int end, const tPaths &paths, fs &f,
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
  
  // inline int arity() { return length(_tofill); }

  inline int nextarg() { return first(_tofill); }
  inline fs nextarg(fs &f) { return f.nth_arg(nextarg()); }
  inline list_int *restargs() { return rest(_tofill); }
  inline int nfilled() { return _nfilled; }

  virtual int startposition() = 0;
  virtual int endposition() = 0;

  virtual void print(FILE *f, bool compact = false);
  virtual void print_family(FILE *f) = 0;
  virtual void print_packed(FILE *f);
  virtual void print_derivation(FILE *f, bool quoted) = 0;
  
  virtual void print_yield(FILE *f) = 0;

  virtual string tsdb_derivation(int protocolversion) = 0;

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

  int _blocked;
  list<tItem *> *_unpack_cache;

 public:
  list<tItem *> parents;
  list<tItem *> packed;

  friend class tLexItem;
  friend class tPhrasalItem;
};

/** Represent the combination requirements of an item. Note that passive
 *  items are represented here as a active items with no further combination
 *  requirements. */
class tActive
{
 public:

    tActive(list_int *toFill)
        : _toFill(toFill)
    {}

    int
    arity()
    {
        return length(_toFill);
    }

    bool
    passive()
    {
        return arity() == 0;
    }

    int
    nextArg()
    {
        if(arity() == 0)
            return 0;
        return first(_toFill);
    }

    list_int *
    restArgs()
    {
        if(arity() == 0)
            return 0;
        return rest(_toFill);
    }

    bool
    leftExtending()
    {
        return nextArg() == 1;
    }
 
 private:

    list_int *_toFill;
};

class tLexItem : public tItem, private tActive
{
 public:

    virtual bool
    passive()
    {
        return tActive::passive();
    }

    virtual int
    arity()
    {
        return tActive::arity();
    }

    virtual bool
    leftExtending()
    {
        return tActive::leftExtending();
    }

    virtual bool
    compatible(tItem *passive, int length)
    {
        if(passive->_trait == INFL_TRAIT)
            return false;

        if(spanningonly())
        {
            if(nextarg() == 1)
            {
                if(passive->start() != 0)
                    return false;
            }
            else if(arity() == 1 && !leftExtending())
            {
                if(passive->end() != length)
                    return false;
            }
        }
  
        if(!opt_lattice && !passive->_paths.compatible(_paths))
            return false;
    
        return true;
    }
  


  tLexItem(int start, int end, const tPaths &paths,
           int ndtrs, int keydtr, class input_token **dtrs,
           fs &f, const char *name);

  ~tLexItem() { delete[] _dtrs; }

  virtual tLexItem &operator=(const tItem &li)
  {
    throw tError("unexpected call to assignment operator of tLexItem");
  }

  tLexItem(const tLexItem &li)
      : tActive(0)
  {
    throw tError("unexpected call to copy constructor of tLexItem");
  }

  virtual void print(FILE *f, bool compact = false);
  virtual void print_family(FILE *f) {}

  virtual void print_derivation(FILE *f, bool quoted);
  virtual void print_yield(FILE *f);
  virtual string tsdb_derivation(int protocolversion);

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

  virtual inline int startposition() { return _dtrs[0]->startposition() ; }
  virtual inline int endposition() { 
    return _dtrs[_ndtrs - 1]->endposition() ; }

  inline const postags &get_in_postags()
  { return _dtrs[_keydtr]->get_in_postags(); }
  inline const postags &get_supplied_postags()
  { return _dtrs[_keydtr]->get_supplied_postags(); }

  bool synthesized() { return _dtrs[_keydtr]->synthesized(); }

  friend bool same_lexitems(const tLexItem &a, const tLexItem &b);

  virtual int identity()
  {
      return _dtrs[_keydtr]->identity();
  }

  virtual list<tItem *> unpack1(int limit);

 private:
  int _ndtrs, _keydtr;
  class input_token **_dtrs;

  fs _fs_full; // unrestricted (packing) structure
};

class tPhrasalItem : public tItem, private tActive
{
 public:

    virtual bool
    passive()
    {
        return tActive::passive();
    }

    virtual int
    arity()
    {
        return tActive::arity();
    }

    virtual bool
    leftExtending()
    {
        return tActive::leftExtending();
    }

    virtual bool
    compatible(tItem *passive, int length)
    {
        if(passive->_trait == INFL_TRAIT)
            return false;

        if(spanningonly())
        {
            if(nextarg() == 1)
            {
                if(passive->start() != 0)
                    return false;
            }
            else if(arity() == 1 && !leftExtending())
            {
                if(passive->end() != length)
                    return false;
            }
        }
  
        if(!opt_lattice && !passive->_paths.compatible(_paths))
            return false;
    
        return true;
    }

  tPhrasalItem(class grammar_rule *, class tItem *, fs &);
  tPhrasalItem(class tPhrasalItem *, class tItem *, fs &);
  tPhrasalItem(class tPhrasalItem *, vector<class tItem *> &, fs &);

  virtual void print(FILE *f, bool compact = false);
  virtual void print_family(FILE *f);
  virtual void print_derivation(FILE *f, bool quoted);
  virtual void print_yield(FILE *f);
  virtual string tsdb_derivation(int protocolversion);

  virtual void daughter_ids(list<int> &ids);
  // Collect all (transitive) children. Uses frosting mechanism.
  virtual void collect_children(list<tItem *> &result);

  virtual void set_result_root(type_t rule);
  virtual void set_result_contrib() { _result_contrib = true; }

  virtual grammar_rule *rule();

  virtual void recreate_fs();

  virtual int startposition() { return _daughters.front()->startposition() ; }
  virtual int endposition() { return _daughters.back()->endposition() ; }

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
  list<tItem *> _daughters;
  tItem * _adaughter;
  grammar_rule *_rule;

  friend class active_and_passive_task;
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

#endif
