/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 *   2004 Bernd Kiefer kiefer@dfki.de
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

#include "pet-config.h"
#include "lexparser.h"
#include "position-mapper.h" 
#include "chart.h"
#include "task.h"
#include "tsdb++.h"
#include "item-printer.h"
#include "cheap.h"

#include <iostream>

lex_parser global_lexparser;
lex_parser &Lexparser = global_lexparser;
extern chart *Chart;
extern tAgenda *Agenda;

// functions from parse.cpp
extern void add_item(tItem *it);
extern void parse_loop(fs_alloc_state &FSAS, list<tError> &errors);
extern void postulate(tItem *passive);

/** Perform lexparser initializations.
 *  - Set carg modification (surface string) path
 */
// _fix_me_
void lex_parser::init() {
  _carg_path = cheap_settings->value("mrs-carg-path");
}

string get_xml_input(string input) {
  string buffer = input;
  const int bufsize = 2048;
  char *inbuf = new char[bufsize];
  inbuf[bufsize - 1] = '\0';
  int onelinecount;
  bool partialread;
  
  do {
    onelinecount = 0;
    // Now read one line, maybe in several pieces if it is longer than the
    // buffer
    do {
      partialread = false;
      cin.getline(inbuf, bufsize - 1, '\n');
      onelinecount += cin.gcount() - 1;
      buffer += inbuf ;
      if (cin.fail()) {
        cin.clear(cin.rdstate() & ~ios::failbit);
        partialread = true;
      } else {
        buffer += '\n';
      }
    } while (partialread);  // line too long, only read partially?
    // exit if we read an empty line or we got an end_of_file
  } while ((onelinecount > 0) && cin) ;
  return buffer;
}

/** Use the registered tokenizer(s) to tokenize the input string and put the
 *  result into \a tokens.
 *
 * At the moment, it seems unlikely that more than one tokenizer should be
 * registered. Nevertheless, the structure to have multiple tokenizers is
 * already there.
 */
void lex_parser::tokenize(string input, inp_list &tokens) {
  if (_tokenizers.empty())
    throw tError("No tokenizer registered");

  // if input mode is XML, consume all the XML input
  if (_tokenizers.front()->description() == "XML input chart reader")
    input=get_xml_input(input);

  // trace output
  if(verbosity > 4)
    {
      cerr << "tokenizer = " << _tokenizers.front()->description() << endl;
	cerr << "tokenizer input:" << endl << input << endl << endl;
    }

  _tokenizers.front()->tokenize(input, tokens);
  
  // trace output
  if(verbosity > 4)
    {
    cerr << "tokenizer output:" << endl ;
      for(inp_list::iterator r = tokens.begin();
  	  r != tokens.end(); ++r)
  	  (*r)->print(ferr);      
    }
 
}

/** Call the registered taggers which add their results to the individual
 *  \a tokens.
 */
void lex_parser::tag(string input, inp_list &tokens) {
  if (! _taggers.empty()) {
    _taggers.front()->compute_tags(input, tokens);
  }
}

/** Call the registered NE recognizers which add their results to \a tokens.
 */
void lex_parser::ne_recognition(string input, inp_list &tokens) {
  if (! _ne_recogs.empty()) {
    _ne_recogs.front()->compute_ne(input, tokens);
  }
}

/** Consider lexica, some parallel and some in a chain of responsibility.
 *
 * The lexica are an ordered list with increasing level. All lexica on the
 * same level are consulted in parallel, i.e., all results are merged into
 * the result list. Lexica on a higher level (lower priority) are only
 * considered if no lexicon on a lower level delivered a result.
 */
list<lex_stem *> lex_parser::get_lex_entries(string stem) {
  if (_lexica.empty())
    throw tError("No lexicon registered");
  return call_resp_chain<lex_stem *>(_lexica.begin(), _lexica.end(), stem);
}

/** Do a morphological analysis of \a form and return the results.
 *
 * The morphological analyzers are called in the same way as the lexical
 * \see get_lex_entries for an explanation.
 */
list<tMorphAnalysis> lex_parser::morph_analyze(string form) {
  if (_morphs.empty())
    throw tError("No morphology registered");
  return call_resp_chain<tMorphAnalysis>(_morphs.begin(), _morphs.end(), form);
}


/** Add a fs modification for the surface form to the given item. This is
 *  only done for generic entries and input items whose HPSG class is
 *  pre-determined (which are presumably externally computed named entities).
 */
// _fix_me_
void lex_parser::add_surface_mod(const string &carg, modlist &mods) {
#ifdef DYNAMIC_SYMBOLS
  if (_carg_path != NULL) {
    // _fix_me_ are these double quotes necessary?
    string s = '"' + carg + '"';
    mods.push_back(pair<string, int>(_carg_path, lookup_symbol(s.c_str())));
  }
#endif
}

/** Add a newly created \c tLexItem either to the global chart (if it is
 *  passive), or to our private mini chart if it still lacks arguments in form
 *  of input items, and in this case, create the appropriate tasks.
 */
void
lex_parser::add(tLexItem *lex) {
  if(lex->passive()) {
    // add new item to the global chart
    add_item(lex);  // from parse.cpp
    stats.words++;  // we count these for compatibility, not the ones after
                    // lexical processing
    // _fix_me_ Berthold says, this is not the right number
  } else {
    // put the lex item onto the lex_parser's private "chart" of active entries
    // in principle, the chart is not necessary because all input items are on
    // the global chart already when lex parsing starts and there are no
    // generics that are multi word entries
    if (lex->left_extending()) {
      _active_left[lex->start()].push_back(lex);
    } else {
      _active_right[lex->end()].push_back(lex);
    }
    chart_iter_adj_passive pass(Chart, lex);
    while(pass.valid()) {
      // we cannot check compatiblity via type, we have to have an
      // input-trait too :(
      if (pass.current()->trait() == INPUT_TRAIT) {
        _agenda.push(new lex_and_inp_task(lex
                               , dynamic_cast<tInputItem *>(pass.current())));
      }
      pass++;
    }
  }
}


/** This function creates new tLexItems based on an input item, its
 *  corresponding lex_stem (lexicon entry) and morphological information.
 * Instantiate the \a stem, add the modifications from \a i_item and create a
 * new tLexItem with inflectional rules \a infl_rules, if this succeeds.
 */
void 
lex_parser::combine(lex_stem *stem, tInputItem *i_item
                    , const list_int *infl_rules, const modlist &mods) {
  // _fix_me_
  fs newfs = stem->instantiate();
  // Add modifications coming from the input
  newfs.modify_eagerly(mods);

  if (newfs.valid()) {
    add(new tLexItem(stem, i_item, newfs, infl_rules));
  }
}


/** Add the input item to the global chart and combine it with the
 * appropriate lexical units, eventually applying morphological processing
 * beforehand.
 *
 * If one of the lexical entries has more than one argument, i.e., if it is a
 * multi-word (MW) entry, the inflected argument must be the first to be
 * filled.
 */
void
lex_parser::add(tInputItem *inp) {
  // add item to the global chart
  Chart->add(inp);
  // check if we have to perform morphology and lexicon access
  if (inp->is_word_token()) {
    // if so, call morphological analysis and lexical lookup and add
    // lex_and_inp_tasks for the resulting tLexItems to the
    // agenda. get_lex_entries eventually looks into several lexica.
    list<tMorphAnalysis> morphs = morph_analyze(inp->form());
    bool nullmorpheme = false;
    for(list<tMorphAnalysis>::iterator mrph = morphs.begin()
          ; mrph != morphs.end(); mrph++) {
      if (inp->form() == mrph->base()) { 
        nullmorpheme = true;
      }
      list<lex_stem *> stems = get_lex_entries(mrph->base());      
      for(list<lex_stem *>::iterator it = stems.begin()
            ; it != stems.end(); it++) {
        list_int *rules = copy_list(mrph->rules());
        combine(*it, inp, rules, inp->mods());
        free_list(rules);
      }
    }
    // If there is no morph analysis with null inflection, do additional lookup
    // based on the input form, e.g., for multi word entries without inflected
    // position.
    // _fix_me_ is this okay?
    if (! nullmorpheme) {
      list<lex_stem *> stems = get_lex_entries(inp->form());
      for(list<lex_stem *>::iterator it = stems.begin()
            ; it != stems.end(); it++){
        combine(*it, inp, NULL, inp->mods());
      }
    }
  } else {
    if (inp->is_stem_token()) {
      // morphology has been done already, just do the lexicon lookup
      list<lex_stem *> stems = get_lex_entries(inp->stem());
      for(list<lex_stem *>::iterator it = stems.begin()
            ; it != stems.end(); it++) {
        combine(*it, inp, inp->inflrs(), inp->mods());
      }
    } else {
      // otherwise, we got the class of the lexical stem by some internal
      // mapping: create a new tLexItem directly from the input item
      lex_stem *stem = Grammar->find_stem(inp->tclass());
      // in this case, add a CARG modification to the input item
      // _fix_me_
      add_surface_mod(inp->orth(), inp->mods());
      // Check if there is an appropriate lexicon entry
      if (stem != NULL) {
        combine(stem, inp, inp->inflrs(), inp->mods());
      }
    } // end if is_stem_token
  } // end if is_word_token
}

/** A binary Predicate that is used to sort a list of input items such that the
 * list is a topological order of the underlying item graph.
 */
struct tInputItem_position_less 
  : public binary_function<tInputItem *, tInputItem *, bool>{
  bool operator()(const tInputItem *a, const tInputItem *b) {
    return ((a->startposition() < b->startposition())
            || ((a->startposition() == b->startposition())
                && (a->endposition() < b->endposition())));
  }
};


/** Remove unrecognized tokens and close gaps that result from this deletion or
 *  gaps in the input positions.
 *  \param counts if \c true, the positions of the input tokens are counts
 *                rather than positions, i.e., a word with length one has
 *                positions (j, j) rather than (j, j+1)
 */
int lex_parser::map_positions(inp_list &tokens, position_map position_mapping) {
  int maxend = 0;
  if (position_mapping==NO_POSITION_MAP) {
    for(inp_iterator it = tokens.begin(); it != tokens.end(); it++) {
      maxend=max(maxend,(*it)->end());
    }
    return maxend;
  }

  position_mapper posmapper((position_mapping == STANDOFF_COUNTS ? true : false));

  // first delete all skip tokens
  tokens.remove_if(mem_fun(&tInputItem::is_skip_token));

  for(inp_iterator it = tokens.begin(); it != tokens.end(); it++) {
    posmapper.add(*it);
  }
  maxend = posmapper.map_to_chart_positions();
  return maxend;
}



void
lex_parser::dependency_filter(struct setting *deps, bool unidirectional
                              , bool lex_exhaustive) {
  if(deps == 0 || opt_chart_man == false)
    return;

  vector<set <int> > satisfied(deps->n);
  multimap<tItem *, pair <int, int> > requires;

  tItem *lex;
  fs f;

  for(chart_iter iter(Chart); iter.valid(); iter++) {
    lex = iter.current();
    // the next condition depends on whether we did exhaustive lexical 
    // processing or not
    if (lex->passive()
        && lex->trait() != INPUT_TRAIT
        && (!lex_exhaustive || lex->inflrs_complete_p())) {
      f = lex->get_fs();
    
      if(verbosity > 4)
        fprintf(fstatus, "dependency information for %s:\n",
                lex->printname());
    
      for(int j = 0; j < deps->n; j++) {
        fs v = f.get_path_value(deps->values[j]);
        if(v.valid()) {
          if(verbosity > 4)
            fprintf(fstatus, "  %s : %s\n", deps->values[j], v.name());

          if(!unidirectional || j % 2 != 0) {
            satisfied[j].insert(v.type());
          }
        
          if(!unidirectional || j % 2 == 0) {
            requires.insert(make_pair(lex,
                                      make_pair((j % 2 == 0)
                                                ? j + 1 : j - 1,
                                                v.type())));
          }
        }
      }
    }
  }
  
  hash_set<tItem *> to_delete;
  for(chart_iter iter2(Chart); iter2.valid(); iter2++) {
    lex = iter2.current();
    // XXX _fix_me_ the next condition might depend on whether we did
    // exhaustive lexical processing or not
    if (lex->passive() && (lex->trait() != INPUT_TRAIT)) {

      pair<multimap<tItem *, pair<int, int> >::iterator,
        multimap<tItem *, pair<int, int> >::iterator> eq =
        requires.equal_range(lex);

      bool ok = true;
      for(multimap<tItem *, pair<int, int> >::iterator dep = eq.first;
          ok && dep != eq.second; ++dep) {
        // we have to resolve a required dependency
        pair<int, int> req = dep->second;
        if(verbosity > 4)
          fprintf(fstatus, "`%s' requires %s at %d -> ",
                  lex->printname(),
                  type_name(req.second), req.first);
       
        bool found = false;
        for(set<int>::iterator it2 = satisfied[req.first].begin();
            it2 != satisfied[req.first].end(); ++it2) {
          if(glb(*it2, req.second) != -1)
            {
              if(verbosity > 4)
                fprintf(fstatus, "[%s]", type_name(*it2));
              found = true;
              break;
            }
        }
       
        if(!found) {
          ok = false;
          stats.words_pruned++;
        }

        if(verbosity > 4)
          fprintf(fstatus, "%s satisfied\n", ok ? "" : "not");
      }
    
      if(!ok)
        to_delete.insert(iter2.current());
    }
  }

  Chart->remove(to_delete);
}

typedef list< pair<int, int> > gaplist;



namespace HASH_SPACE {
  /** hash function for pointer that just looks at the pointer content */
  template<> struct hash< tInputItem * > {
    /** \return A hash code for a pointer */
    inline size_t operator()(tInputItem *key) const {
      return (size_t) key;
    }
  };
}

/** \brief This predicate should be used in find_unexpanded if lexical
 *  processing is not exhaustive. All non-input items are valid.
 */
struct valid_non_input : public item_predicate {
  virtual ~valid_non_input() {}

  virtual bool operator()(tItem *item) {
    return item->trait() != INPUT_TRAIT;
  }
};

/** \brief This predicate should be used in find_unexpanded if lexical
 *  processing is exhaustive. All items that are not input items and have
 *  satisified all inflection rules are valid.
 */
struct valid_lex_complete : public item_predicate {
  virtual ~valid_lex_complete() {}

  virtual bool operator()(tItem *item) {
    return (item->trait() != INPUT_TRAIT) && (item->trait() != INFL_TRAIT);
  }
};

void
mark_recursive(tItem *item, hash_set< tItem * > &checked
               , hash_map< tInputItem *, bool > &expanded){
  checked.insert(item);
  const list<tItem *> &dtrs = item->daughters();
  for(list<tItem *>::const_iterator it=dtrs.begin(); it != dtrs.end(); it++){
    tInputItem *inp = dynamic_cast<tInputItem *>(*it);
    if (inp != NULL) {
      expanded[inp] = true;
    } else {
      if(checked.find(*it) == checked.end()) {
        mark_recursive(*it, checked, expanded);
      }
    }
  }
}

/** Find all input items that did not expand into an item that is useful for
 *  further processing (according to \a valid_item).
 *
 * Adding generics for all the returned items results should be the most robust
 * alternative, since it does not take into account covered/uncovered regions
 * (in terms of chart positions).
 */
list<tInputItem *>
find_unexpanded(chart *ch, item_predicate &valid_item) {
  hash_set< tItem * > checked;
  hash_map< tInputItem *, bool > expanded;

  // only passive items, topological order is not relevant
  chart_iter_topo iter(ch);
  while(iter.valid()) {
    tInputItem *inp = dynamic_cast<tInputItem *>(iter.current());
    if (inp != NULL) {
      if (expanded.find(inp) == expanded.end())
        expanded[inp] = false;
    } else {
      // did we consider this item already?
      if(checked.find(iter.current()) == checked.end()) {
        if (valid_item(iter.current())) {
          // walk down the daughters links
          mark_recursive(iter.current(), checked, expanded);
        }
      }
    }
    iter++;
  }
  
  // collect all unexpanded input items
  list<tInputItem *> result_list;
  for(hash_map< tInputItem *, bool >::iterator it = expanded.begin()
        ; it != expanded.end(); it++) {
    if(! it->second) 
      result_list.push_back(it->first);
  }
  return result_list;
}

void
lex_parser::add_generics(list<tInputItem *> &unexpanded) {
  list< lex_stem * > gens;

  if(verbosity > 4)
    fprintf(ferr, "adding generic les\n");

  for(list<tInputItem *>::iterator it = unexpanded.begin()
        ; it != unexpanded.end(); it++) {
    if(verbosity > 4) {
      fprintf(ferr, "  token ");
      (*it)->print(ferr);
      fprintf(ferr, "\n");
    }

    if ((! (*it)->parents.empty())
        && cheap_settings->lookup("pos-completion")) {
      postags missing((*it)->get_in_postags());

      if(verbosity > 4) {
        fprintf(ferr, "    token provides tags:");
        missing.print(ferr);
        fprintf(ferr, "\n    already supplied:");
        postags((*it)->parents).print(ferr);
        fprintf(ferr, "\n");
      }

      missing.remove(postags((*it)->parents));

      if(verbosity > 4) {
        fprintf(ferr, "    -> missing tags:");
        missing.print(ferr);
        fprintf(ferr, "\n");
      }
            
      if(!missing.empty())
        gens = (*it)->generics(missing);
    }
    else {
      gens = (*it)->generics();
    }

    for(list<lex_stem *>::iterator ls = gens.begin()
          ; ls != gens.end(); ls++) {
      modlist in_mods = (*it)->mods();
      // _fix_me_
      add_surface_mod((*it)->orth(), in_mods);
      // TODO: is it sensible here to apply the (guessed) inflection rules here
      // to the generics??
      combine(*ls, *it, (*it)->inflrs(), in_mods);
    }
  }
}

void 
lex_parser::reset() {
  // The items were created with an active default_owner and because of this
  // are correctly destroyed together with the global chart.
  for(int i = 0; i <= _maxpos; i++) {
    _active_left[i].clear();
    _active_right[i].clear();
  }
  _maxpos = -1;
}

int
lex_parser::process_input(string input, inp_list &inp_tokens) {
  // Tokenize the input
  tokenize(input, inp_tokens);

  // Attach POS tags to the input 
  tag(input, inp_tokens);

  // NE recognition has to care about morphology itself
  ne_recognition(input, inp_tokens);

  // map the input positions into chart positions
  position_map position_mapping = _tokenizers.front()->position_mapping();
  _maxpos = map_positions(inp_tokens, position_mapping);

  return _maxpos;
}


void
lex_parser::lexical_processing(inp_list &inp_tokens, bool lex_exhaustive
                               , fs_alloc_state &FSAS, list<tError> &errors) {
  // if opt_lex_exhaustive, process inflectional and lexical rules first and
  // exhaustively before applying syntactic rules
  // This allows more elaborate checking of gaps and chart dependencies
  if (lex_exhaustive) {
    Grammar->activate_lex_rules();
  } else {
    Grammar->deactivate_all_rules();
  }

  _active_left.resize(_maxpos + 1);
  _active_right.resize(_maxpos + 1);

  // now initialize agenda of lexical parser with list of input tokens
  // and start lexical processing with application of lex entries and lex rules
  for(inp_iterator it=inp_tokens.begin(); it != inp_tokens.end(); it++) {
    add(*it);
  }

  while (! _agenda.empty()) {
    _agenda.front()->execute(*this);
    _agenda.pop();
  }

  if (lex_exhaustive) {
    parse_loop(FSAS, errors);
  }

  // exhaustively apply lexical and inflection rules
  // dependency filtering is done on the chart itself
  dependency_filter(cheap_settings->lookup("chart-dependencies"),
                    (cheap_settings->lookup(
                            "unidirectional-chart-dependencies") != 0),
                    lex_exhaustive);

  // Should gap computation precede adding generics, and generics being only
  // applied where gaps are found? It is easy to find examples where adding
  // generics at points where there is no gap may lead to a full parse that
  // would otherwise be impossible. As a consequence, it depends on whether
  // maximal robustness or fast parsing is desired.

  item_predicate *valid;
  if(lex_exhaustive) {
    valid = new valid_lex_complete();
  } else {
    valid = new valid_non_input() ;
  }

  //tTclChartPrinter chp("/tmp/lex-chart", 0);
  //Chart->print(&chp);

  list< tInputItem * > unexpanded;
  if (! Chart->connected(*valid)) {
    unexpanded = find_unexpanded(Chart, *valid) ;
  }

  // This may in principle lead to new lexical parsing, but only if generics
  // may be multi word entries.
  if(opt_default_les && ! unexpanded.empty()) {
    add_generics(unexpanded);
    unexpanded.clear();
    
    while (! _agenda.empty()) {
      _agenda.front()->execute(*this);
      _agenda.pop();
    }
    
    if (lex_exhaustive) {
      parse_loop(FSAS, errors);
    }
    if (! Chart->connected(*valid)) {
      unexpanded = find_unexpanded(Chart, *valid);
    }
  }

  // throw an error if there are unexpanded input items
  if (! unexpanded.empty()) {
    string missing = unexpanded.front()->orth();
    for(inp_iterator inp = ++unexpanded.begin()
          ; inp != unexpanded.end(); inp++) {
      missing += ", " + (*inp)->orth();
    }
    throw tError("no lexicon entries for " + missing) ;
  }
  
  if (lex_exhaustive) {
    Grammar->activate_syn_rules();
  } else {
    Grammar->activate_all_rules();
  }

  // Now we have to create the appropriate tasks for passive items on the chart
  chart_iter ci(Chart);
  while (ci.valid()) {
    if (ci.current()->passive() && (ci.current()->trait() != INPUT_TRAIT)) {
      // _fix_me_ Use item_tasks here that were used with the input chart?
      // The item tasks would add the item to the chart again. What is
      // missing here is the test for packed items, the `rootness' of each
      // item, and, in principle, fundamental_for_passive(), which will do
      // nothing since there are no active items on the chart.  (see the code
      // of add_item() in parse.cpp)
      postulate(ci.current());
    }
    ci++;
  }

}
