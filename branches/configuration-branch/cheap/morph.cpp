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

/* implementation of LKB style morphological analysis and generation */

#include "pet-config.h"
#include "morph.h"
#ifdef HAVE_ICU
#include "unicode.h"
typedef UChar32 MChar;
typedef UnicodeString MString;
#else
typedef char MChar;
// typedef string MString;

class MString {
  friend class StringCharacterIterator;
  string _str;
public:
  MString() {}
  MString(const string &s) : _str(s) {} 
  void remove(unsigned int start = 0, unsigned int len = string::npos) {
    _str.erase(start, len);
  }
  string str() { return _str; }
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
  string &_str;
  string::iterator _it;
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
  string convert(MString s) { return s.str() ; }
  MString convert(string s) { return s ; }
  MString convert(MChar c) { return string() + c ; }
};

Converter Convert;
Converter *Conv = &Convert;

#endif

#include "cheap.h"
#include "grammar-dump.h"

#define LETTERSET_CHAR ((MChar) '\x8')

bool operator==(const tMorphAnalysis &a, const tMorphAnalysis &b) {
  if (a.base() != b.base()) return false;
  list<type_t>::const_iterator ar, ae, br, be;
  ar = a._rules.begin();
  ae = a._rules.end();
  br = b._rules.begin();
  be = b._rules.end();
  while (ar != ae) {
    if ((br == be) || (*ar != *br)) return false;
    ar++; 
    br++;
  }
  if (br != be) return false;
  return true;
}


void erase_duplicates(list<tMorphAnalysis> &li) {
  li.sort(tMorphAnalysis::less_than());
  // filter duplicates
  list<tMorphAnalysis>::iterator new_last
    = unique(li.begin(), li.end(), tMorphAnalysis::equal());
  li.erase(new_last, li.end());
}


//
// utility functions
//

string morph_unescape_string(const string &s) {

  string res = "";

  for(string::size_type i = 0; i < s.length(); i++) {
    if(s[i] != '\\')
      //
      // move magic letter set character from |!| to |\x8|, something that can
      // otherwise not appear in rule strings.
      //
      if(s[i] == '!') res += '\x8';
      else res += s[i];
    else {
      i++;
      if(i >= s.length())
        return res;
      switch(s[i]) {
      case '!':
        res += "!";
        break;
      case '?':
        res += "?";
        break;
      case '*':
        res += "*";
        break;
      case ')':
        res += ")";
        break;
      case '\\':
        res += "\\";
        break;
      default:
        res += s[i];
        break;
      } // switch
    } // else
  } // for

  return res;

} // morph_unescape_string()


/** Return the substring which is enclosed in parentheses after position \a
 *  start in \a s.
 *  Escaped closing parentheses are enclosed in the result and do not count as
 *  terminating parenthesis.
 * 
 *  \attention { _fix_me_
 * these strings come straight from undumping, i.e. are not converted; hence,
 * we would have to do UniCode conversion here, prior to parsing the string:
 * for `unsafe' encodings, part of a double-byte character may appear to be
 * an opening or closing paren.  while the active multi-byte encodings are 
 * UTF-8 and EUC-JP, this will not be an issue in practice.   (29-oct-05; oe)}
 *
 * \return the substring and the position that after the subrule in \a stop
 */
string get_next_subrule(string &s, 
                        string::size_type start,
                        string::size_type &stop,
                        bool letterset = false) {

  stop = start;
  //
  // first, find opening paren (the caller has already confirmed and stripped 
  // the magic strings `prefix', `suffix' or `letter-set'
  //
  string::size_type open = s.find("(", start);
  if(open == string::npos || ++open == s.length()) return string();

  //
  // now, extract the subrule or letterset, bounded by a non-escaped closing
  // paren
  //
  string::size_type close;
  bool escapep = false;
  for(close = open; close < s.length(); ++close) {
    if(s[close] == '\\') escapep = true;
    else {
      if(s[close] == ')' && !escapep) break;
      escapep = false;
    } // else
  } //for
  
  if(s[close] != ')') 
    throw tError(string("invalid ")
                 + (letterset ? "letter set" : "orthographemic subrule" )
                 + " |" + s.substr(open) +"|");

  stop = close;
  return s.substr(open, close - open);

} // get_next_subrule()


string get_next_letter_set(string &s, 
                           string::size_type start,
                           string::size_type &stop) {

  //
  // _fix_me_
  // these strings come straight from undumping, i.e. are not converted; hence,
  // we would have to do UniCode conversion here, prior to parsing the string:
  // for `unsafe' encodings, part of a double-byte character may appear to be
  // an opening or closing paren.  while the active multi-byte encodings are 
  // UTF-8 and EUC-JP, this will not be an issue in practice.   (29-oct-05; oe)
  //

  stop = start;
  //
  // first, find opening paren and confirm magic string `letter-set'
  //
  string::size_type open = s.find("(", start);
  if(open == string::npos || ++open == s.length()) return string();
  if(s.compare(open, 10, "letter-set") != 0) return string();

  return get_next_subrule(s, open, stop, true);

} // get_next_letter_set()


// This is the old function which has been replaced by the ones above
//
// return next list (stuff between (balanced) parantheses) in s, starting at
// start; the position of the closing paren (relative to s) is returned in
// stop
string get_next_list(string &s, string::size_type start,
                     string::size_type &stop)
{
  stop = start;

  string::size_type openp = s.find("(", start);
  if(openp == string::npos)
    return string();

  int plevel = 0;
  string::size_type closep;
  for(closep = openp; closep < s.length(); ++closep)
  {
    if(s[closep] == '(') plevel++;
    else if(s[closep] == ')') plevel--;
    if(plevel == 0) break;
  }
  
  if(plevel != 0) throw tError("unbalanced list");

  stop = closep;
  return s.substr(openp+1, closep-openp-1);
}

void print_analyses(FILE *f, list<tMorphAnalysis> res)
{
  int id = 0;
  for(list<tMorphAnalysis>::iterator it = res.begin(); it != res.end(); ++it)
  {
    fprintf(f, "[%d]: ", id++);
    it->print(f);
    fprintf(f, "\n");
  }
}

//
// internal classes: interface
//

/// represents one letterset, primary purpose is keeping track of bindings
class morph_letterset
{
public:
  morph_letterset() :
    _bound(0) {};

  morph_letterset(string name, string elems);

  const set<MChar> &elems() { return _elems; }

  void bind(MChar c) { _bound = c; }
  MChar bound() { return _bound; }

  const string &name() const { return _name; }
  
  void print(FILE *f);

private:
  string _name;
  set<MChar> _elems;
  MChar _bound;
};

// collection of morph_letterset's
class morph_lettersets
{
public:
  morph_lettersets() {};

  void add(string s);
  morph_letterset *get(string name);

  void undo_bindings();

  void print(FILE *f);

private:

  map<string, morph_letterset*> _m;
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
  morph_subrule(tMorphAnalyzer *a, type_t rule,
                MString left, MString right)
    : _analyzer(a), _rule(rule), _left(left), _right(right) {}
  
  /** The AVM rule of this sub-rule */
  type_t rule() { return _rule; }

  /** Can the string \a matched + \a rest be reduced to a base form with this
   *  sub-rule?
   *  If this is the case, return the reduced form in \a result.
   */
  bool base_form(MString matched, MString rest,
                 MString &result);

  void print(FILE *f);

private:
  tMorphAnalyzer *_analyzer;

  type_t _rule;
  
  MString _left, _right;

  bool establish_and_check_bindings(MString matched);
};

class trie_node
{
public:
  trie_node(tMorphAnalyzer *a) :
    _analyzer(a)
  {};

  trie_node *get_node(MChar c, bool add = false);

  void add_path(MString path, morph_subrule *rule);

  bool has_rules() { return _rules.size() > 0; }
  vector<morph_subrule *> &rules() { return _rules; }

  void print(FILE *f, int depth = 0);
  
private:
  tMorphAnalyzer *_analyzer;

  map<MChar, trie_node *> _s;

  vector<morph_subrule *> _rules;
};

class morph_trie
{
public:
  morph_trie(tMorphAnalyzer *a, bool suffix) :
    _analyzer(a), _suffix(suffix), _root(_analyzer)
  {};

  void add_subrule(type_t rule, string subrule);

  list<tMorphAnalysis> analyze(tMorphAnalysis a);

  void print(FILE *f);

private:
  tMorphAnalyzer *_analyzer;
  bool _suffix;
  trie_node _root;
};

//
// Internal classes: implementation
//

//
// Letterset
//

morph_letterset::morph_letterset(string name, string elems_u8) :
  _name(name), _bound(0)
{
  MString elems = Conv->convert(elems_u8);

  StringCharacterIterator it(elems);

  MChar c;
  bool escapep = false;
  while(it.hasNext()) {
    c = it.next32PostInc();
    if(c == '\\' && !escapep) 
      escapep = true;
    else {
      _elems.insert(c);
      escapep = false;
    } // else
  } // while
}

void morph_letterset::print(FILE *f)
{
  fprintf(f, "!%s -> ", _name.c_str());

  for(set<MChar>::iterator it = _elems.begin(); it != _elems.end(); ++it)
    fprintf(f, "%s", Conv->convert(*it).c_str());

  fprintf(f, " [%s]", Conv->convert(_bound).c_str());
}

//
// Lettersets (collection)
//

void morph_lettersets::add(string s)
{
  LOG(loggerUncategorized, Level::DEBUG,"LETTERSET: <%s>", s.c_str());

  // s consists of the name and the characters making up the set
  // seperated by whitespace
  
  unsigned int p = 0;
  while(!isspace(s[p]) && p < s.length()) p++;
  if(p < s.length())
  {
    string name = s.substr(1, p - 1);
    while(isspace(s[p]) && p < s.length()) p++;
    if(p < s.length())
    {
      string elems = s.substr(p, s.length() - p); 
      LOG(loggerUncategorized, Level::DEBUG,
          " -> <%s> <%s>", name.c_str(), elems.c_str());
      morph_letterset *ls = new morph_letterset(name, elems);
      _m[name] = ls;
    }
    else
    {
      LOG(loggerUncategorized, Level::INFO,
          "Ignoring ill-formed letterset definition: %s",
          s.c_str());
    }
  }
  else
  {
    LOG(loggerUncategorized, Level::INFO,
        "Ignoring ill-formed letterset definition: %s",
        s.c_str());
  }
}

morph_letterset *morph_lettersets::get(string name)
{
  map<string, morph_letterset*>::iterator pos = _m.find(name);

  if(pos == _m.end())
    return 0;
  
  return pos->second;
}

void morph_lettersets::undo_bindings()
{
  for(map<string, morph_letterset*>::iterator it = _m.begin();
      it != _m.end(); ++it)
    it->second->bind(0);
}

void morph_lettersets::print(FILE *f)
{
  fprintf(f, "--- morph_lettersets[%x]:\n", (size_t) this);
  for(map<string, morph_letterset*>::iterator it = _m.begin();
      it != _m.end(); ++it)
  {
    fprintf(f, "  ");
    it->second->print(f);
    fprintf(f, "\n");
  }
}

//
// morph subrule
//

bool morph_subrule::establish_and_check_bindings(MString matched)
{
  // Iterate over the right side of the morphological sub-rule: The part to be
  // replaced during analysis.
  StringCharacterIterator it1(_right);
  StringCharacterIterator it2(matched);
  MChar c1, c2;

  /* Reset all bindings */
  _analyzer->undo_letterset_bindings();

  while(it1.hasNext())
  {
    c1 = it1.next32PostInc();
    c2 = it2.next32PostInc();
    if(c1 == LETTERSET_CHAR)
    {
      // get the appropriate letterset for the character following the
      // initiator
      c1 = it1.next32PostInc();
      morph_letterset *ls = _analyzer->letterset(string(1, (char) c1));
      if(ls == 0)
        throw tError("Referencing undefined letterset !" + string(1, (char) c1));
      // Check if letterset is either unbound (first occurence) or the
      // character is the same, as required 
      if(ls->bound() == 0 || ls->bound() == c2)
        ls->bind(c2);
      else
        return false;
    }
    else
    {
      if(c1 != c2)
        throw tError("Conception error in morphology");
    }
  }
  return true;
}

/** Can this rule reduce the string consisting of \a matched and \a rest to a
 * valid base form ?
 * If so, return the reduced form in \a result.
 */
bool morph_subrule::base_form(MString matched,
                              MString rest,
                              MString &result)
{
  // Check the co-occurence restrictions introduced by the lettersets on the
  // matched part
  if(establish_and_check_bindings(matched) == false)
    return false;

  // build base form by replacing matched string with the left part of the
  // subrule and translating references to lettersets

  result.remove();

  StringCharacterIterator it(_left);

  MChar c;
  while(it.hasNext())
  {
    c = it.next32PostInc();
    if(c == LETTERSET_CHAR)
    {
      c = it.next32PostInc();
      morph_letterset *ls = _analyzer->letterset(string(1, (char) c));
      if(ls == 0)
        throw tError("Referencing undefined letterset !" + string(1, (char) c));
      result.append(ls->bound());
    }
    else
      result.append(c);
  }

  result.append(rest);

  return true;
}

void morph_subrule::print(FILE *f)
{
  fprintf(f, "morph_subrule[%x] %s: %s->%s",
          (size_t) this, print_name(_rule),
          Conv->convert(_left).c_str(), Conv->convert(_right).c_str());
}

//
// trie node
//

trie_node *trie_node::get_node(MChar c, bool add)
{
  map<MChar, trie_node *>::iterator it = _s.find(c);

  if(it == _s.end())
  {
    if(add) 
    {
      trie_node *n = new trie_node(_analyzer);
      _s[c] = n;
      return n;
    }
    else
      return 0;
  }
  else
    return it->second;
}

void trie_node::add_path(MString path, morph_subrule *rule)
{
  if(path.length() == 0)
  {
    // base case, add rule to this node
    _rules.push_back(rule);
  }
  else
  {
    // recursive case, treat first element in path
    if(path.char32At(0) != LETTERSET_CHAR)
    {
      // it's not a letterset
      MChar c = path.char32At(0);
      MString rest(path);
      rest.remove(0, 1);

      trie_node *n = get_node(c, true);
      n->add_path(rest, rule);
    }
    else
    {
      // it's a letterset
      MChar c = path.char32At(1);
      MString rest(path);
      rest.remove(0, 2);

      string lsname = string(1, (char) c);
      morph_letterset *ls = _analyzer->letterset(lsname);

      if(ls == 0)
        throw tError("Referencing undefined letterset !" + lsname);

      const set<MChar> &elems = ls->elems();
      for(set<MChar>::const_iterator it = elems.begin();
          it != elems.end(); ++it)
      {
        trie_node *n = get_node(*it, true);
        n->add_path(rest, rule);
      }
    }
  }
}

void trie_node::print(FILE *f, int depth)
{
  fprintf(f, "%*s", depth, "");

  fprintf(f, "trie[%x] (", (size_t) this);
  
  for(vector<morph_subrule *>::iterator it = _rules.begin();
      it != _rules.end(); ++it)
  {
    (*it)->print(f);
    fprintf(f, " ");
  }
  fprintf(f, ")\n");

  for(map<MChar, trie_node *>::iterator it = _s.begin();
      it != _s.end(); ++it)
  {
    fprintf(f, "%*s", depth, "");

    fprintf(f, "[%s]\n", Conv->convert(it->first).c_str());

    it->second->print(f, depth+2);
  }
}

//
// Trie
//

void reverse_subrule(MString &s)
  // basically just a reverse, but restoring !-sequences 
{
  s.reverse();
  
  int32_t off = 0;
  while((off = s.indexOf((MChar) LETTERSET_CHAR, off)) != -1)
    s.reverse(s.getChar32Start(off-1), 2);
}

void morph_trie::add_subrule(type_t rule, string subrule)
{
  string left_u8, right_u8;

  string::size_type curr = 0;
  while(curr < subrule.length() && !isspace(subrule[curr])) ++curr;

  if(curr == subrule.length())
    throw tError("Invalid subrule `" + subrule + "' in rule " + print_name(rule));

  left_u8 = subrule.substr(0, curr);
  if(left_u8 == "*")
    left_u8 = "";

  // strip blanks at the beginning of the left part of a morph subrule
  while(curr < subrule.length() && isspace(subrule[curr])) ++curr;

  // strip blanks at the end of the left part of a morph subrule
  string::size_type rule_end = subrule.length() - 1;
  while(rule_end >= curr && isspace(subrule[rule_end])) --rule_end;

  if(curr > rule_end)
    throw tError("Invalid subrule `" + subrule + "' in rule " + print_name(rule));

  right_u8 = subrule.substr(curr, rule_end - curr + 1);

  MString left = Conv->convert(left_u8);
  MString right = Conv->convert(right_u8);

  if(_suffix)
  {
    reverse_subrule(left);
    reverse_subrule(right);
  }

  LOG(loggerUncategorized, Level::DEBUG, "INFLSUBRULE<%s>: %s (`%s' -> `%s')",
      print_name(rule), subrule.c_str(),
      Conv->convert(left).c_str(), Conv->convert(right).c_str());

  morph_subrule *sr = new morph_subrule(_analyzer, rule, left, right);
  _analyzer->add_subrule(sr);
  _root.add_path(right, sr);

}

/** Remove all possible prefixes (or suffixes) encoded in this trie from the
 *  base string in \a a and return the list of newly created analyses.
 */
list<tMorphAnalysis> morph_trie::analyze(tMorphAnalysis a)
{
  list<tMorphAnalysis> res;

  MString s = Conv->convert(a.base());
  if(_suffix) s.reverse();

  MString matched;

  trie_node *n = &_root;

  while(s.length() > 0)
  {
    MChar c = s.char32At(0);
    s.remove(0, 1);
    matched.append(c);

    // is there a branch at this trie node labeled with character c?
    // If not so, we are done
    n = n->get_node(c);
    if(n == 0) return res;

    // Iterate through all subrules that have been matched completedly when
    // arriving at the current trie node.
    for(vector<morph_subrule *>::iterator r = n->rules().begin();
        r != n->rules().end(); ++r)
    {
      MString base;
      // Can the rule reduce the string "matched + s" to a valid base form ?
      if((*r)->base_form(matched, s, base) == false)
        continue;

      type_t candidate = (*r)->rule();
      list<type_t> rules = a.rules();

      // Check the rule filter (if activated) with candidate as daughter rule
      // and the first rule in the 'rules' list as mother.
      if (_analyzer->_rule_filter && ! rules.empty()
          && ! Grammar->filter_compatible(rules.front(), 1, candidate))
        continue;

      if(_suffix) base.reverse();

      string st = Conv->convert(base);

      //
      // in case the `orthographemics-minimum-stem-length' parameter was used
      // to impose a lower bound on how much to reduce the input form, see to
      // it that hypotheses violating that constraint cannot get into .res.;
      // given substitution and addition rules (plus variation among subrues),
      // i am not quite sure whether we could maybe save work by moving this
      // test further up; i think not.                         (12-feb-05; oe) 
      //
      if(_analyzer->_minimal_stem_length > 0
         && st.size() < _analyzer->_minimal_stem_length) 
        continue;

      list<string> forms = a.forms();
      list<type_t>::iterator rule;
      list<string>::iterator form;

      //
      // now test for potentially cyclic rule applications: if this rule
      // has been used on the same form before, re-applying it here must
      // take us into a cyclic derivation.  ERB assures us, it is fully
      // undesirable anyway, not even for German or Japanese.
      //
      bool cyclep = false;
      for(rule = rules.begin(), form = forms.begin();
          rule != rules.end() && form != forms.end();
          ++rule, ++form) 
      {
        if(*rule == candidate 
           && (_analyzer->_duplicate_filter_p || *form == st))
        {
          cyclep = true;
          break;
        }
      }
      if(cyclep) continue;

      rules.push_front(candidate);
      forms.push_front(st);
	  
      res.push_back(tMorphAnalysis(forms, rules));
    }
  }

  return res;
}

void morph_trie::print(FILE *f)
{
  fprintf(f, "--- morph_trie[%x] (%s)\n", (size_t) this,
          _suffix ? "suffix" : "prefix");
  _root.print(f);
}

//
// Exported classes
//

//
// Analysis
//

void tMorphAnalysis::print(FILE *f) const
{
    fprintf(f, "%s = %s", complex().c_str(), base().c_str());

    for(list<type_t>::const_iterator it = _rules.begin()
          ; it != _rules.end(); ++it)
        fprintf(f, " + %s", print_name(*it));
}

void tMorphAnalysis::print_lkb(FILE *f)
{
  list<string>::iterator form = _forms.begin();

  fprintf(f, "(%s", form->c_str());
  ++form;

  list<type_t>::iterator rule = _rules.begin(); 

  while(form != _forms.end() && rule != _rules.end())
  {
    fprintf(f, " (%s %s)", print_name(*rule), form->c_str());
    ++form; ++rule;
  }

  fprintf(f, ")");
}

//
// Analyzer
//

tMorphAnalyzer::tMorphAnalyzer()
  : _lettersets(new morph_lettersets),
    _suffixrules(new morph_trie(this, true)),
    _prefixrules(new morph_trie(this, false)),
    _irregs_only(false),
    _duplicate_filter_p(false), _maximal_depth(0), _minimal_stem_length(0),
    _rule_filter(false)
{
  char *foo;
  if((foo = cheap_settings->value("orthographemics-maximum-chain-depth")) != 0)
    _maximal_depth 
      = strtoint(foo, "as value of orthographemics-maximum-chain-depth");
  if((foo = cheap_settings->value("orthographemics-minimum-stem-length")) != 0)
    _minimal_stem_length 
      = strtoint(foo, "as value of orthographemics-minimum-stem-length");
  if(cheap_settings->lookup("orthographemics-duplicate-filter"))
    _duplicate_filter_p = true;
  if(cheap_settings->lookup("orthographemics-cohesive-chains") != NULL)
    _rule_filter = true;
}

tMorphAnalyzer::~tMorphAnalyzer()
{
  for(vector<morph_subrule *>::iterator it = _subrules.begin();
      it != _subrules.end(); ++it)
    delete *it;

  for(multimap<string, tMorphAnalysis *>::iterator it =
        _irregs_by_stem.begin(); it != _irregs_by_stem.end(); ++it)
    delete it->second;

  delete _prefixrules;
  delete _suffixrules;
  delete _lettersets;
}

void tMorphAnalyzer::add_global(string rule) {
  if(verbosity > 9)
    LOG(loggerUncategorized, Level::INFO,
        "INFLR<global>: %s", rule.c_str());

  string::size_type start, stop;
  
  start = 0; stop = 1;
  while(start != stop) {

    string ls = get_next_letter_set(rule, start, stop);
    if(start != stop) {
      _lettersets->add(ls);
      start = stop; stop = 1;
    } // if
    else {
      //
      // ignore remaining trailing parens when looking at the final element
      //
      if(rule.compare(start, 2, "))") != 0) {
        string s = rule.substr(start);
        LOG(loggerUncategorized, Level::INFO,
            "ignoring remaining letter-set(s) <%s>",
            s.c_str());
      } // if 
    } // else
  } // while

} // tMorphAnalyzer::add_global()


void tMorphAnalyzer::parse_rule(type_t t, string rule, bool suffix)
{
  string::size_type start, stop;
  
  start = 0; stop = start + 1;
  while(start != stop)
  {
    string subrule = get_next_subrule(rule, start, stop);
    if(start != stop)
    {
      subrule = morph_unescape_string(subrule);
      if(suffix)
        _suffixrules->add_subrule(t, subrule);
      else
        _prefixrules->add_subrule(t, subrule);

      start = stop;
      stop = start + 1;
    }
  }
}

void tMorphAnalyzer::add_rule(type_t t, string rule)
{
  if(verbosity > 9)
    LOG(loggerUncategorized, Level::INFO,
        "INFLR<%s>: %s", print_name(t), rule.c_str());

  if(rule.compare(0, 6, "suffix") == 0)
    parse_rule(t, rule.substr(6, rule.length() - 6), true);
  else if(rule.compare(0, 6, "prefix") == 0)
    parse_rule(t, rule.substr(6, rule.length() - 6), false);
  else
    throw tError(string("unknown type of morphological rule [") + print_name(t) + "]: " + rule);
}

void tMorphAnalyzer::add_irreg(string stem, type_t t, string form)
{
  if(verbosity > 14)
    LOG(loggerUncategorized, Level::INFO,
        "IRREG: %s + %s = %s",
        stem.c_str(), print_name(t), form.c_str());

  list<type_t> rules;
  rules.push_front(t);
  
  list<string> forms;
  forms.push_front(form);
  forms.push_front(stem);

  tMorphAnalysis *a = new tMorphAnalysis(forms, rules);

  _irregs_by_stem.insert(make_pair(stem, a));
  _irregs_by_form.insert(make_pair(form, a));
}

bool tMorphAnalyzer::empty()
{
    return _irregs_by_stem.size() == 0 && _subrules.size() == 0;
}

void tMorphAnalyzer::print(FILE *f)
{
  fprintf(stderr, "tMorphAnalyzer[%x]:\n", (size_t) this);
  _lettersets->print(f);
  _prefixrules->print(f);
  _suffixrules->print(f);
}

morph_letterset *tMorphAnalyzer::letterset(string name)
{
  return _lettersets->get(name);
}

void tMorphAnalyzer::undo_letterset_bindings()
{
  _lettersets->undo_bindings();
}

list<tMorphAnalysis> tMorphAnalyzer::analyze1(tMorphAnalysis form)
{
  list<tMorphAnalysis> pre, suf;

  /* Apply prefix and suffix rules in parallel to form and append the result
     lists efficiently to one result */
  pre = _prefixrules->analyze(form);
  suf = _suffixrules->analyze(form);

  pre.splice(pre.begin(), suf);


  // handle irregular forms

  // 1) suppress regular decompositions, if so desired by the grammar

  if(_irregs_only)
  {
    suf.splice(suf.end(), pre);

    for(list<tMorphAnalysis>::iterator it = suf.begin();
        it != suf.end(); ++it)
      if(!matching_irreg_form(*it)) pre.push_back(*it);
  }

  // 2) add irregular analyses from table

  string base = form.base();
  pair<multimap<string, tMorphAnalysis *>::iterator,
    multimap<string, tMorphAnalysis *>::iterator> eq =
    _irregs_by_form.equal_range(base);

  for(multimap<string, tMorphAnalysis *>::iterator it = eq.first;
      it != eq.second; ++it) {

    //
    // _fix_me_
    // the following largely duplicates code from morph_trie::analyze(): here,
    // we need to check the orthographemic chain so far, so as to avoid adding
    // a cycle (as triggered, for example, by rules like `bet past_verb bet).
    // 
    //
    type_t candidate = it->second->rules().front();
    list<type_t> rules = form.rules();
    list<string> forms = form.forms();
    list<type_t>::iterator rule;
    list<string>::iterator form;
    bool cyclep = false;
    for(rule = rules.begin(), form = forms.begin();
        rule != rules.end() && form != forms.end();
        ++rule, ++form) {

      if(*rule == candidate && (_duplicate_filter_p || *form == base)) {
        cyclep = true;
        break;
      } // if

    } // for

    if(cyclep) continue;

    rules.push_front(candidate);
    forms.push_front(it->second->base());
    pre.push_back(tMorphAnalysis(forms, rules));

  } // for

  return pre;
}

/** Check if there is an irregular entry with the same base form and first
 *  inflection rule as \a a.
 */
bool tMorphAnalyzer::matching_irreg_form(tMorphAnalysis a)
{
  pair<multimap<string, tMorphAnalysis *>::iterator,
    multimap<string, tMorphAnalysis *>::iterator> eq =
    _irregs_by_stem.equal_range(a.base());

  if(a.rules().size() == 0)
    return false;

  type_t first = a.rules().front();

  for(multimap<string, tMorphAnalysis *>::iterator it = eq.first;
      it != eq.second; ++it)
  {
    if(it->second->rules().front() == first)
    {
      if(verbosity > 4)
      {
        fprintf(fstatus, "filtering ");
        a.print(fstatus);
        fprintf(fstatus, " [irreg ");
        it->second->print(fstatus);
        fprintf(fstatus, "]\n");
      }

      return true;
    }
  }

  return false;
}

list<tMorphAnalysis> tMorphAnalyzer::analyze(string form)
{
  LOG(loggerUncategorized, Level::DEBUG,
      "tMorphAnalyzer::analyze(%s)", form.c_str());

  // least fixpoint iteration
  
  // final_results accumulates the final results. prev_results contains the
  // results found in the previous iteration. it's initialized with the
  // input form. the new results found in one iteration are accumulated in
  // current_results.

  list<tMorphAnalysis> final_results, prev_results;

  list<string> forms;
  forms.push_front(form);

  prev_results.push_back(tMorphAnalysis(forms, list<type_t>()));
  
  unsigned int i = 0;
  while(prev_results.size() > 0 
        && (!_maximal_depth || i++ <= _maximal_depth))
  {
    if(verbosity > 9)
    {
      fprintf(fstatus, "tMorphAnalyzer working on:\n");
      print_analyses(fstatus, prev_results);
      fprintf(fstatus, "--------------------------\n");
    }

    list<tMorphAnalysis> current_results;
    for(list<tMorphAnalysis>::iterator it = prev_results.begin();
        it != prev_results.end(); ++it)
    {
      list<tMorphAnalysis> r = analyze1(*it);
      current_results.splice(current_results.end(), r);
      final_results.push_back(*it);
    }

    prev_results.clear();
    prev_results.splice(prev_results.end(), current_results);
  }

  // filter duplicates
  erase_duplicates(final_results);
  
  return final_results;
}

string tMorphAnalyzer::generate(tMorphAnalysis m)
{
  return string("");
}

void
tLKBMorphology::undump_inflrs(dumper &dmp) {
  int ninflrs = dmp.undump_int();
  for(int i = 0; i < ninflrs; i++)
    {
      int t = dmp.undump_int();
      char *r = dmp.undump_string();

      if(r == 0)
        continue;
	  
      if(t == -1)
        _morph.add_global(string(r));
      else
        {
          _morph.add_rule(t, string(r));

          grammar_rule *rule = Grammar->find_rule(t);
          if (rule != NULL) {
            if(rule->trait() == SYNTAX_TRAIT)
              LOG(loggerUncategorized, Level::WARN,
                  "warning: found syntax `%s' rule "
                  "with attached infl rule `%s'",
                  print_name(t), r);
                
            rule->trait(INFL_TRAIT);
          } else {
            LOG(loggerUncategorized, Level::WARN,
                "warning: rule `%s' with infl annotation "
                "`%s' doesn't correspond to any of the parser's "
                "rules", print_name(t), r);
          }
        }

      delete[] r;
    }

  if(verbosity > 4)
    LOG(loggerUncategorized, Level::INFO, ", %d infl rules", ninflrs);

  if(verbosity >14) _morph.print(fstatus);
}

void
tLKBMorphology::undump_irregs(dumper &dmp) {  // irregular forms
  int nirregs = dmp.undump_int();
  for(int i = 0; i < nirregs; i++)
    {
      char *form = dmp.undump_string();
      char *infl = dmp.undump_string();
      char *stem = dmp.undump_string();

      strtolower(infl);
      type_t inflr = lookup_type(infl);
      if(inflr == -1)
        {
          LOG(loggerUncategorized, Level::INFO,
              "Ignoring entry with unknown rule `%s' "
              "in irregular forms", infl);
          delete[] form; delete[] infl; delete[] stem;
          continue;
        }
	  
      _morph.add_irreg(string(stem), inflr, string(form));
      delete[] form; delete[] infl; delete[] stem;
    }
}

/** LKB like online morphology with regexps for suffixes and prefixes and a
 *  table for irregular forms.
 */
tLKBMorphology *
tLKBMorphology::create(dumper &dmp) {
  tLKBMorphology *result = new tLKBMorphology();

  // XXX _fix_me_ this should go into grammar-dump
  dmp.seek(0);
  int version;
  undump_header(&dmp, version);
  dump_toc toc(&dmp);
  if(cheap_settings->lookup("irregular-forms-only"))
    result->_morph.set_irregular_only(true);

  if(toc.goto_section(SEC_INFLR))
    {
      result->undump_inflrs(dmp);
    }
  if(toc.goto_section(SEC_IRREGS))
    {
      result->undump_irregs(dmp);
    }
  if(result->_morph.empty()) {
    delete result;
    return NULL;
  } else {
    return result;
  }
}


/** Create a morphology component from the fullform tables, if available. */
tFullformMorphology *
tFullformMorphology::create(dumper &dmp) {
  dmp.seek(0);
  int version;
  undump_header(&dmp, version);
  dump_toc toc(&dmp);
  if(toc.goto_section(SEC_FULLFORMS)) {
    tFullformMorphology *newff = new tFullformMorphology(dmp);
    if (newff->_fullforms.empty()) {
      delete newff;
      newff = NULL;
    }
    return newff;
  } else {
    return NULL;
  }       
}

tFullformMorphology::tFullformMorphology(dumper &dmp) {
  int nffs = dmp.undump_int();
  int invalid = 0;

  int preterminal, affix, offset;
  char *s;
  lex_stem *lstem;

  for(int i = 0; i < nffs; i++)
    {
      preterminal = dmp.undump_int();
      affix = dmp.undump_int();
      offset = dmp.undump_char();
      s = dmp.undump_string();

      // If we do not find a stem for the preterminal, this full form is not
      // valid 
      if ((lstem = Grammar->find_stem(preterminal)) != NULL) {
        
        list<type_t> affixes;
        if (affix != -1) {
          affixes.push_front(affix);
        }
          
        // XXX _fix_me_ I'm not sure about what that offset really means.
        // It seems to me to be modelled wrongly and rather belong to the stem.
        // lstem->keydtr(offset);
          
        const char *stem = lstem->orth(offset);
        list<string> forms;
        forms.push_front(string(stem));
        forms.push_back(string(s));
        tMorphAnalysis new_morph(forms, affixes);
          
        bool found = false;
        ffdict::iterator it = _fullforms.find(s);
        if (it == _fullforms.end()) {
          _fullforms[s].push_back(new_morph);
        }
        else {
          list< tMorphAnalysis > &li = it->second;
          if (find(li.begin(), li.end(), new_morph) == li.end()) {
            li.push_back(new_morph);
          } else {
            found = true;
          }
        }

        if(verbosity > 14)
          {
            LOG_ONLY(PrintfBuffer pb);
            LOG_ONLY(pbprintf(pb, "("));
            LOG_ONLY(
              for(list<type_t>::iterator affix = affixes.begin()
                  ; affix != affixes.end(); affix++)
              {
                pbprintf(pb, "%s@%d ", print_name(*affix), offset);
              }
            );
              
            LOG_ONLY(pbprintf(pb, "("));
            LOG_ONLY(lstem->print(pb));
            LOG_ONLY(pbprintf(pb, ")"));
              
            LOG_ONLY(
              for(int i = 0; i < lstem->length(); i++)
                pbprintf(pb, " \"%s\"", lstem->orth(i));
            );
  
            LOG_ONLY( pbprintf(pb, ")%s", (found ? "dupl" : "")));
            
            LOG(loggerUncategorized, Level::DEBUG, "%s", pb.getContents());
          }
      } else {
        invalid++;
      }

      delete[] s;
    }

  LOG(loggerUncategorized, Level::DEBUG, "%d full form entries", nffs);
  if (invalid > 0)
    LOG(loggerUncategorized, Level::DEBUG, "%d of them invalid", invalid);
}

void tFullformMorphology::print(FILE *out) {
  for(ffdict::iterator it=_fullforms.begin(); it != _fullforms.end(); it++){
    const string &ff = it->first;
    for(list<tMorphAnalysis>::iterator jt = it->second.begin()
          ; jt != it->second.end(); jt++) {
      list<lex_stem *> stems = Grammar->lookup_stem(jt->base());
      for(list<lex_stem *>::iterator kt = stems.begin()
            ; kt != stems.end(); kt++) {
        fprintf(out, "%s\t%s\t0\t%s\t%s\n", ff.c_str()
                , print_name(jt->rules().front())
                , (*kt)->orth((*kt)->inflpos()), (*kt)->printname());
      }
    }
  }
}

/** Compute morphological results for \a form. */
list<tMorphAnalysis>
tFullformMorphology::operator()(const myString &form){
  ffdict::iterator it = _fullforms.find(form);
  if (it != _fullforms.end())
    return it->second;
  else
    return _emptyresult;
}
