/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* grammar rules, grammar, lexicon entries and lexicon */

#ifndef _GRAMMAR_H_
#define _GRAMMAR_H_

#include <stdio.h>

#include <list>
#include <map>

#include "list-int.h"
#include "types.h"
#include "fs.h"
#include "tokenlist.h"
#include "dumper.h"

class lex_entry
{
 public:

  lex_entry(dumper *f);
  lex_entry(const lex_entry &le);
  ~lex_entry();

  lex_entry & operator=(const lex_entry &le);

  bool matches0(int pos, tokenlist *input);
  bool matches(int pos, tokenlist *input);

  fs instantiate();

  inline char *name() { return typenames[_base]; }
  inline const char *affixname() { 
    return _affix == -1 ? "n/a" : typenames[_affix]; 
  }

  inline int basetype() { return _base; }

  inline int offset() { return _inflpos; }
  inline int length() { return _nwords; }

  inline const char *key() { return _orth[_inflpos]; }
  
  void print_derivation(FILE *f, bool quoted, int start, int end, int id = -1);
  void tsdb_print_derivation(int start, int end, int id = -1);
  void print(FILE *f);

  inline int priority() { return _p; }

  inline string description() { return string(name()) + "[" + string(affixname()) + "]"; } 

 private:
  static int next_id;
  static char *affix_path;

  int _id;

  int _nwords;
  char **_orth;

  int _base;   // type index
  int _affix;  // type index

  int _inflpos;

  int _p;

  bool _invalid;

  friend class grammar;
};

class grammar_rule
{
 public:
  grammar_rule(dumper *f);

  inline int arity() { return _arity; }
  inline int nextarg() { return first(_tofill); }
  inline bool left_extending() { return first(_tofill) == 1; }

  inline int type() { return _type; }
  inline char *name() { return typenames[_type]; }
  inline int id() { return _id; }

  void print(FILE *f);

  fs instantiate();

  inline fs nextarg(fs &f) { return f.nth_arg(first(_tofill)); }
  inline list_int *restargs() { return rest(_tofill); }

  inline type_t *qc_vector(int arg) { return _qc_vector[arg - 1]; }

  inline int priority() { return _p; }
  inline bool hyperactive() { return _hyper; }

  int actives, passives;

 private:
  static int next_id;

  int _id;
  int _type;  // type index
  int _arity;
  list_int *_tofill;

  type_t **_qc_vector;
  void init_qc_vector();

  int _p; // priority

  bool _hyper;

  friend class grammar;
};

struct grammar_info
{
  char *version;
  char *vocabulary;
  char *ntypes;
  char *ntemplates;
};

class grammar
{
 public:
  grammar(const char *filename);
  ~grammar();

  bool root(fs &, int &maxp);

  list_int *deleted_daughters() { return _deleted_daughters; }

  inline bool filter_compatible(grammar_rule *mother, int arg, grammar_rule *daughter)
    { if(daughter == NULL) return true;
      return _filter[daughter->id() + _nrules * mother->id()] & (1 << (arg - 1)); }

  inline grammar_info &info() { return _info; }
  inline int nlexentries() { return _lexicon.size(); }
  inline int nrules() { return _rules.size(); }
  int nhyperrules();

 private:
  list<lex_entry *> _lexicon;
  multimap<const string, lex_entry *> _dict_lexicon;

  int _nrules;
  list<grammar_rule *> _rules;

  list_int *_root_insts;
  map<int, int> _root_weight;

  fs _root;

  fs_factory *_fsf;

  char *_filter;
  void initialize_filter();

  void init_rule_qc();
  int _qc_inst;

  list_int *_deleted_daughters;

  struct grammar_info _info;
  void init_grammar_info();
  void init_parameters();

  friend class le_iter;
  friend class rule_iter;
};

class le_iter
{
 public:
  inline le_iter(grammar *G, int pos, tokenlist *input)
    : _pos(pos), _input(input)
    {
      _range = G->_dict_lexicon.equal_range((*input)[pos]);
      _curr = _range.first;
      filter();
    }

  inline le_iter &operator++(int)
    {
      ++_curr; filter(); return *this;
    }

  inline bool valid()
    {
      return _curr != _range.second;
    }
  
  inline lex_entry *current()
    {
      if(valid()) return _curr->second; else return 0;
    }

 private:
  int _pos;
  tokenlist *_input;

  typedef multimap<const string, lex_entry *>::iterator _le_iter;
  _le_iter _curr;
  pair<_le_iter, _le_iter> _range;

  inline void filter()
    { 
      while(valid() && !_curr->second->matches0(_pos, _input))
	++_curr;
    }
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
