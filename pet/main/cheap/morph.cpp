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

#include "pet-system.h"
#include "cheap.h"
#include "unicode.h"
#include "morph.h"

//
// utility functions
//

// return next list (stuff between (balanced) parantheses) in s, starting at
// start; the position of the closing paren (relative to s) is returned in
// stop
string get_next_list(string &s, string::size_type start,
                     string::size_type &stop)
{
  stop = start;

  string::size_type openp = s.find("(", start);
  if(openp == STRING_NPOS)
    return string();

  int plevel = 0;
  string::size_type closep;
  for(closep = openp; closep < s.length(); ++closep)
  {
    if(s[closep] == '(') plevel++;
    else if(s[closep] == ')') plevel--;
    if(plevel == 0) break;
  }
  
  if(plevel != 0)
  {
    throw tError("unbalanced list");
  }

  stop = closep;
  return s.substr(openp+1, closep-openp-1);
}

void print_analyses(FILE *f, list<morph_analysis> res)
{
  int id = 0;
  for(list<morph_analysis>::iterator it = res.begin(); it != res.end(); ++it)
  {
    fprintf(f, "[%d]: ", id++);
    it->print(f);
    fprintf(f, "\n");
  }
}

//
// internal classes: interface
//

// represents one letterset, primary purpose is keeping track of bindings
class morph_letterset
{
public:
  morph_letterset() :
    _bound(0) {};

  morph_letterset(string name, string elems);

  const set<UChar32> &elems() { return _elems; }

  void bind(UChar32 c) { _bound = c; }
  UChar32 bound() { return _bound; }

  const string &name() const { return _name; }
  
  void print(FILE *f);

private:
  string _name;
  set<UChar32> _elems;
  UChar32 _bound;
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

class morph_subrule
{
public:
  morph_subrule(morph_analyzer *a, type_t rule,
                UnicodeString left, UnicodeString right)
    : _analyzer(a), _rule(rule), _left(left), _right(right) {}
  
  type_t rule() { return _rule; }

  bool base_form(UnicodeString matched, UnicodeString rest,
                 UnicodeString &result);

  void print(FILE *f);

private:
  morph_analyzer *_analyzer;

  type_t _rule;
  
  UnicodeString _left, _right;

  bool establish_and_check_bindings(UnicodeString matched);
};

class trie_node
{
public:
  trie_node(morph_analyzer *a) :
    _analyzer(a)
  {};

  trie_node *get_node(UChar32 c, bool add = false);

  void add_path(UnicodeString path, morph_subrule *rule);

  bool has_rules() { return _rules.size() > 0; }
  vector<morph_subrule *> &rules() { return _rules; }

  void print(FILE *f, int depth = 0);
  
private:
  morph_analyzer *_analyzer;

  map<UChar32, trie_node *> _s;

  vector<morph_subrule *> _rules;
};

class morph_trie
{
public:
  morph_trie(morph_analyzer *a, bool suffix) :
    _analyzer(a), _suffix(suffix), _root(_analyzer)
  {};

  void add_subrule(type_t rule, string subrule);

  list<morph_analysis> analyze(morph_analysis a);

  void print(FILE *f);

private:
  morph_analyzer *_analyzer;
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
  UnicodeString elems = Conv->convert(elems_u8);

  StringCharacterIterator it(elems);

  UChar32 c;
  while(it.hasNext())
  {
    c = it.next32PostInc();
    _elems.insert(c);
  }
}

void morph_letterset::print(FILE *f)
{
  fprintf(f, "!%s -> ", _name.c_str());

  for(set<UChar32>::iterator it = _elems.begin(); it != _elems.end(); ++it)
    fprintf(f, "%s", Conv->convert(*it).c_str());

  fprintf(f, " [%s]", Conv->convert(_bound).c_str());
}

//
// Lettersets (collection)
//

void morph_lettersets::add(string s)
{
  if(verbosity > 14)
    fprintf(fstatus, "LETTERSET: <%s>", s.c_str());

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
      if(verbosity > 14)
        fprintf(fstatus, " -> <%s> <%s>\n", name.c_str(), elems.c_str());
	  
      morph_letterset *ls = new morph_letterset(name, elems);
      _m[name] = ls;
    }
    else
    {
      fprintf(ferr, "Ignoring ill-formed letterset definition: %s\n",
              s.c_str());
    }
  }
  else
  {
    fprintf(ferr, "Ignoring ill-formed letterset definition: %s\n",
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
  fprintf(f, "--- morph_lettersets[%x]:\n", (int) this);
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

bool morph_subrule::establish_and_check_bindings(UnicodeString matched)
{
  StringCharacterIterator it1(_right);
  StringCharacterIterator it2(matched);
  UChar32 c1, c2;

  _analyzer->undo_letterset_bindings();

  while(it1.hasNext())
  {
    c1 = it1.next32PostInc();
    c2 = it2.next32PostInc();
    if(c1 == '!')
    {
      c1 = it1.next32PostInc();
      morph_letterset *ls = _analyzer->letterset(string(1, (char) c1));
      if(ls == 0)
        throw tError("Referencing undefined letterset !" + string(1, (char) c1));
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

bool morph_subrule::base_form(UnicodeString matched,
                              UnicodeString rest,
                              UnicodeString &result)
{
  if(establish_and_check_bindings(matched) == false)
    return false;

  // build base form, translating lettersets

  result.remove();

  StringCharacterIterator it(_left);

  UChar32 c;
  while(it.hasNext())
  {
    c = it.next32PostInc();
    if(c == '!')
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
          (int) this, printnames[_rule],
          Conv->convert(_left).c_str(), Conv->convert(_right).c_str());
}

//
// trie node
//

trie_node *trie_node::get_node(UChar32 c, bool add)
{
  map<UChar32, trie_node *>::iterator it = _s.find(c);

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

void trie_node::add_path(UnicodeString path, morph_subrule *rule)
{
  if(path.length() == 0)
  {
    // base case, add rule to this node
    _rules.push_back(rule);
  }
  else
  {
    // recursive case, treat first element in path
    if(path.char32At(0) != '!')
    {
      // it's not a letterset
      UChar32 c = path.char32At(0);
      UnicodeString rest(path);
      rest.remove(0, 1);

      trie_node *n = get_node(c, true);
      n->add_path(rest, rule);
    }
    else
    {
      // it's a letterset
      UChar32 c = path.char32At(1);
      UnicodeString rest(path);
      rest.remove(0, 2);

      string lsname = string(1, (char) c);
      morph_letterset *ls = _analyzer->letterset(lsname);

      if(ls == 0)
        throw tError("Referencing undefined letterset !" + lsname);

      const set<UChar32> &elems = ls->elems();
      for(set<UChar32>::const_iterator it = elems.begin();
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

  fprintf(f, "trie[%x] (", (int) this);
  
  for(vector<morph_subrule *>::iterator it = _rules.begin();
      it != _rules.end(); ++it)
  {
    (*it)->print(f);
    fprintf(f, " ");
  }
  fprintf(f, ")\n");

  for(map<UChar32, trie_node *>::iterator it = _s.begin();
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

void reverse_subrule(UnicodeString &s)
  // basically just a reverse, but restoring !-sequences 
{
  s.reverse();
  
  UTextOffset off = 0;
  while((off = s.indexOf((UChar32) '!', off)) != -1)
    s.reverse(s.getChar32Start(off-1), 2);
}

void morph_trie::add_subrule(type_t rule, string subrule)
{
  string left_u8, right_u8;

  string::size_type curr = 0;
  while(curr < subrule.length() && !isspace(subrule[curr])) ++curr;

  if(curr == subrule.length())
    throw tError("Invalid subrule `" + subrule + "' in rule " + printnames[rule]);

  left_u8 = subrule.substr(0, curr);
  if(left_u8 == "*")
    left_u8 = "";

  while(curr < subrule.length() && isspace(subrule[curr])) ++curr;

  if(curr == subrule.length())
    throw tError("Invalid subrule `" + subrule + "' in rule " + printnames[rule]);

  right_u8 = subrule.substr(curr, subrule.length() - curr);

  UnicodeString left = Conv->convert(left_u8);
  UnicodeString right = Conv->convert(right_u8);

  if(_suffix)
  {
    reverse_subrule(left);
    reverse_subrule(right);
  }

  if(verbosity > 14)
    fprintf(fstatus, "INFLSUBRULE<%s>: %s (`%s' -> `%s')\n",
            printnames[rule], subrule.c_str(),
            Conv->convert(left).c_str(), Conv->convert(right).c_str());

  morph_subrule *sr = new morph_subrule(_analyzer, rule, left, right);
  _analyzer->add_subrule(sr);
  _root.add_path(right, sr);

}

list<morph_analysis> morph_trie::analyze(morph_analysis a)
{
  list<morph_analysis> res;

  UnicodeString s = Conv->convert(a.base());
  if(_suffix) s.reverse();

  UnicodeString matched;

  trie_node *n = &_root;

  while(s.length() > 0)
  {
    UChar32 c = s.char32At(0);
    s.remove(0, 1);
    matched.append(c);

    n = n->get_node(c);
    if(n == 0) return res;

    for(vector<morph_subrule *>::iterator r = n->rules().begin();
        r != n->rules().end(); ++r)
    {
      UnicodeString base;
      if((*r)->base_form(matched, s, base) == false)
        continue;
	  
      if(_suffix) base.reverse();

      string st = Conv->convert(base);

      type_t candidate = (*r)->rule();
      list<type_t> rules = a.rules();
      list<string> forms = a.forms();
      list<type_t>::iterator rule;
      list<string>::iterator form;

      //
      // See if sequence of rule applications is compatible with
      // the rule filter.
      //
      if(rules.size() > 0 && 
         !_analyzer->_grammar->filter_compatible(rules.front(), 1, candidate))
      {
        continue;
      }

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
        if(*rule == candidate && *form == st) 
        {
          cyclep = true;
          break;
        }
      }
      if(cyclep) continue;

      list<lex_stem *> stems = _analyzer->_grammar->lookup_stem(st);

      rules.push_front(candidate);
      forms.push_front(st);
	  
      res.push_back(morph_analysis(forms, rules, stems));
    }
  }

  return res;
}

void morph_trie::print(FILE *f)
{
  fprintf(f, "--- morph_trie[%x] (%s)\n", (int) this,
          _suffix ? "suffix" : "prefix");
  _root.print(f);
}

//
// Exported classes
//

//
// Analysis
//

void morph_analysis::print(FILE *f)
{
  fprintf(f, "%s = %s (%d stem%s", complex().c_str(), base().c_str(),
          _stems.size(), _stems.size() == 1 ? "" : "s");

#ifdef DEBUG
  for(list<lex_stem *>::iterator it = _stems.begin(); it != _stems.end(); ++it)
    fprintf(f, " %x", (int) *it);
#endif

  fprintf(f, ")");

  for(list<type_t>::iterator it = _rules.begin(); it != _rules.end(); ++it)
    fprintf(f, " + %s", printnames[*it]);
}

void morph_analysis::print_lkb(FILE *f)
{
  list<string>::iterator form = _forms.begin();

  fprintf(f, "(%s", form->c_str());
  ++form;

  list<type_t>::iterator rule = _rules.begin(); 

  while(form != _forms.end() && rule != _rules.end())
  {
    fprintf(f, " (%s %s)", printnames[*rule], form->c_str());
    ++form; ++rule;
  }

  fprintf(f, ")");
}

//
// Analzer
//

morph_analyzer::morph_analyzer(tGrammar *G)
  : _grammar(G),
    _lettersets(new morph_lettersets),
    _suffixrules(new morph_trie(this, true)),
    _prefixrules(new morph_trie(this, false)),
    _irregs_only(false)
{
}

morph_analyzer::~morph_analyzer()
{
  for(vector<morph_subrule *>::iterator it = _subrules.begin();
      it != _subrules.end(); ++it)
    delete *it;

  for(multimap<string, morph_analysis *>::iterator it =
        _irregs_by_stem.begin(); it != _irregs_by_stem.end(); ++it)
    delete it->second;

  delete _prefixrules;
  delete _suffixrules;
  delete _lettersets;
}

void morph_analyzer::add_global(string rule)
{
  if(verbosity > 9)
    fprintf(fstatus, "INFLR<global>: %s\n", rule.c_str());

  string::size_type start, stop;
  
  start = 0; stop = 1;
  while(start != stop)
  {
    string s = get_next_list(rule, start, stop);
    if(start != stop)
    {
      if(s.substr(0, 10) == string("letter-set"))
      {
        string::size_type stop;
        string ls = get_next_list(s, 0, stop);
        if(stop != 0)
        {
          _lettersets->add(ls);
        }
      }
      else
      {
        fprintf(ferr, "ignoring unknown type of inflr <%s>\n",
		      s.c_str());
      }
      start = stop; stop = 1;
    }
  }
}

void morph_analyzer::parse_rule(type_t t, string rule, bool suffix)
{
  string::size_type start, stop;
  
  start = 0; stop = start + 1;
  while(start != stop)
  {
    string subrule = get_next_list(rule, start, stop);
    if(start != stop)
    {
      if(suffix)
        _suffixrules->add_subrule(t, subrule);
      else
        _prefixrules->add_subrule(t, subrule);

      start = stop;
      stop = start + 1;
    }
  }
}

void morph_analyzer::add_rule(type_t t, string rule)
{
  if(verbosity > 9)
    fprintf(fstatus, "INFLR<%s>: %s\n", printnames[t], rule.c_str());

  if(rule.substr(0, 6) == string("suffix"))
    parse_rule(t, rule.substr(6, rule.length() - 6), true);
  else if(rule.substr(0,6) == string("prefix"))
    parse_rule(t, rule.substr(6, rule.length() - 6), false);
  else
    throw tError(string("unknown type of morphological rule [") + printnames[t] + "]: " + rule);
}

void morph_analyzer::add_irreg(string stem, type_t t, string form)
{
  if(verbosity > 14)
    fprintf(fstatus, "IRREG: %s + %s = %s\n",
            stem.c_str(), printnames[t], form.c_str());

  list<lex_stem *> stems = _grammar->lookup_stem(stem);
  if(stems.empty())
  {
    fprintf(ferr, "Ignoring entry with unknown stem `%s' "
            "in irregular forms\n", stem.c_str());
    return;
  }

  list<type_t> rules;
  rules.push_front(t);
  
  list<string> forms;
  forms.push_front(form);
  forms.push_front(stem);

  morph_analysis *a = new morph_analysis(forms, rules, stems);

  _irregs_by_stem.insert(make_pair(stem, a));
  _irregs_by_form.insert(make_pair(form, a));
}

bool morph_analyzer::empty()
{
    return _irregs_by_stem.size() == 0 && _subrules.size() == 0;
}

void morph_analyzer::print(FILE *f)
{
  fprintf(stderr, "morph_analyzer[%x]:\n", (int) this);
  _lettersets->print(f);
  _prefixrules->print(f);
  _suffixrules->print(f);
}

morph_letterset *morph_analyzer::letterset(string name)
{
  return _lettersets->get(name);
}

void morph_analyzer::undo_letterset_bindings()
{
  _lettersets->undo_bindings();
}

list<morph_analysis> morph_analyzer::analyze1(morph_analysis form)
{
  list<morph_analysis> pre, suf;

  pre = _prefixrules->analyze(form);
  suf = _suffixrules->analyze(form);

  pre.splice(pre.begin(), suf);

  return pre;
}

bool morph_analyzer::matching_irreg_form(morph_analysis a)
{
  pair<multimap<string, morph_analysis *>::iterator,
    multimap<string, morph_analysis *>::iterator> eq =
    _irregs_by_stem.equal_range(a.base());

  if(a.rules().size() == 0)
    return false;

  type_t first = a.rules().front();

  for(multimap<string, morph_analysis *>::iterator it = eq.first;
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

list<morph_analysis> morph_analyzer::analyze(string form, bool lexfilter)
{
  if(verbosity > 7)
    fprintf(fstatus, "morph_analyzer::analyze(%s, %d)\n", form.c_str(), (int) lexfilter);

  // least fixpoint iteration
  
  // final_results accumulates the final results. prev_results contains the
  // results found in the previous iteration. it's initialized with the
  // input form. the new results found in one iteration are accumulated in
  // current_results.

  list<morph_analysis> final_results, prev_results;

  list<lex_stem *> stems = _grammar->lookup_stem(form);
  list<string> forms;
  forms.push_front(form);

  prev_results.push_back(morph_analysis(forms, list<type_t>(), stems));
  
  while(prev_results.size() > 0)
  {
    if(verbosity > 9)
    {
      fprintf(fstatus, "morph_analyzer working on:\n");
      print_analyses(fstatus, prev_results);
      fprintf(fstatus, "--------------------------\n");
    }

    list<morph_analysis> current_results;
    for(list<morph_analysis>::iterator it = prev_results.begin();
        it != prev_results.end(); ++it)
    {
      list<morph_analysis> r = analyze1(*it);
      current_results.splice(current_results.end(), r);
      if(!lexfilter || it->valid())
        final_results.push_back(*it);
    }

    prev_results.clear();
    prev_results.splice(prev_results.end(), current_results);
  }

  // handle irregular forms

  // 1) filter regular results if desired

  if(_irregs_only)
  {
    prev_results.clear();
    prev_results.splice(prev_results.end(), final_results);

    for(list<morph_analysis>::iterator it = prev_results.begin();
        it != prev_results.end(); ++it)
      if(!matching_irreg_form(*it))
        final_results.push_back(*it);
  }

  // 2) add irregular analyses from table

  pair<multimap<string, morph_analysis *>::iterator,
    multimap<string, morph_analysis *>::iterator> eq =
    _irregs_by_form.equal_range(form);

  for(multimap<string, morph_analysis *>::iterator it = eq.first;
      it != eq.second; ++it)
    final_results.push_back(*it->second);

  return final_results;
}

string morph_analyzer::generate(morph_analysis m)
{
  return string("");
}
