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

/* class `item' */

#ifndef _ITEM_H_
#define _ITEM_H_

#include <limits.h>
#include "list-int.h"
#include "fs.h"
#include "options.h"
#include "grammar.h"
#include "inputtoken.h"

class item
{
 public:
  item(int start, int end, const tPaths &paths, int p, fs &f,
       const char *printname);
  item(int start, int end, const tPaths &paths, int p,
       const char *printname);

  virtual ~item();

  virtual item &operator=(const item &li)
  {
    throw error("unexpected call to copy constructor of item");
  }

  item()
  {
    throw error("unexpected call to constructor of item");
  }

  static void default_owner(class item_owner *o) { _default_owner = o; }
  static class item_owner *default_owner() { return _default_owner; }

  static void reset_ids() { _next_id = 1; }

  inline int id() { return _id; }
  inline rule_trait trait() { return _trait; }

  inline int stamp() { return _stamp; }
  inline void stamp(int t) { _stamp = t; }
  inline bool in_chart() { return _stamp != -1; }

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
  
  inline bool compatible(item *active, int length)
  {
      if(_trait == INFL_TRAIT)
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

  inline bool adjacent(class item *passive) // assumes `this' is an active item
  {
      return (left_extending() ? (_start == passive->_end)
              : (_end == passive->_start));
  }

  inline bool root(class grammar *G, int length, type_t &rule)
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
  
  inline int nextarg() { return first(_tofill); }
  inline fs nextarg(fs &f) { return f.nth_arg(nextarg()); }
  inline list_int *restargs() { return rest(_tofill); }
  inline int arity() { return length(_tofill); }
  inline int nfilled() { return _nfilled; }

  virtual int startposition() = 0;
  virtual int endposition() = 0;

  virtual int age() = 0;

  virtual void print(FILE *f, bool compact = false);
  virtual void print_family(FILE *f) = 0;
  virtual void print_packed(FILE *f);
  virtual void print_derivation(FILE *f, bool quoted) = 0;
  
  virtual void print_yield(FILE *f) = 0;

  virtual void getTagSequence(list<string> &tags, list<list<string> > &words) = 0;

  //
  // on 14-jan-02, the UDF steering committee (with consultation from EMB)
  // once and for all unanimously voted to drop the foolish (PAGE-style)
  // offset computation; for once, it makes comparison of complete charts
  // impossible and, secondly, it breaks the fine tree comparison tool.
  //
  virtual string tsdb_derivation() = 0;

  inline type_t result_root() { return _result_root; }
  inline bool result_contrib() { return _result_contrib; }

  virtual void set_result_root(type_t rule ) = 0;
  virtual void set_result_contrib() = 0;
  
  inline type_t *qc_vector() { return _qc_vector; }

  virtual grammar_rule *rule() = 0;

  virtual void recreate_fs() = 0;

  inline int priority() { return _p; }

  inline void priority(int p) { _p = p; }

  virtual int identity() = 0;
  virtual double score(tSM *) = 0;

  void block(int mark);
  inline void frost() { block(1); }
  inline void freeze() { block(2); }
  inline bool blocked() { return _blocked != 0; }
  inline bool frosted() { return _blocked == 1; }
  inline bool frozen() { return _blocked == 2; }

  list<item *> unpack(int limit);
  virtual list<item *> unpack1(int limit) = 0;

  inline const char *printname() { return _printname; }

 private:
  static class item_owner *_default_owner;

  static int _next_id;

  int _id;
  int _stamp;       // ascending order according to time of insertion to chart
                    // -1 for items not yet in the chart

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

  int _nparents;

  type_t *_qc_vector;

  int _p;
  
  tSM *_score_model;
  double _score;

  const char *_printname;

  int _blocked;
  list<item *> *_unpack_cache;

 public:
  list<item *> parents;
  list<item *> packed;

  friend class lex_item;
  friend class phrasal_item;
};

class lex_item : public item
{
 public:
  lex_item(int start, int end, const tPaths &paths,
           int ndtrs, int keydtr, class input_token **dtrs,
           int priority, fs &f, const char *name);

  ~lex_item() { delete[] _dtrs; }

  virtual lex_item &operator=(const item &li)
  {
    throw error("unexpected call to assignment operator of lex_item");
  }

  lex_item(const lex_item &li)
  {
    throw error("unexpected call to copy constructor of lex_item");
  }

  virtual void print(FILE *f, bool compact = false);
  virtual void print_family(FILE *f) {}
  virtual void print_derivation(FILE *f, bool quoted);
  virtual void print_yield(FILE *f);
  virtual string tsdb_derivation();

  virtual void getTagSequence(list<string> &tags, list<list<string> > &words);

  virtual void set_result_root(type_t rule);
  virtual void set_result_contrib() { _result_contrib = true; }

  virtual grammar_rule *rule();

  virtual fs get_fs(bool full = false)
  {
      return full ? _fs_full : _fs;
  }

  virtual void recreate_fs();

  string description();

  virtual inline int startposition() { return _dtrs[0]->startposition() ; }
  virtual inline int endposition() { 
    return _dtrs[_ndtrs - 1]->endposition() ; }

  virtual inline int age() { return _id; }

  inline const postags &get_in_postags()
  { return _dtrs[_keydtr]->get_in_postags(); }
  inline const postags &get_supplied_postags()
  { return _dtrs[_keydtr]->get_supplied_postags(); }

  bool synthesized() { return _dtrs[_keydtr]->synthesized(); }

  friend bool same_lexitems(const lex_item &a, const lex_item &b);
  
  void
  adjust_priority(int adjustment)
  { _p += adjustment; }
  
  void adjust_priority(const char *setting);

  virtual int identity()
  {
      return _dtrs[_keydtr]->identity();
  }

  virtual double score(tSM *);

  virtual list<item *> unpack1(int limit);

 private:
  int _ndtrs, _keydtr;
  class input_token **_dtrs;

  fs _fs_full; // unrestricted (packing) structure
};

class phrasal_item : public item
{
 public:
  phrasal_item(class grammar_rule *, class item *, fs &);
  phrasal_item(class phrasal_item *, class item *, fs &);
  phrasal_item(class phrasal_item *, vector<class item *> &, fs &);

  virtual void print(FILE *f, bool compact = false);
  virtual void print_family(FILE *f);
  virtual void print_derivation(FILE *f, bool quoted);
  virtual void print_yield(FILE *f);
  virtual string tsdb_derivation();

  virtual void getTagSequence(list<string> &tags, list<list<string> > &words);

  virtual void set_result_root(type_t rule);
  virtual void set_result_contrib() { _result_contrib = true; }

  virtual grammar_rule *rule();

  virtual void recreate_fs();

  virtual int startposition() { return _daughters.front()->startposition() ; }
  virtual int endposition() { return _daughters.back()->endposition() ; }

  virtual inline int age() { return _id; }

  virtual int identity()
  {
      if(_rule)
          return _rule->type();
      else
          return 0;
  }
  virtual double score(tSM *);

  virtual list<item *> unpack1(int limit);
  void unpack_cross(vector<list<item *> > &dtrs,
                    int index, vector<item *> &config,
                    list<item *> &res);
  item *unpack_combine(vector<item *> &config);

 private:
  list<item *> _daughters;
  item * _adaughter;
  grammar_rule *_rule;
};

class greater_than_score
{
public:
    greater_than_score(tSM *sm)
        : _sm(sm) {}

    bool operator() (item *i, item *j)
    {
        return i->score(_sm) > j->score(_sm);
    }
 private:
    tSM *_sm;
};

class item_owner
// allow proper release of memory
{
 public:
  item_owner() {}
  ~item_owner()
    {
      for(list<item *>::iterator curr = _list.begin(); 
	  curr != _list.end(); 
	  ++curr)
	delete *curr;
      item::reset_ids();
    }
  void add(item *it) { _list.push_back(it); }
 private:
  list<item *> _list;
};

#endif
