/* -*- Mode: C++ -*- */

#ifndef _MORPH_INNER_H
#define _MORPH_INNER_H

/* ---------------------------------------------------------------------------
 "Inner" classes: don't look at them
 --------------------------------------------------------------------------- */

#ifdef HAVE_ICU
#include "unicode.h"
typedef UChar32 MChar;
typedef UnicodeString MString;
#else
// fake classes to make it possible to use cheap without ICU
typedef char MChar;
// typedef string MString;
class MString {
  friend class StringCharacterIterator;
  std::string _str;
public:
  MString() {}
  MString(const std::string &s) : _str(s) {} 
  void remove(unsigned int start = 0, unsigned int len = std::string::npos) {
    _str.erase(start, len);
  }
  std::string str() { return _str; }
  MChar char32At(int index) { return _str[index]; }
  int getChar32Start(int offset) { return offset; }
  void reverse() { std::reverse(_str.begin(), _str.end()); }
  void reverse(int start, int length) {
    std::reverse(_str.begin() + start, _str.begin() + start + length); 
  }
  int indexOf(MChar c, int offset) { return _str.find(c, offset); }
  int indexOf(const MString s) { return _str.find(s._str); }
  void append(MChar c) { _str += c ; }
  void append(const MString s) { _str += s._str; }
  const char *c_str() { return _str.c_str(); }
  int length() { return _str.length(); }
};

class StringCharacterIterator {
  std::string &_str;
  std::string::iterator _it;
public:
  StringCharacterIterator(MString str) : _str(str._str) {
    _it = _str.begin();
  }
  bool hasNext() { return (_it != _str.end()) ; }
  MChar next32PostInc() { return *_it++; }
};

class Converter {
public:
  Converter() {} 
  std::string convert(MString s) { return s.str() ; }
  MString convert(std::string s) { return s ; }
  MString convert(MChar c) { return std::string() + c ; }
};

#endif

/// represents one letterset, primary purpose is keeping track of bindings
class morph_letterset
{
public:
  morph_letterset() :
    _bound(0) {};

    void set(std::string name, std::string elems);

  const std::set<MChar> &elems() const { return _elems; }

  void bind(MChar c) { _bound = c; }
  MChar bound() const { return _bound; }

  const std::string &name() const { return _name; }
  
  void print(FILE *f) const;

private:
  std::string _name;
  std::set<MChar> _elems;
  MChar _bound;
};

// collection of morph_letterset
class morph_lettersets
{
public:
  morph_lettersets() {};

  void add(std::string s);
  morph_letterset * get(std::string name) const;

  void undo_bindings();

  void print(FILE *f) const;

private:
  typedef hash_map<std::string, morph_letterset>::iterator ml_iterator;
  typedef hash_map<std::string, 
                   morph_letterset>::const_iterator ml_const_iterator;

  hash_map<std::string, morph_letterset> _m;
};

/** This implements one element of a morphological rule specification, the
 * so-called sub-rule. 
 *
 * It consists of the left part (the part to be substitued) and right part (the
 * substitution). During analysis, the roles are switched: the right part has
 * to be found in the string and is then replaced by the left part to come
 * closer to the base form.
 */
class morph_subrule
{
public:
  morph_subrule(class tMorphAnalyzer *a, class grammar_rule *rule,
                MString left, MString right)
    : _analyzer(a), _rule(rule), _left(left), _right(right) {}
  
  /** The AVM rule of this sub-rule */
  grammar_rule *rule() { return _rule; }

  /** Can the string \a matched + \a rest be reduced to a base form with this
   *  sub-rule?
   *  If this is the case, return the reduced form in \a result.
   */
  bool base_form(MString matched, MString rest,
                 MString &result);

  void print(FILE *f);

private:
  tMorphAnalyzer *_analyzer;

  grammar_rule *_rule;
  
  MString _left, _right;

  bool establish_and_check_bindings(MString matched);
};



class trie_node {
  typedef std::map<MChar, trie_node *> tn_map;
  typedef tn_map::iterator tn_iterator;
  typedef tn_map::const_iterator tn_const_iterator;
  
public:
  trie_node() {}
  ~trie_node();

  bool has_subnode(MChar c) const { return _s.find(c) != _s.end(); }
  const class trie_node * const get_node(MChar c) const {
    tn_const_iterator it = _s.find(c);
    if (it == _s.end()) return NULL;
    return it->second;
  }

  void add_path(MString path, morph_subrule *rule, const morph_lettersets &ls);

  bool has_rules() const { return ! _rules.empty(); }
  const vector<morph_subrule *> &rules() const { return _rules; }

  void print(FILE *f, int depth = 0) const;
  
private:
  inline trie_node * request_node(MChar c) {
    tn_const_iterator it = _s.find(c);
    if (it == _s.end()) _s[c] = new trie_node();
    return _s[c];
  }

  tn_map _s;

  vector<morph_subrule *> _rules;
};

class morph_trie
{
public:
  morph_trie(tMorphAnalyzer *a, bool suffix) :
    _analyzer(a), _suffix(suffix), _root()
  {};

  void add_subrule(grammar_rule *rule, std::string subrule);

  std::list<tMorphAnalysis> analyze(tMorphAnalysis a);

  void print(FILE *f);

private:
  tMorphAnalyzer *_analyzer;
  bool _suffix;
  trie_node _root;
};



#endif
