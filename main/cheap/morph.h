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

#ifndef _MORPH_H_
#define _MORPH_H_

class morph_analysis
{
 public:
  morph_analysis() {}

  morph_analysis(list<string> forms, list<type_t> rules)
    : _forms(forms), _rules(rules)
    {}

  list<string> &forms() { return _forms; }

  string base() { return _forms.front(); }
  string complex() { return _forms.back(); }

  list<type_t> &rules() { return _rules; }

  void print(FILE *f);
  void print_lkb(FILE *f);

 private:
  list<string> _forms;
  list<type_t> _rules;
};

class morph_analyzer
{
 public:
  morph_analyzer();
  ~morph_analyzer();

  void add_global(string rule);
  void add_rule(type_t t, string rule);
  void add_irreg(string stem, type_t t, string form);

  bool empty();  

  void set_irregular_only(bool b)
    { _irregs_only = b; }

  class morph_letterset *letterset(string name);
  void undo_letterset_bindings();

  list<morph_analysis> analyze(string form);
  string generate(morph_analysis);

  void print(FILE *f);

 private:
  void parse_rule(type_t t, string rule, bool suffix);

  list<morph_analysis> analyze1(morph_analysis form);
  bool matching_irreg_form(morph_analysis a);

  void add_subrule(class morph_subrule *sr) 
    { _subrules.push_back(sr); }

  class morph_lettersets *_lettersets;
  class morph_trie *_suffixrules;
  class morph_trie *_prefixrules;

  vector<class morph_subrule *> _subrules;

  bool _irregs_only;

  multimap<string, morph_analysis *> _irregs_by_stem;
  multimap<string, morph_analysis *> _irregs_by_form;

  friend class morph_trie;
};

#endif
