/* -*- Mode: C++ -*- */

#ifndef _MORPH_INNER_H
#define _MORPH_INNER_H
#include <QString>

/* ---------------------------------------------------------------------------
 "Inner" classes: don't look at them
 --------------------------------------------------------------------------- */

class StringCharacterIterator {
  QString _str;
  QString::iterator _it;
public:
  StringCharacterIterator(const QString& str) : _str(str) {
    _it = _str.begin();
  }
  bool hasNext() { return (_it != _str.end()) ; }
  QChar next32PostInc() { return *_it++; }
};

class Converter {
public:
  Converter() {} 
  std::string convert(const QString& s) { return s.toUtf8(); }
  QString convert(const std::string& s) { return QString::fromUtf8(s.c_str()); }
  QString convert(QChar c) { return QString(c); }
};

/// represents one letterset, primary purpose is keeping track of bindings
class morph_letterset
{
public:
  morph_letterset() :
    _bound(0) {};

    void set(const std::string& name, const std::string& elems);

      const std::set<QChar> &elems() const { return _elems; }

      void bind(QChar c) { _bound = c; }
      QChar bound() const { return _bound; }

  const std::string &name() const { return _name; }
  
  void print(std::ostream &) const;

private:
  std::string _name;
    std::set<QChar> _elems;
    QChar _bound;
};

// collection of morph_letterset
class morph_lettersets
{
public:
  morph_lettersets() {};

  void add(const std::string& s);
  morph_letterset * get(std::string name) const;

  void undo_bindings();

  void print(std::ostream &out) const;

private:
  typedef HASH_SPACE::unordered_map<std::string, morph_letterset>::iterator
    ml_iterator;
  typedef HASH_SPACE::unordered_map<std::string, morph_letterset>::const_iterator
    ml_const_iterator;

  HASH_SPACE::unordered_map<std::string, morph_letterset> _m;
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
                const QString& left, const QString& right)
    : _analyzer(a), _rule(rule), _left(left), _right(right) {}
  
  /** The AVM rule of this sub-rule */
  grammar_rule *rule() { return _rule; }

  /** Can the string \a matched + \a rest be reduced to a base form with this
   *  sub-rule?
   *  If this is the case, return the reduced form in \a result.
   */
  bool base_form(const QString& matched, const QString& rest,
                 QString &result);

  void print(std::ostream &) const;

private:
  tMorphAnalyzer *_analyzer;

  grammar_rule *_rule;
  
  QString _left;
  QString _right;

  bool establish_and_check_bindings(const QString& matched);
};



class trie_node {
  typedef std::map<QChar, trie_node *> tn_map;
  typedef tn_map::iterator tn_iterator;
  typedef tn_map::const_iterator tn_const_iterator;
  
public:
  trie_node() {}
  ~trie_node();

  bool has_subnode(QChar c) const { return _s.find(c) != _s.end(); }
  const class trie_node * const get_node(QChar c) const {
    tn_const_iterator it = _s.find(c);
    if (it == _s.end()) return NULL;
    return it->second;
  }

  void add_path(const QString& path, morph_subrule *rule, const morph_lettersets &ls);

  bool has_rules() const { return ! _rules.empty(); }
  const std::vector<morph_subrule *> &rules() const { return _rules; }

  void print(std::ostream &, int depth = 0) const;
  
private:
  inline trie_node * request_node(QChar c) {
    tn_const_iterator it = _s.find(c);
    if (it == _s.end()) _s[c] = new trie_node();
    return _s[c];
  }

  tn_map _s;

  std::vector<morph_subrule *> _rules;
};

class morph_trie
{
public:
  morph_trie(tMorphAnalyzer *a, bool suffix) :
    _analyzer(a), _suffix(suffix), _root()
  {};

  void add_subrule(grammar_rule *rule, std::string subrule);

  std::list<tMorphAnalysis> analyze(tMorphAnalysis a);

  void print(std::ostream &) const;

private:
  tMorphAnalyzer *_analyzer;
  bool _suffix;
  trie_node _root;
};

inline std::ostream &operator<<(std::ostream &out, const morph_lettersets &ml) {
  ml.print(out); return out;
}

inline std::ostream &operator<<(std::ostream &out, const morph_letterset &ml) {
  ml.print(out); return out;
}

inline std::ostream &operator<<(std::ostream &out, const morph_trie &mt) {
  mt.print(out); return out;
}

inline std::ostream &operator<<(std::ostream &out, const trie_node &tn) {
  tn.print(out); return out;
}

#endif
