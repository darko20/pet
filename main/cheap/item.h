/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* class `item' */

#ifndef _ITEM_H_
#define _ITEM_H_

#include <stdio.h>

#include <list>

#include "list-int.h"
#include "fs.h"

#include "tokenlist.h"
#include "grammar.h"

class item
{
 public:
  item(int start, int end, fs &f, char *name);
  item(int start, int end, char *name);

  virtual ~item();

  inline int id() { return _id; }

  inline int stamp() { return _stamp; }
  inline void stamp(int t) { _stamp = t; }
  inline bool in_chart() { return _stamp != -1; }

  inline bool passive() { return _tofill == 0; }
  inline int start() { return _start; }
  inline int end() { return _end; }

  inline bool shape_fits(class grammar_rule *R, int length)
    {
      return (R->left_extending() ? (_end + R->arity() - 1 <= length ) :
	      (_start - (R->arity() - 1) >= 0) ) ;
    }

  inline bool left_extending() { return _tofill == 0 || first(_tofill) == 1; }
  inline bool adjacent(class item *passive) // assumes `this' is an active item
    { return (left_extending() ? (_start == passive->_end) : (_end == passive->_start)); }

  inline bool root(class grammar *G, tokenlist *I, int &maxp)
    { if(_start == 0 && _end == I->length() - I->offset()) 
        return G->root(_fs, maxp); 
      else 
        return false; }

  inline fs get_fs()
    {
      if(_fs.temp() && _fs.temp() != unify_generation)
	recreate_fs();
      return _fs;
    }

  inline void set_fs(const fs &f) { _fs = f; }

  inline int nextarg() { return first(_tofill); }
  inline fs nextarg(fs &f) { return f.nth_arg(nextarg()); }
  inline list_int *restargs() { return rest(_tofill); }
  inline int arity() { return length(_tofill); }

  virtual void print(FILE *f, bool compact = false);
  virtual void print_derivation(FILE *f, bool quoted, int offset = 0) = 0;
#ifdef TSDBAPI
  virtual void tsdb_print_derivation(int offset = 0) = 0;
#endif

  inline bool result_root() { return _result_root; }
  inline bool result_contrib() { return _result_contrib; }

  virtual void set_result(bool root) = 0;
  
  inline type_t *qc_vector() { return _qc_vector; }

  virtual grammar_rule *rule() = 0;

  virtual void recreate_fs() = 0;
  inline void set_done(int t) { _done = t; }
  inline int done() { return _done; }

  inline int priority() { return _p; }
  inline int qriority() { return _q; }

#ifdef PACKING
  inline int frozen() { return _frozen; }
  inline void freeze(int mark) { _frozen = mark; }
#endif

  inline char *name() { return _name; }

 private:
  static int next_id;
  int _id;
  int _stamp; // ascending order according to time of insertion to chart
  // -1 for items not yet in the chart

  int _start, _end;

  fs _fs;

  list_int *_tofill;

  int _result_root : 1;
  int _result_contrib : 1;

  int _nparents;

  type_t *_qc_vector;

  int _p;
  int _q;

  char *_name;

  int _done;

#ifdef PACKING
  int _frozen;
#endif

 public:
  list<item *> parents;
  list<item *> packed;

  friend class item_priority_less;
  friend class lex_item;
  friend class generic_le_item;
  friend class phrasal_item;
};

class item_priority_less
{
 public:
  inline bool operator() (const item* x, const item* y) const
    {
      return x->_p < y->_p;
    }
};

class lex_item : public item
{
 public:
  lex_item(class lex_entry *, int pos, fs &, const postags &POS, int offset);

  virtual void print(FILE *f, bool compact = false);
  virtual void print_derivation(FILE *f, bool quoted, int offset = 0);
#ifdef TSDBAPI
  virtual void tsdb_print_derivation(int offset = 0);
#endif

  virtual void set_result(bool root);

  virtual grammar_rule *rule();

  void skew(int offset) { _start -= offset; _end -= offset; }

  virtual void recreate_fs();

  inline string description()
    { if(_le) return _le->description(); else return string(); }

  int offset() { return _offset; }

  string postag();

 private:
  class lex_entry *_le;
  int _offset;
  string _pos;
};

class generic_le_item : public item
{
 public:
  generic_le_item(int instance, int pos, string orth, const postags &POS);

  virtual void print(FILE *f, bool compact = false);
  virtual void print_derivation(FILE *f, bool quoted, int offset = 0);
#ifdef TSDBAPI
  virtual void tsdb_print_derivation(int offset = 0);
#endif

  virtual void set_result(bool root);

  virtual grammar_rule *rule();

  void skew(int offset) { _start -= offset; _end -= offset; }

  virtual void recreate_fs();

 private:
  void compute_pos_priority(const postags &POS);

  int _type;
  string _orth;
};

class phrasal_item : public item
{
 public:
  phrasal_item(class grammar_rule *, class item *, fs &);
  phrasal_item(class phrasal_item *, class item *, fs &);
  
  virtual void print(FILE *f, bool compact = false);
  virtual void print_derivation(FILE *f, bool quoted, int offset = 0);
#ifdef TSDBAPI
  virtual void tsdb_print_derivation(int offset = 0);
#endif

  virtual void set_result(bool root);

  virtual grammar_rule *rule();

  virtual void recreate_fs();

 private:
  list<item *> _daughters;
  item * _adaughter;
  grammar_rule *_rule;

};

class item_owner
// allow proper release of memory
{
 public:
  item_owner() {}
  ~item_owner()
    {
      for(list<item *>::iterator pos = _list.begin(); 
	  pos != _list.end(); 
	  ++pos)
	delete *pos;
    }
  void add(item *it) { _list.push_back(it); }
 private:
  list<item *> _list;
};

#endif
