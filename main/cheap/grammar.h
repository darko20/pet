/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* grammar rules, grammar, stems and lexicon */

#ifndef _GRAMMAR_H_
#define _GRAMMAR_H_

#include "list-int.h"
#include "../common/utility.h"
#include "types.h"
#include "fs.h"

// global variables for quick check
extern int qc_len;
extern qc_node *qc_paths;

class lex_stem
{
 public:

  lex_stem(type_t t);
  lex_stem(const lex_stem &le);
  ~lex_stem();

  lex_stem & operator=(const lex_stem &le);

  fs instantiate();

  inline char *name() const { return typenames[_type]; }
  inline char *printname() const { return printnames[_type]; }

  inline int type() const { return _type; }
  inline int length() const { return _nwords; }
  inline int inflpos() const { return _nwords - 1; }
  inline int priority() const { return _p; }

  inline const char *orth(int i) const { return _orth[i]; }
  
  void print(FILE *f) const;

 private:
  static int next_id;

  int _id;

  int _type;        // type index

  int _nwords;      // length of _orth
  char **_orth;     // array of _nwords strings

  int _p;           // priority

  vector<string> get_stems();

  friend class grammar;
};

class full_form
{
 public:
  full_form() :
    _stem(0), _affixes(0), _offset(0) {};

  full_form(lex_stem *st, modlist mods = modlist(), list_int *affixes = 0)
    : _stem(st), _affixes(affixes), _offset(0), _mods(mods) {};

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

  inline int priority()
  {
    if(valid()) return _stem->priority(); else return 0;
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
  static char *_affix_path;

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

  fs instantiate();

  inline fs nextarg(fs &f) { return f.nth_arg(first(_tofill)); }
  inline list_int *restargs() { return rest(_tofill); }

  inline type_t *qc_vector(int arg) { return _qc_vector[arg - 1]; }

  inline int priority(int def) { return _p == 0 ? def : _p ; }
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

  type_t **_qc_vector;
  void init_qc_vector();

  int _p;           // priority

  bool _hyper;
  bool _spanningonly;

  friend class grammar;
};

struct grammar_info
{
  char *version;
  char *ntypes;
  char *ntemplates;
};

class grammar
{
 public:
  grammar(const char *filename);
  ~grammar();

  bool root(fs &, type_t &rule, int &maxp);

  list_int *deleted_daughters() { return _deleted_daughters; }

  inline bool filter_compatible(grammar_rule *mother, int arg,
				grammar_rule *daughter)
    {
      if(daughter == NULL) return true;
      return _filter[daughter->id() + _nrules * mother->id()] &
	(1 << (arg - 1));
    }

  inline grammar_info &info() { return _info; }
  inline int nrules() { return _rules.size(); }
  int nhyperrules();

  inline int nstems() { return _lexicon.size(); }
  lex_stem *lookup_stem(int inst_key);
  list<lex_stem *> lookup_stem(string s);

  list<full_form> lookup_form(const string form);

  list_int *generics() { return _generics; }

  bool punctuationp(const string &s);

#ifdef ONLINEMORPH
  class morph_analyzer *morph() { return _morph; }
#endif
  
 private:
#ifndef ICU
  string _punctuation_characters;
#else
  UnicodeString _punctuation_characters;
#endif

  map<type_t, lex_stem *> _lexicon;
  multimap<string, lex_stem *> _stemlexicon;
  
  typedef multimap<string, full_form *> ffdict;
  ffdict _fullforms;

#ifdef ONLINEMORPH
  class morph_analyzer *_morph;
#endif

  int _nrules;
  list<grammar_rule *> _rules;

  bool _weighted_roots;
  list_int *_root_insts;
  map<type_t, int> _root_weight;

  list_int *_generics;

  char *_filter;
  void initialize_filter();

  void init_rule_qc();
  int _qc_inst;

  list_int *_deleted_daughters;

  struct grammar_info _info;
  void init_grammar_info();
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
