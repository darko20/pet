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
#include "chart-mapping.h"
#include "fs-chart.h"
#include "task.h"
#include "tsdb++.h"
#include "item-printer.h"
#include "cheap.h"
#include "list-int.h"
#include "hashing.h"
#include "sm.h"
#include "settings.h"
#include "configs.h"
#include "logging.h"
#include "parse.h"

#include <iostream>

using namespace std;
using namespace HASH_SPACE;

static lex_parser &init();

lex_parser global_lexparser;
lex_parser &Lexparser = init();
extern chart *Chart;
extern tAbstractAgenda *AbstractAgenda;

static std::string default_les_strategy_names[] = { 
  "no", "traditional", "all"
};

class DefaultLesStrategyConverter : public AbstractConverter<default_les_strategy> {
  /** Names for the corresponding strategies in enum default_les_strategy. */
  virtual ~DefaultLesStrategyConverter() {}
  virtual std::string toString(const default_les_strategy& id) {
    return default_les_strategy_names[id];
  }
  virtual default_les_strategy fromString(const std::string& s) {
    default_les_strategy id = NO_DEFAULT_LES;
    while (id < DEFAULT_LES_INVALID &&
           s.compare(default_les_strategy_names[id]) != 0) {
      id = (default_les_strategy) (id + 1);
    }
    if (id == DEFAULT_LES_INVALID) {
      LOG(logAppl, WARN, "Unknown default les strategy. Ignoring.");
      id = NO_DEFAULT_LES;
    }
    return id;
  }
};

static lex_parser &init() {
  /** @name Unknown word handling */
  //@{
  managed_opt<default_les_strategy>("opt_default_les",
    "Try to use default (generic) lexical entries if no regular entries could "
    "be found. Uses POS annotation, if available.",
    NO_DEFAULT_LES,
    new DefaultLesStrategyConverter());

  managed_opt("opt_predict_les",
    "Try to use lexical type predictor if no regular entries could "
    "be found. Use at most the number of entries specified", 0);
  //@}

  managed_opt("opt_chart_man",
    "allow/disallow chart manipulation (currently only dependency filter)",
    true);
  
  return global_lexparser;
} 

/** Perform lexparser initializations.
 *  - Set carg modification (surface string) path
 */
// _fix_me_
void lex_parser::init()
{
  const char* cp = cheap_settings->value("mrs-carg-path");
  _carg_path = cp ? cp : "";
}

void debug_tokenize(string desc, string input, inp_list &tokens)
{
  ostringstream cdeb;
  cdeb << "tokenizer = " << desc << endl
       << "tokenizer input:" << endl << input << endl << endl
       << "tokenizer output:" << endl;
  for(inp_iterator r = tokens.begin(); r != tokens.end(); ++r)
    cdeb << (*r);
  LOG(logLexproc, DEBUG, cdeb.str());
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

  _tokenizers.front()->tokenize(input, tokens);
  
  // trace output
  if(LOG_ENABLED(logLexproc, DEBUG))
    debug_tokenize(_tokenizers.front()->description(), input, tokens);
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
  if (!get_opt_int("opt_chart_mapping") && !_carg_path.empty()) {
    mods.push_back(pair<string, int>(_carg_path
                                     , retrieve_string_instance(carg)));
  }
#endif
}

/** Add a newly created \c tLexItem either to the global chart (if it is
 *  passive), or to our private mini chart if it still lacks arguments in form
 *  of input items, and in this case, create the appropriate tasks.
 */
void lex_parser::add(tLexItem *lex)
{
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

/**
 * Helper function for printing debug logs for lex_parser::combine().
 */
std::string
debug_combine(lex_stem *stem, tInputItem *i_item, const list_int *infl_rules) {
  ostringstream cdeb;
  cdeb << "combine("
       << stem->printname()
       << ", id:" << i_item->id()
       << " form:" << i_item->form()
       << " infl_rules:[";
  for(const list_int *l = infl_rules; l != 0; l = rest(l)) {
    cdeb << print_name(first(l));
    if (rest(l) != 0)
      cdeb << " ";
  }
  cdeb << "], mods) failed in creating a valid fs.";
  return cdeb.str();
}

/** This function creates new tLexItems based on an input item, its
 *  corresponding lex_stem (lexicon entry) and morphological information.
 * Instantiate the \a stem, add the modifications from \a i_item and create a
 * new tLexItem with inflectional rules \a infl_rules, if this succeeds.
 */
void 
lex_parser::combine(lex_stem *stem, tInputItem *i_item
                    , const list_int *infl_rules, const modlist &mods)
{
  /// \todo somebody thought a fix is needed here
  fs newfs = stem->instantiate();
  // Add modifications coming from the input
  newfs.modify_eagerly(mods);

   /** \todo
   *for non-MWE lexical entries, unify the input feature structure into the
   *`tokens' path.  this may fail, e.g. for generic lexical entries that put
   *constraints on their tokens.  for MWEs, however, we let the creation of
   *lexical items run its course, until lexical parsing has fully instantiated
   *the argument slots of the lexical entry.  regrettably, something similar
   *to the code below is then applied in lex_and_inp_task::execute().  i find
   *it tricky to identify the right time and place for this unification to be
   *attempted: we need all the pieces, but it must happen prior to invoking
   *the tLexItem constructor on the passive item.              (16-jan-09; oe)
   */
  
  if (stem->length() == 1 
      && tChartUtil::lexicon_tokens_path() != 0
      && tChartUtil::lexicon_last_token_path() != 0) {
    //
    // build a list of the right arity, containing the token FSs
    //
    fs list_fs(BI_CONS);
    fs nil_fs(BI_NIL);
    fs token_fs = i_item->get_fs();
    list_fs = unify(list_fs, list_fs.get_attr_value(BIA_FIRST), token_fs);
    list_fs = unify(list_fs, list_fs.get_attr_value(BIA_REST), nil_fs);
    //
    // unify token FSs into final item fs:
    //
    if (newfs.valid()) {
      fs anchor_fs = newfs.get_path_value(tChartUtil::lexicon_tokens_path());
      if (!anchor_fs.valid()) {
        LOG(logLexproc, ERROR, debug_combine(stem, i_item, infl_rules) +
            " Reason: lexicon_tokens_path cannot be resolved in fs.");
        return;
      }
      newfs = unify(newfs, anchor_fs, list_fs);
    }
    if (newfs.valid()) {
      fs anchor_fs = newfs.get_path_value(tChartUtil::lexicon_last_token_path());
      if (!anchor_fs.valid()) {
        LOG(logLexproc, ERROR, debug_combine(stem, i_item, infl_rules) +
            " Reason: lexicon_last_token_path cannot be resolved in fs.");
        return;
      }
      newfs = unify(newfs, anchor_fs, token_fs);
    }
  }

  if (newfs.valid()) {
    // LOG(logParse, DEBUG, "combine() succeeded in creating valid fs");
    add(new tLexItem(stem, i_item, newfs, infl_rules));
  } else {
    LOG(logLexproc, DEBUG, debug_combine(stem, i_item, infl_rules));
  }
}



/** Return the inflection rules that have to be applied to perform this
 *  morphological analysis.
 */
static list_int *get_rules(tMorphAnalysis &analysis) {
  const std::list<grammar_rule *> &rules = analysis.rules();
  list_int *res = NULL;
  for(std::list<grammar_rule *>::const_reverse_iterator it = rules.rbegin()
        ; it != rules.rend(); ++it) {
    res = cons((*it)->type(), res);
  }
  return res;
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
  assert(!inp->blocked());

  // add item to the global chart
  Chart->add(inp);
  
  // do a morphological analysis if necessary:
  list<tMorphAnalysis> morphs;
  //
  // _fix_me_/
  // why does DEFAULT_LES_ALL force morphological analysis?    (16-jan-09; oe)
  //
  default_les_strategy opt_default_les;
  get_opt("opt_default_les", opt_default_les);
  if (opt_default_les == DEFAULT_LES_ALL || inp->is_word_token()) {
    morphs = morph_analyze(inp->form());
  }
  
  // add all compatible generic entries for this token: 
  if (opt_default_les == DEFAULT_LES_ALL) {
    // iterate over all generic lexicon entries:
    for (list_int *gens = Grammar->generics(); gens != 0; gens = rest(gens)) {
      lex_stem *ls = Grammar->find_stem(first(gens));
      modlist in_mods = inp->mods(); // should be empty
      add_surface_mod(inp->orth(), in_mods);
      if (morphs.empty()) {
        combine(ls, inp, inp->inflrs(), in_mods);
      } else {
        for(list<tMorphAnalysis>::iterator mrph = morphs.begin()
              ; mrph != morphs.end(); mrph++) {
          list_int *rules = get_rules(*mrph);
          combine(ls, inp, rules, in_mods);
          free_list(rules);
        }
      }
    }
  }
  
  // check if we have to perform morphology and lexicon access
  if (inp->is_word_token()) {
    // if so, call morphological analysis and lexical lookup and add
    // lex_and_inp_tasks for the resulting tLexItems to the
    // agenda. get_lex_entries eventually looks into several lexica.
    bool nullmorpheme = false; // any of the morphs was a null morpheme
    for(list<tMorphAnalysis>::iterator mrph = morphs.begin()
          ; mrph != morphs.end(); mrph++)
    {
      // update flag whether any of the morphs was a null morpheme:
      nullmorpheme = nullmorpheme || (inp->form() == mrph->base());
      // instantiate all stems with this morphological analysis:
      list<lex_stem *> stems = get_lex_entries(mrph->base());      
      for(list<lex_stem *>::iterator it = stems.begin()
            ; it != stems.end(); it++) {
        list_int *rules = get_rules(*mrph);
        combine(*it, inp, rules, inp->mods());
        free_list(rules);
      }
    }
    // If there is no morph analysis with null inflection, do additional lookup
    // based on the input form, e.g., for multi word entries without inflected
    // position.
    // _fix_me_ is this okay?
    if (! nullmorpheme) { // none of the analyses was a null morpheme
      list<lex_stem *> stems = get_lex_entries(inp->form());
      for(list<lex_stem *>::iterator it = stems.begin()
            ; it != stems.end(); it++){
        combine(*it, inp, NULL, inp->mods());
      }
    }
  } else if (inp->is_stem_token()) {
      // morphology has been done already, just do the lexicon lookup
      list<lex_stem *> stems = get_lex_entries(inp->stem());
      for(list<lex_stem *>::iterator it = stems.begin()
            ; it != stems.end(); it++) {
        combine(*it, inp, inp->inflrs(), inp->mods());
      }
  } else { // neither word nor stem
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
  }
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
lex_parser::dependency_filter(setting *deps, bool unidirectional, bool lex_exhaustive)
{
  if(deps == 0 || !get_opt_bool("opt_chart_man"))
    return;

  vector<set <int> > satisfied(deps->n());
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
    
      LOG(logLexproc, DEBUG, "dependency information for " << 
          lex->printname() << ":");
    
      for(int j = 0; j < deps->n(); j++) {
        fs v = f.get_path_value(deps->values[j].c_str());
        if(v.valid()) {
          LOG(logLexproc, DEBUG, "  " << deps->values[j] << " : " << v.name());

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
  
  unordered_set<tItem *> to_delete;
  for(chart_iter iter2(Chart); iter2.valid(); iter2++) {
    lex = iter2.current();
    // _fix_me_ the next condition might depend on whether we did
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
        LOG(logLexproc, DEBUG, "`" << lex->printname() << "' requires "
            << type_name(req.second) << " at " << req.first << " -> ");
       
        bool found = false;
        for(set<int>::iterator it2 = satisfied[req.first].begin();
            it2 != satisfied[req.first].end(); ++it2) {
          if(glb(*it2, req.second) != -1)
            {
              LOG(logLexproc, DEBUG, "[" << type_name(*it2) << "]");
              found = true;
              break;
            }
        }
       
        if(!found) {
          ok = false;
          stats.words_pruned++;
        }

        LOG(logLexproc, DEBUG, (ok ? "" : "not") << " satisfied");
      }
    
      if(!ok)
        to_delete.insert(iter2.current());
    }
  }

  Chart->remove(to_delete);
}

typedef list< pair<int, int> > gaplist;

void mark_recursive(tItem *item, unordered_set< tItem * > &checked,
               unordered_map< tInputItem *, bool > &expanded)
{
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
find_unexpanded(chart *ch, item_predicate valid_item) {
  unordered_set< tItem * > checked;
  unordered_map< tInputItem *, bool > expanded;

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
  for(unordered_map< tInputItem *, bool >::iterator it = expanded.begin()
        ; it != expanded.end(); it++) {
    if(! it->second) {
      bool covered = false;
      //
      // when working from a token lattice, it may be legitimate to not have
      // instantiated _all_ input items, as long as for each chart cell there
      // is at least one valid lexical item.  
      // _fix_me_
      // for now, make this dependent on whether or not chart mapping is used,
      // but probably it should be a more general option.      (25-sep-08; oe)
      //
      // after emailing the PET co-developers about this, they all nodded their
      // heads in perfect silence.  hence, i now think we always want the more
      // restrictive (aka focused) reporting.                  (10-nov-08; oe)
      //
      chart_iter_span_passive 
        coverage(ch, it->first->start(), it->first->end());
      while(coverage.valid()) {
        if (valid_item(coverage.current())) {
          covered = true;
          break;
        }
        coverage++;
      } // while
      if (!covered) result_list.push_back(it->first);
    } // if
  }
  return result_list;
}

void
lex_parser::add_generics(list<tInputItem *> &unexpanded) {
  list< lex_stem * > gens;

  LOG(logLexproc, DEBUG, "adding generic les");
 
  for(inp_iterator it = unexpanded.begin()
        ; it != unexpanded.end(); it++) {

    // debug messages
    LOG(logLexproc, DEBUG, "  token " << (*it) << endl);

    // instantiate gens
    if ((! (*it)->parents.empty())
        && cheap_settings->lookup("pos-completion")) {
      // optional pos-completion mode

      postags missing((*it)->get_in_postags());

      // debug messages
      LOG(logLexproc, DEBUG, "    token provides tags:" << missing << endl
          << "    already supplied:" << postags((*it)->parents) << endl);

      missing.remove(postags((*it)->parents));

      // debug messages
      LOG(logLexproc, DEBUG, "    -> missing tags:" << missing << endl);
            
      if(!missing.empty())
        gens = (*it)->generics(missing);
    }
    else {
      gens = (*it)->generics();
    }

    // the input tokens which lead to generics must be considered a word_token,
    // because otherwise, the precomputed stem would not lead to a lexicon
    // entry (bad) or the precomputed type name would not exist (even worse)
    //
    // _fix_me_
    // why would it be bad for a generic entry to have a stem for which there
    // is no lexical entry?  in a sense, that is exactly what a generic entry
    // is meant to provide.  for the SRG, there is an external morphology that
    // does lemmatization and inflectional analysis.  hence, all input tokens
    // carry a list of orthographemic rules (the infamous `_inflrs_todo').  it
    // may be the case that, for a given stem, there is no lexical entry.  in
    // that case, we want to activate generic entries (in theory that could be
    // based on additional PoS tags), and those entries should be expected to
    // go through the orthographemic rules required on the token.  hence, the
    // invocation of morph_analyze() is now restricted to `word' tokens.
    //                                                          (12-sep-08; oe)
    //
    if (!gens.empty()) {
      /* \todo is it sensible here to apply the (guessed) inflection rules here
      * to the generics??
      *
      * re-do morphology computation. 
      * \todo check if this uses up significant processing time. There is
      * another way to do this (store the previous result in the tInputItem),
      * but that messes up the whole failed stem token thing and all
      *
      * get morphs
      */
      list<tMorphAnalysis> morphs;
      if((*it)->is_word_token()) morphs = morph_analyze((*it)->form());

      // iterate thru gens
      for(list<lex_stem *>::iterator ls = gens.begin()
            ; ls != gens.end(); ls++) {
        // get mods
        modlist in_mods = (*it)->mods();
        // _fix_me_
        add_surface_mod((*it)->orth(), in_mods);
        if (morphs.empty()) { /// \todo as far as I can see, this will never be the case (pead)
          combine(*ls, *it, (*it)->inflrs(), in_mods);
        } else {
          for(list<tMorphAnalysis>::iterator mrph = morphs.begin()
                ; mrph != morphs.end(); mrph++) {
            list_int *rules = get_rules(*mrph);
            combine(*ls, *it, rules, in_mods);
            free_list(rules);
          }
        }
      }
    }
  }
}

list<lex_stem *>
predict_les(tInputItem *item, list<tInputItem*> &inp_tokens, int n) {
   list<lex_stem*> results;
  if (Grammar->lexsm()) {
    vector<string> words(5);
    
    words[4] = item->orth();
    vector<vector<type_t> > types(4);
    for (inp_iterator it = inp_tokens.begin(); it != inp_tokens.end(); it ++) {
      int idx = -1;
      if ((*it)->endposition() == item->startposition() - 1)
        idx = 0;
      if ((*it)->endposition() == item->startposition())
        idx = 1;
      if ((*it)->startposition() == item->endposition())
        idx = 2;
      if ((*it)->startposition() == item->endposition() + 1)
        idx = 3;
      if (idx != -1) {
        words[idx] = (*it)->orth();
        chart_iter_topo iter(Chart);
        while (iter.valid()) {
          tLexItem *lex = dynamic_cast<tLexItem*>(iter.current());
          if (lex != NULL)
            for (list<tItem*>::const_iterator dtrit = lex->daughters().begin();
                 dtrit != lex->daughters().end(); dtrit ++)
              if (*it == *dtrit)
                types[idx].push_back(lex->identity());
          iter ++; 
        }
      }
    }
    list<type_t> pred_types = Grammar->lexsm()->bestPredict(words,types, n);
    for (list<type_t>::iterator tit = pred_types.begin();
         tit != pred_types.end(); tit ++) 
      results.push_back(Grammar->find_stem(*tit));
  }
    
  return results; 
}


void debug_predictions(inp_list &unexpanded) {
  ostringstream cdeb;
  cdeb << "adding prediction les" << endl;
  for (inp_iterator it = unexpanded.begin(); it != unexpanded.end(); it ++) {
    cdeb << "  token " << *it << endl;
  }
  LOG(logLexproc, DEBUG, cdeb.str());
}

void 
lex_parser::add_predicts(inp_list &unexpanded, inp_list &inp_tokens,
                         int nr_predicts) {
  list<lex_stem*> predicts;
  
  if(LOG_ENABLED(logLexproc, DEBUG)) debug_predictions(unexpanded);
  
  for (inp_iterator it = unexpanded.begin(); it != unexpanded.end(); it ++) {
    predicts = predict_les(*it, inp_tokens, nr_predicts);
    list<tMorphAnalysis> morphs = morph_analyze((*it)->form());
    for (list<lex_stem*>::iterator ls = predicts.begin();
         ls != predicts.end(); ls ++) {
      modlist in_mods = (*it)->mods();
      add_surface_mod((*it)->orth(), in_mods);
      if (morphs.empty()) {
        combine(*ls, *it, (*it)->inflrs(), in_mods);
      } else {
        for (list<tMorphAnalysis>::iterator mrph = morphs.begin()
               ; mrph != morphs.end(); mrph++) {
          list_int *rules = get_rules(*mrph);
          combine(*ls, *it, rules, in_mods);
          free_list(rules);
        }
      }
    }
  }
}

void lex_parser::reset()
{
  // The items were created with an active default_owner and because of this
  // are correctly destroyed together with the global chart.
  for(int i = 0; i <= _maxpos; i++) {
    _active_left[i].clear();
    _active_right[i].clear();
  }
  _maxpos = -1;
}


bool lex_parser::next_input(std::istream &in, std::string &result) {
  if (_tokenizers.empty())
    throw tError("No tokenizer registered");
  
  return _tokenizers.front()->next_input(in, result);
}

int
lex_parser::process_input(string input, inp_list &inp_tokens, bool chart_mapping) {
  /// \todo get rid of this logging control; use proper logging instead
  int chart_mapping_loglevel = get_opt_int("opt_chart_mapping");
  
  // Tokenize the input
  tokenize(input, inp_tokens);
  
  // Attach POS tags to the input 
  tag(input, inp_tokens);
  
  // NE recognition has to care about morphology itself
  ne_recognition(input, inp_tokens);
  
  // map the input positions into chart positions
  position_map position_mapping = _tokenizers.front()->position_mapping();
  _maxpos = map_positions(inp_tokens, position_mapping);
  
  // token mapping:
  if (chart_mapping) {
    if (LOG_ENABLED(logChartMapping, NOTICE) || chart_mapping_loglevel & 1) {
      fprintf(stderr, "[cm] token mapping starts\n");
    }
    // map to tChart:
    tChart chart;
    tItemPrinter ip(cerr, false, true);
    tChartUtil::map_chart(inp_tokens, chart);
    if (LOG_ENABLED(logChartMapping, INFO) || chart_mapping_loglevel & 4) {
      fprintf(stderr, "[cm] initial token chart:\n");
      chart.print(cerr, &ip, passive_unblocked);
    } // if
    // apply chart mapping rules:
    const std::list<class tChartMappingRule*> &rules = Grammar->tokmap_rules();
    tChartMappingEngine mapper(rules, "token mapping");
    mapper.process(chart);
    // map back:
    _maxpos = tChartUtil::map_chart(chart, inp_tokens);
    // chart and chart vertices memory is released by ~tChart 
    // chart items should be handled by tItem::default_owner()
    if (LOG_ENABLED(logChartMapping, INFO) || chart_mapping_loglevel & 4) {
      fprintf(stderr, "[cm] final token chart:\n");
      chart.print(cerr, &ip, passive_unblocked);
    } // if
    if (LOG_ENABLED(logChartMapping, NOTICE) || chart_mapping_loglevel & 1) {
      fprintf(stderr, "[cm] token mapping ends\n");
    }
  }
  
  return _maxpos;
}

void
lex_parser::lexical_parsing(inp_list &inp_tokens,
                            bool chart_mapping, bool lex_exhaustive, 
                            fs_alloc_state &FSAS, list<tError> &errors){
  /// \todo get rid of this logging control; use proper logging instead
  int chart_mapping_loglevel = get_opt_int("opt_chart_mapping");
  
  // if lex_exhaustive, process inflectional and lexical rules first and
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
    // add all input items that have not been deleted in token mapping:
    if (!(*it)->blocked()) { 
    add(*it);
  }
  }

  //
  // when in chart mapping mode, we do not want packing during lexical parsing
  // for two reasons: (a) lexical filtering can only apply once we have reached
  // a fix-point in lexical parsing, and if there were packed edges alrady, the
  // filtering becomes unpredictable; (b) in the current code, at least, there
  // is a provision to map vertices from the token to the `real' chart (where
  // for example two adjacent tokens may have been combined, such that 0:2 in
  // the token chart corresponds to 0:1 in the syntax chart).  this code fails
  // to adjust vertices in packed items, leading to inconsistent derivations.
  // with common DELPH-IN grammars, we hardly expect packing in lexical parsing
  // anyway, so in principle we may even save time here (fewer subsumptions);
  // with the ERG at least, the problem only became visible due to a bug in
  // multi-word lexical entries.  i only wonder about one thing: as we leave
  // lexical filtering, will someone attempt to pack those edges, i.e. before
  // they start deriving larger phrases?                        (24-jan-09; oe)
  //
  int opt_packing_backup = get_opt_int("opt_packing");
  if (chart_mapping)
    set_opt("opt_packing", 0);

  while (! _agenda.empty()) {
    _agenda.front()->execute(*this);
    _agenda.pop();
  }

  // exhaustively apply lexical and inflection rules
  if (lex_exhaustive) {
    parse_loop(FSAS, errors, 0);
  }
  
  set_opt("opt_packing", opt_packing_backup);
  
  // Lexical chart mapping (a.k.a. lexical filtering):
  if (chart_mapping) {
    if (LOG_ENABLED(logChartMapping, NOTICE) || chart_mapping_loglevel & 1)
      fprintf(stderr, "[cm] lexical filtering starts\n");
    // map to tChart:
    tChart chart;
    tItemPrinter ip(cerr, false, true);
    tChartUtil::map_chart(*Chart, chart);
    if (LOG_ENABLED(logChartMapping, INFO) || chart_mapping_loglevel & 8) {
      fprintf(stderr, "[cm] initial lexical chart:\n");
      chart.print(cerr, &ip, passive_unblocked_non_input);
    } // if
    // apply chart mapping rules:
    const std::list<class tChartMappingRule*> &rules = Grammar->lexflt_rules();
    tChartMappingEngine mapper(rules, "lexical filtering");
    mapper.process(chart);
    // map back:
    _maxpos = tChartUtil::map_chart(chart, *Chart);
    // chart and chart vertices memory is released by ~tChart 
    // chart items should be handled by tItem::default_owner()
    if (LOG_ENABLED(logChartMapping, INFO) || chart_mapping_loglevel & 8) {
      fprintf(stderr, "[cm] final lexical chart:\n");
      chart.print(cerr, &ip, passive_unblocked_non_input);
    } // if
    if (LOG_ENABLED(logChartMapping, NOTICE) || chart_mapping_loglevel & 1)
      fprintf(stderr, "[cm] lexical filtering ends\n");
  }
}


void
lex_parser::lexical_processing(inp_list &inp_tokens
                               , bool chart_mapping, bool lex_exhaustive
                               , fs_alloc_state &FSAS, list<tError> &errors) {

  lexical_parsing(inp_tokens, chart_mapping, lex_exhaustive, FSAS, errors);

  // dependency filtering is done on the chart itself
  dependency_filter(cheap_settings->lookup("chart-dependencies"),
                    (cheap_settings->lookup(
                            "unidirectional-chart-dependencies") != 0),
                    lex_exhaustive);

  //tTclChartPrinter chp("/tmp/lex-chart", 0);
  //Chart->print(&chp);

  // If -default-les or -predict-les is used, lexical entries for unknown
  // input items are only added where there are gaps in the chart.
  // If -default-les=all is used, generics have already been added for those
  // input items which are compatible with the generic (viz. where the input
  // feature structures unifies into a predefined path in the generic lexical
  // entry), regardless of whether there is a gap in the chart or not. This
  // latter approach may be slower, so there can be a tradeoff between maximal
  // robustness and fast parsing.
  
  // Gap computation.
  list< tInputItem * > unexpanded;
  item_predicate valid = (lex_exhaustive ? lex_complete : non_input);
  if (! Chart->connected(valid)) {
    unexpanded = find_unexpanded(Chart, valid) ;
  }

  int opt_predict_les;
      get_opt("opt_predict_les", opt_predict_les);
  
  default_les_strategy opt_default_les;
  get_opt("opt_default_les", opt_default_les);
  
  // Add generic or predicted lexical types for unexpanded input items.
  // Generic lexical entries will only be added here if
  // `-default-les=traditional' is set (not if -default-les=all is set).
      // Lexical type predictor will only be used when there is
      // unexpanded inps and the `-default-les' is not used.
  if ((opt_default_les == DEFAULT_LES_POSMAP_LEXGAPS || opt_predict_les)
      && !unexpanded.empty()) {
    if (opt_default_les == DEFAULT_LES_POSMAP_LEXGAPS) { 
      add_generics(unexpanded);
    } else if (opt_predict_les) {
        add_predicts(unexpanded, inp_tokens, opt_predict_les);
      }
      unexpanded.clear();
      
    // This may in principle lead to new lexical parsing, but only if generics
    // may be multi word entries.
      while (! _agenda.empty()) {
        _agenda.front()->execute(*this);
        _agenda.pop();
      }
      
      if (lex_exhaustive) {
        parse_loop(FSAS, errors, 0);
      }
      if (! Chart->connected(*valid)) {
        unexpanded = find_unexpanded(Chart, *valid);
      }
    }

  // throw an error if there are unexpanded input items
  if (! unexpanded.empty()) {
    string missing = "";
    for (inp_iterator inp = unexpanded.begin(); inp != unexpanded.end(); ++inp)
    {
      missing += "\n\t\"" + (*inp)->orth() + "\"";
      if(opt_default_les) 
        // it's useful to know the pos tags of failed entries
        {
          postags tags = (*inp)->get_in_postags();
          missing += " ["+ tags.getPrintString() +"]";
        }
    }
    throw tError("no lexicon entries for:" + missing) ;
  }
  

  //
  // finally, we need to create new tasks for the second parsing phase.  in
  // case there was a full `lexical' parsing phase already, make sure to not
  // tasks that would lead to duplicate, i.e. do not use lexical rules for the
  // creation of new tasks.  otherwise, use all rules.
  //
  if (lex_exhaustive) {
    Grammar->activate_syn_rules();
  } else {
    Grammar->activate_all_rules();
  }

  chart_iter ci(Chart);
  while (ci.valid()) {
    tItem *item = ci.current();
    if (item->passive() && !item->blocked() && (item->trait() != INPUT_TRAIT)) {
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
  //
  // from here on, lexical as well as non-lexical rules should be active.  in
  // principle at least, we have bought into the LKB point of view that allows
  // `lexical' rules to apply to phrases too.
  //
  Grammar->activate_all_rules();
}


/**
 * Helper function for printing debug logs for lex_and_inp_task::execute().
 */
std::string
debug_lex_and_inp_task_execute(tLexItem *l_item, tInputItem *i_item) {
  ostringstream cdeb;
  cdeb << "Combining lexical item "
       << l_item->printname()
       << "(id:" << l_item->id()
       << ", form:" << l_item->get_yield()
       << ") with input item "
       << i_item->printname()
       << "(id:" << i_item->id()
       << ", form:" << i_item->form()
       << ") failed in creating a valid fs.";
  return cdeb.str();
}

void lex_and_inp_task::execute(class lex_parser &parser) {

  // the lex item already has its infl position filled (as long as the
  // MW extension infl AND keypos is not done)
  // check if the current orthography matches the one of the input item
  // and that the same combination was not executed before
  if (_lex_item->compatible(_inp_item)) {
    //
    // in case we have found all arguments for this lexical entry, including
    // MWEs, unify all input feature structures into the `tokens' path.  it is
    // necessary to do this now, before we create a new (passive) lexical item,
    // because lexical instantiation can fail at this point, specifically for
    // generic lexical items that carry constraints that are incompatible with
    // the token information.
    //
    if (_lex_item->near_passive() 
        && tChartUtil::lexicon_tokens_path()
        && tChartUtil::lexicon_last_token_path()) {
      
      fs item_fs = _lex_item->get_fs(true);
      //
      // build a list of the right arity, containing the token FSs
      //
      fs list_fs(BI_LIST);
      fs nil_fs(BI_NIL);
      list_int *lpath = 0;
      list<tItem *> daughters(_lex_item->daughters());
      if(_lex_item->left_extending())
        daughters.push_front(_inp_item);
      else
        daughters.push_back(_inp_item);
      for(item_citer daughter = daughters.begin();
          daughter != daughters.end();
          ++daughter) {
        fs token_fs = dynamic_cast<tInputItem *>(*daughter)->get_fs();
        fs cons_fs(BI_CONS);
        cons_fs = unify(cons_fs, cons_fs.get_attr_value(BIA_FIRST), token_fs);
        list_fs = unify(list_fs, list_fs.get_path_value(lpath), cons_fs);
        lpath = append(lpath, BIA_REST);
      } // for
      list_fs = unify(list_fs, list_fs.get_path_value(lpath), nil_fs);
      free_list(lpath);
      //
      // unify token FSs into final item fs:
      //
      if (item_fs.valid()) {
        fs anchor_fs = item_fs.get_path_value(tChartUtil::lexicon_tokens_path());
        if (!anchor_fs.valid()) {
          LOG(logLexproc, ERROR, debug_lex_and_inp_task_execute(_lex_item, _inp_item) +
              " Reason: lexicon_tokens_path cannot be resolved in fs.");
          return;
        }
        item_fs = unify(item_fs, anchor_fs, list_fs);
      }
      if (item_fs.valid()) {
        fs anchor_fs = item_fs.get_path_value(tChartUtil::lexicon_last_token_path());
        if (!anchor_fs.valid()) {
          LOG(logLexproc, ERROR, debug_lex_and_inp_task_execute(_lex_item, _inp_item) +
              " Reason: lexicon_last_token_path cannot be resolved in fs.");
          return;
        }
        fs token_fs = dynamic_cast<tInputItem *>(daughters.back())->get_fs();
        item_fs = unify(item_fs, anchor_fs, token_fs);
      } // if
      //
      // only create (and add()) a new lexical item when unification succeeded
      // 
      if(item_fs.valid()) {
        add(parser, new tLexItem(_lex_item, _inp_item, item_fs));
      } // if
    } // if
    else {
      // add the lex item to the chart and create appropriate tasks
      add(parser, new tLexItem(_lex_item, _inp_item));
    } // else
  }

} // lex_and_inp_task::execute()
