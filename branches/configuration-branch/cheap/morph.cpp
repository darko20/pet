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

#include "cheap.h"
#include "grammar-dump.h"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/transitive_closure.hpp>
//#include <boost/graph/graph_utility.hpp>
#include <iomanip>


#ifndef HAVE_ICU

Converter Convert;
Converter *Conv = &Convert;

#endif


#define LETTERSET_CHAR ((MChar) '\x8')

bool operator==(const tMorphAnalysis &a, const tMorphAnalysis &b) {
  if (a.base() != b.base()) return false;
  ruleiter ar, ae, br, be;
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

void print_analyses(std::ostream &out, list<tMorphAnalysis> res)
{
  int id = 0;
  for(list<tMorphAnalysis>::iterator it = res.begin(); it != res.end(); ++it) {
    out << "[" << id++ << "]: " << *it << endl;
  }
}

//
// internal classes: interface: see morph-inner.h
//

//
// Internal classes: implementation
//

//
// Letterset
//

void morph_letterset::set(string name, string elems_u8) {
  _name = name;
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

void morph_letterset::print(std::ostream &out) const {
  out << "!" << _name << " -> ";
  for(std::set<MChar>::const_iterator it = _elems.begin();
      it != _elems.end(); ++it)
    out << Conv->convert(*it);
  out << " [" << Conv->convert(_bound) << "]";
}


//
// Lettersets (collection)
//

void morph_lettersets::add(string s)
{
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
      LOG(logMorph, DEBUG,
          "LETTERSET: <" << s << "> => <" << name << "> <" << elems << ">");
      //morph_letterset ls = morph_letterset(name, elems);
      _m[name].set(name, elems);
    }
  }

  if (p >= s.length())
    LOG(logMorph, WARN, "Ignoring ill-formed letterset definition: " << s);
}

morph_letterset * morph_lettersets::get(string name) const
{
  ml_const_iterator pos = _m.find(name);

  if(pos == _m.end())
    return 0;
  
  return const_cast<morph_letterset *>(&pos->second);
}

void morph_lettersets::undo_bindings() {
  for(ml_iterator it = _m.begin(); it != _m.end(); ++it)
    (it->second).bind(0);
}

void morph_lettersets::print(ostream &out) const {
  out << "--- morph_lettersets[" << ((size_t) this) << "]:" << endl ;
  for(ml_const_iterator it = _m.begin(); it != _m.end(); ++it) {
    out << "  " << (it->second) << endl;
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
    else // This would be a conception error in morphology
      assert(c1 == c2);
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

void morph_subrule::print(ostream &out) const {
  out << "morph_subrule[" << std::setbase(16) << (size_t) this << "] "
      << _rule->printname() << ": " 
      << Conv->convert(_left) << "->" << Conv->convert(_right) ;
}

//
// trie node
//

trie_node::~trie_node() {
  for(tn_const_iterator it = _s.begin(); it != _s.end(); ++it)
    delete it->second;
}

void trie_node::add_path(MString path, morph_subrule *rule,
                         const morph_lettersets &lettersets)
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
      
      request_node(c)->add_path(rest, rule, lettersets);
    }
    else
    {
      // it's a letterset
      MChar c = path.char32At(1);
      MString rest(path);
      rest.remove(0, 2);

      string lsname = string(1, (char) c);
      morph_letterset *ls = lettersets.get(lsname);

      if(ls == 0)
        throw tError("Referencing undefined letterset !" + lsname);

      const set<MChar> &elems = ls->elems();
      for(set<MChar>::const_iterator it = elems.begin();
          it != elems.end(); ++it) {
        request_node(*it)->add_path(rest, rule, lettersets);
      }
    }
  }
}

void trie_node::print(std::ostream &out, int depth) const {
  out << std::setw(depth) << "" 
      << "trie[" << setbase(16) << (size_t) this << "] (";
  for(vector<morph_subrule *>::const_iterator it = _rules.begin();
      it != _rules.end(); ++it) {
    out << (*it) << " ";
  }
  out << ")" << endl;

  for(tn_const_iterator it = _s.begin(); it != _s.end(); ++it) {
    out << std::setw(depth) << "" 
        << "[" << Conv->convert(it->first) << "]" << endl;
    it->second->print(out, depth+2);
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

void morph_trie::add_subrule(grammar_rule *rule, string subrule)
{
  string left_u8, right_u8;

  string::size_type curr = 0;
  while(curr < subrule.length() && !isspace(subrule[curr])) ++curr;

  if(curr == subrule.length())
    throw tError("Invalid subrule `" + subrule + "' in rule " 
                 + rule->printname());

  left_u8 = subrule.substr(0, curr);
  if(left_u8 == "*")
    left_u8 = "";

  // strip blanks at the beginning of the left part of a morph subrule
  while(curr < subrule.length() && isspace(subrule[curr])) ++curr;

  // strip blanks at the end of the left part of a morph subrule
  string::size_type rule_end = subrule.length() - 1;
  while(rule_end >= curr && isspace(subrule[rule_end])) --rule_end;

  if(curr > rule_end)
    throw tError("Invalid subrule `" + subrule + "' in rule " 
                 + rule->printname());

  right_u8 = subrule.substr(curr, rule_end - curr + 1);

  MString left = Conv->convert(left_u8);
  MString right = Conv->convert(right_u8);

  if(_suffix)
  {
    reverse_subrule(left);
    reverse_subrule(right);
  }

  LOG(logMorph, DEBUG, "INFLSUBRULE<" << rule->printname() << ">: "
      << subrule << " (`" << Conv->convert(left) << "' -> `"
      <<  Conv->convert(right) << "')");

  morph_subrule *sr = new morph_subrule(_analyzer, rule, left, right);
  _analyzer->add_subrule(sr);
  _root.add_path(right, sr, _analyzer->_lettersets);

} 

/** Remove all possible prefixes (or suffixes) encoded in this trie from the
 *  base string in \a a and return the list of newly created analyses.
 */
list<tMorphAnalysis> morph_trie::analyze(tMorphAnalysis analysis) {
  list<tMorphAnalysis> res;

  MString s = Conv->convert(analysis.base());
  if(_suffix) s.reverse();

  MString matched;

  const trie_node *node = &_root;

  while(s.length() > 0)
  {
    MChar c = s.char32At(0);
    s.remove(0, 1);
    matched.append(c);

    // is there a branch at this trie node labeled with character c?
    // If not so, we are done
    if (! node->has_subnode(c)) return res;
    node = node->get_node(c);

    // Iterate through all subrules that have been matched completedly when
    // arriving at the current trie node.
    for(vector<morph_subrule *>::const_iterator r = node->rules().begin();
        r != node->rules().end(); ++r)
    {
      MString base;
      // Can the rule reduce the string "matched + s" to a valid base form ?
      if((*r)->base_form(matched, s, base) == false)
        continue;

      grammar_rule *candidate = (*r)->rule();
      list<grammar_rule *> rules = analysis.rules();
 
      // ATTENTION! CHECK THIS!
      // \todo does this depend on the trie being suffix or prefix ???
      // Check the rule filter (if activated) with candidate as daughter rule
      // and the first rule in the 'rules' list as mother, because the
      // morphological analysis works itself from the bottom up but is applied
      // in reverse direction (aargh)
      if (! rules.empty()
          && ! _analyzer->lexfilter_compatible(rules.front(), candidate))
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

      list<string> forms = analysis.forms();
      ruleiter rule;
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

void morph_trie::print(std::ostream &out) const {
  out << "--- morph_trie (" << (_suffix ? "suffix" : "prefix") << ")" << endl
      << _root;
}

//
// Exported classes
//

//
// Analysis
//

void tMorphAnalysis::print(std::ostream &out) const {
  out << complex() << " = " << base();
  for(ruleiter it = _rules.begin(); it != _rules.end(); ++it)
    out << " + " <<  (*it)->printname();
}

void tMorphAnalysis::print_lkb(std::ostream &out) const {
  list<string>::const_iterator form = _forms.begin();

  out << "(" << *form;
  ++form;

  ruleiter rule = _rules.begin(); 
  while(form != _forms.end() && rule != _rules.end()) {
    out << " (" << (*rule)->printname() << " " << *form << ")";
    ++form; ++rule;
  }

  out << ")";
}

//
// Analyzer
//

tMorphAnalyzer::tMorphAnalyzer()
  : _lettersets(),
    _suffixrules(this, true),
    _prefixrules(this, false),
    _irregs_only(false),
    _duplicate_filter_p(false), _maximal_depth(0), _minimal_stem_length(0) {
  char *foo;
  if((foo = cheap_settings->value("orthographemics-maximum-chain-depth")) != 0)
    _maximal_depth 
      = strtoint(foo, "as value of orthographemics-maximum-chain-depth");
  if((foo = cheap_settings->value("orthographemics-minimum-stem-length")) != 0)
    _minimal_stem_length 
      = strtoint(foo, "as value of orthographemics-minimum-stem-length");
  if(cheap_settings->lookup("orthographemics-duplicate-filter"))
    _duplicate_filter_p = true;
}

tMorphAnalyzer::~tMorphAnalyzer()
{
  for(vector<morph_subrule *>::iterator it = _subrules.begin();
      it != _subrules.end(); ++it)
    delete *it;

  for(multimap<string, tMorphAnalysis *>::iterator it =
        _irregs_by_stem.begin(); it != _irregs_by_stem.end(); ++it)
    delete it->second;
}

void tMorphAnalyzer::add_global(string rule) {
  LOG(logMorph, DEBUG, "INFLR<global>: " << rule);

  string::size_type start, stop;
  
  start = 0; stop = 1;
  while(start != stop) {

    string ls = get_next_letter_set(rule, start, stop);
    if(start != stop) {
      _lettersets.add(ls);
      start = stop; stop = 1;
    } // if
    else {
      //
      // ignore remaining trailing parens when looking at the final element
      //
      if(rule.compare(start, 2, "))") != 0) {
        string s = rule.substr(start);
        LOG(logMorph, WARN, "ignoring remaining letter-set(s) <" << s << ">");
      } // if 
    } // else
  } // while

} // tMorphAnalyzer::add_global()


void tMorphAnalyzer::parse_rule(grammar_rule *fsrule, string rule, bool suffix)
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
        _suffixrules.add_subrule(fsrule, subrule);
      else
        _prefixrules.add_subrule(fsrule, subrule);

      start = stop;
      stop = start + 1;
    }
  }
}

void tMorphAnalyzer::add_rule(grammar_rule *fsrule, string rule) {
  LOG(logMorph, DEBUG, "INFLR<" << fsrule->printname() << ">: "<< rule);

  if(rule.compare(0, 6, "suffix") == 0)
    parse_rule(fsrule, rule.substr(6, rule.length() - 6), true);
  else if(rule.compare(0, 6, "prefix") == 0)
    parse_rule(fsrule, rule.substr(6, rule.length() - 6), false);
  else
    throw tError(string("unknown type of morphological rule [") 
                 + fsrule->printname() + "]: " + rule);
}

void tMorphAnalyzer::add_irreg(string stem, type_t t, string form)
{
  LOG(logMorph, DEBUG,
      "IRREG: " << stem << " + " << print_name(t) << " = " << form);

  grammar_rule *rule = Grammar->find_rule(t);
  if (rule == NULL) {
    LOG(logMorph, WARN, "rule `" <<  print_name(t) 
        << "' in irregs definition `" << stem << "':`" << form 
        << "' doesn't correspond to any of the parser's rules" << endl);
    return;
  }
  rulelist rules;
  rules.push_front(rule);
  
  list<string> forms;
  forms.push_front(form);
  forms.push_front(stem);

  tMorphAnalysis *a = new tMorphAnalysis(forms, rules);

  _irregs_by_stem.insert(make_pair(stem, a));
  _irregs_by_form.insert(make_pair(form, a));
}


void tMorphAnalyzer::initialize_lexrule_filter() {
  using namespace boost;
  typedef adjacency_list<> graph_t;

  int nr_lexrules = Grammar->lexrules().size();
  std::vector<grammar_rule *> lexrules(nr_lexrules);
  int inflindex = 0;
  int lexindex = nr_lexrules - 1;
  // split the lexical rules into affixation and non-affixation rules
  for(ruleiter it = Grammar->lexrules().begin();
      it != Grammar->lexrules().end(); ++it) {
    assert((*it)->id() < nr_lexrules);
    lexrules[((*it)->trait() == INFL_TRAIT) ? inflindex++ : lexindex --] = *it;
  }
  // now let lexindex point to the first lexical rule
  lexindex++;
  assert(inflindex == lexindex);

  // we use two nodes per affixation rule to split the incoming and outgoing
  // edges appropriately
  // vertices 0 .. lexindex - 1 : inflrules for ingoing edges
  //          lexindex .. nr_lexrules - 1 : lexrules
  //          nr_lexrules .. verts.size() - 1 : inflrules for outgoing edges
  graph_t infl_reach(inflindex + nr_lexrules);

  // j is a non-affixation rule
  for (int j = lexindex; j < nr_lexrules; ++j) {
    // i is an affixation rule
    for (int i = 0; i < lexindex; ++i) {
      if (Grammar->filter_compatible(lexrules[i], 1, lexrules[j]))
        add_edge(i, j, infl_reach);
      if (Grammar->filter_compatible(lexrules[j], 1, lexrules[i]))
        add_edge(j, i + nr_lexrules, infl_reach);
    }
    if (Grammar->filter_compatible(lexrules[j], 1, lexrules[j]))
      add_edge(j, j, infl_reach);
    // k is a non-affixation rule
    for (int k = j + 1; k < nr_lexrules; ++k) {
      if (Grammar->filter_compatible(lexrules[k], 1, lexrules[j]))
        add_edge(k, j, infl_reach);
      if (Grammar->filter_compatible(lexrules[j], 1, lexrules[k]))
        add_edge(j, k, infl_reach);
    }
  }
  //std::cout << std::endl << "Graph infl_reach:" << std::endl;
  //print_graph(infl_reach);

  graph_t infl_trans;
  transitive_closure(infl_reach, infl_trans);

  //std::cout << std::endl << "Graph infl_trans:" << std::endl;
  //print_graph(infl_trans); std::cout << std::endl;

  // since we require to have all lexrules to not have SYNTAX_TRAIT and force
  // an exit in the case where this is violated, we can use a smaller filter
  // here
  // it would suffice if i would run only up to lexindex - 1 and only
  // those adjacent edges beyond nr_lexrules would be added since this filter
  // will only be applied to inflectional rules. We complete it since the costs
  // are negligible and it's easier for debugging
  _rulefilter.resize(nr_lexrules);
  graph_traits < graph_t >::adjacency_iterator ai, a_end;
  for (int i = 0; i < nr_lexrules; ++i) {
    // for the direct feeding of inflectional rules, we have to copy the old
    // filter since the graph only gives us the dependencies with at least one
    // non-affixation rule in between
    for (int j = 0; j < lexindex; ++j)
      if (Grammar->filter_compatible(lexrules[i], 1, lexrules[j]))
        _rulefilter.set(lexrules[i], lexrules[j]);

    tie(ai, a_end) = adjacent_vertices(i, infl_trans);
    for (; ai != a_end; ++ai) {
      std::size_t j = *ai;
      if (j >= (std::size_t) nr_lexrules) j -= nr_lexrules;
      _rulefilter.set(lexrules[i], lexrules[j]);
    }
  }
  /*
  printf("\n");
  char ticks[] = ".!*O";
  for (int i = 0; i < nr_lexrules; ++i)
    printf("%s\n", lexrules[i]->printname());
  for (int i = 0; i < nr_lexrules; ++i) {
    for (int j = 0; j < nr_lexrules; ++j) {
      int k = (_rulefilter.get(lexrules[i], lexrules[j]) ? 2 : 0)
        + (Grammar->filter_compatible(lexrules[i], 1, lexrules[j]) ? 1 : 0);
      printf("%c", ticks[k]);
    }
    printf("\n"); fflush(stdout);
  }
  */
}

bool tMorphAnalyzer::empty()
{
    return _irregs_by_stem.size() == 0 && _subrules.size() == 0;
}

void tMorphAnalyzer::print(ostream &out) const {
  out << "tMorphAnalyzer[" << std::setbase(16) << (size_t) this << "]:" << endl
      << _lettersets << _prefixrules << _suffixrules;
}

morph_letterset * tMorphAnalyzer::letterset(string name) const
{
  return _lettersets.get(name);
}

void tMorphAnalyzer::undo_letterset_bindings()
{
  _lettersets.undo_bindings();
}

void tMorphAnalyzer::analyze1(tMorphAnalysis form, list<tMorphAnalysis> &result)
{
  list<tMorphAnalysis> pre, suf;

  /* Apply prefix and suffix rules in parallel to form and append the result
     lists efficiently to one result */
  pre = _prefixrules.analyze(form);
  suf = _suffixrules.analyze(form);

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
    grammar_rule *candidate = it->second->rules().front();
    rulelist rules = form.rules();
    list<string> forms = form.forms();
    ruleiter rule;
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

  result.splice(result.end(), pre);
}

/** Check if there is an irregular entry with the same base form and first
 *  inflection rule as \a a.
 */
bool tMorphAnalyzer::matching_irreg_form(tMorphAnalysis analysis)
{
  pair<multimap<string, tMorphAnalysis *>::iterator,
    multimap<string, tMorphAnalysis *>::iterator> eq =
    _irregs_by_stem.equal_range(analysis.base());

  if(analysis.rules().size() == 0)
    return false;

  grammar_rule *first = analysis.rules().front();

  for(multimap<string, tMorphAnalysis *>::iterator it = eq.first;
      it != eq.second; ++it)
  {
    if(it->second->rules().front() == first)
    {
      LOG(logMorph, DEBUG, "filtering " << analysis 
          << " [irreg " << it->second << "]" << endl);

      return true;
    }
  }

  return false;
}

void debug_morph(list<tMorphAnalysis> results) {
  ostringstream cdeb;;
  cdeb << "tMorphAnalyzer working on:" << endl;
  print_analyses(cdeb, results);
  cdeb << "--------------------------" << endl ;
  LOG(logMorph, DEBUG, cdeb.str());
}


list<tMorphAnalysis> tMorphAnalyzer::analyze(string form)
{
  LOG(logMorph, DEBUG, "tMorphAnalyzer::analyze(" << form << ")");

  // least fixpoint iteration
  
  // final_results accumulates the final results. prev_results contains the
  // results found in the previous iteration. it's initialized with the
  // input form. the new results found in one iteration are accumulated in
  // current_results.

  list<tMorphAnalysis> final_results, prev_results;

  list<string> forms;
  forms.push_front(form);

  prev_results.push_back(tMorphAnalysis(forms, list<grammar_rule *>()));
  
  unsigned int i = 0;
  while(prev_results.size() > 0 
        && (!_maximal_depth || i++ <= _maximal_depth))
  {
    if(LOG_ENABLED(logMorph, DEBUG)) debug_morph(prev_results);

    list<tMorphAnalysis> current_results;
    for(list<tMorphAnalysis>::iterator it = prev_results.begin();
        it != prev_results.end(); ++it)
    {
      analyze1(*it, current_results);
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
  for(int i = 0; i < ninflrs; i++) {
    int t = dmp.undump_int();
    char *r = dmp.undump_string();

    if(r == 0)
      continue;
      
    if(t == -1)
      _morph.add_global(string(r));
    else {
      grammar_rule *rule = Grammar->find_rule(t);
      if (rule == NULL) {
        throw tError( ((string) "Error: type `") + print_name(t) + 
                      "' with infl annotation `" + r +
                      "' doesn't correspond to any of the parser's rules.\n" +
                      "Check the type status, it has to be one of the " +
                      "lexrule-status-values."
                      );
      }
      if(rule->trait() == SYNTAX_TRAIT) {
        throw tError( ((string) "error: found syntax rule `") + print_name(t)
                      + "' with infl annotation `" + r + "'\n" +
                      "Check the type status, it has to be one of the " +
                      "lexrule-status-values instead.");
      }
      _morph.add_rule(rule, string(r));
      rule->trait(INFL_TRAIT);
    }
    delete[] r;
  }                      

  _morph.initialize_lexrule_filter();

  LOG(logMorph, DEBUG, ", " << ninflrs << " infl rules" << _morph) ;
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
          LOG(logMorph, WARN, "Ignoring entry with unknown rule `" 
              << infl << "' in irregular forms");
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

  // \todo vvvv this should go into grammar-dump
  dmp.seek(0);
  int version;
  undump_header(&dmp, version);
  dump_toc toc(&dmp);
  //  ^^^^^^^^^

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

void debug_ffmorph(rulelist &affixes, lex_stem * lstem, int offset, bool found){
  ostringstream cdeb;
  cdeb << "(";
  for(ruleiter affix = affixes.begin(); affix != affixes.end(); affix++) {
    cdeb << (*affix)->printname() << "@" << offset << " ";
  }
  cdeb << "(" << lstem << ")";            
  for(int i = 0; i < lstem->length(); i++)
    cdeb << " \"" << lstem->orth(i) << "\"";
  
  cdeb << ")";
  if (found) cdeb << "dupl" ;
            
  LOG(logMorph, DEBUG, cdeb.str());
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
        
        rulelist affixes;
        if (affix != -1) {
          grammar_rule *rule = Grammar->find_rule(affix);
          if (rule == NULL) {
            LOG(logMorph, WARN, "rule `" << print_name(affix) 
                << "' in fullform definition `" << s << "':`" 
                << print_name(preterminal) 
                << "' doesn't correspond to any of the parser's rules" << endl);
            continue;
          } else {
            affixes.push_front(rule);
          }
        }
          
        // _fix_me_ I'm not sure about what that offset really means.
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

        if(LOG_ENABLED(logMorph, DEBUG)) 
          debug_ffmorph(affixes, lstem, offset, found);
          
      } else {
        invalid++;
      }

      delete[] s;
    }

  LOG(logMorph, DEBUG, nffs << " full form entries");
  if (invalid > 0)
    LOG(logMorph, DEBUG, invalid << " of them invalid");
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
                , jt->rules().front()->printname()
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
