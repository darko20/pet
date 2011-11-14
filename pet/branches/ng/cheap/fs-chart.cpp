/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 2007 Peter Adolphs
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

#include "fs-chart.h"

#include "chart.h"
#include "cheap.h"
#include "configs.h"
#include "errors.h"
#include "item.h"
#include "item-printer.h"
#include "logging.h"
#include "settings.h"

#include <list>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;
using namespace HASH_SPACE;
using boost::lexical_cast;


// =====================================================
// class tChart
// =====================================================

// helper function:
/**
 * Add all \a items that are not included in \a skip to \a result.
 */
inline static void
filter_items(const item_list &items,
    bool skip_blocked,
    bool skip_pending_inflrs,
    item_list &skip,
    item_list &result)
{
  item_list::const_iterator it;
  for (it = items.begin(); it != items.end(); ++it) {
    if (!(skip_blocked && (*it)->blocked())
        && !(skip_pending_inflrs && !(*it)->inflrs_complete_p())
        && (find(skip.begin(), skip.end(), *it) == skip.end())) {
      result.push_back(*it);
    }
  }
}

void
tChart::clear()
{
  for (std::vector<tChartVertex*>::iterator it = _vertices.begin();
       it != _vertices.end();
       ++it)
    delete *it;
  _vertices.clear();
  _items.clear(); // items are usually released by the tItem::default_owner
}

std::list<tChartVertex*>
tChart::start_vertices(bool connected) const
{
  std::list<tChartVertex*> vertices;
  for (std::vector<tChartVertex*>::const_iterator it = _vertices.begin();
       it != _vertices.end();
       ++it)
  {
    if ((*it)->ending_items().empty() &&
        (!connected || !(*it)->starting_items().empty()))
    {
      vertices.push_back(*it);
    }
  }
  return vertices;
}

std::list<tChartVertex*>
tChart::end_vertices(bool connected) const
{
  std::list<tChartVertex*> vertices;
  for (std::vector<tChartVertex*>::const_iterator it = _vertices.begin();
       it != _vertices.end();
       ++it)
  {
    if ((*it)->starting_items().empty() &&
        (!connected || !(*it)->ending_items().empty()))
    {
      vertices.push_back(*it);
    }
  }
  return vertices;
}

tItem*
tChart::add_item(tItem *item, const tChartVertex *cprec,
                 const tChartVertex *csucc)
{
  // const_cast is OK here since tChart is the only one to tamper with
  // chart vertices
  tChartVertex *prec = const_cast<tChartVertex *>(cprec);
  tChartVertex *succ = const_cast<tChartVertex *>(csucc);
  assert((item != NULL) && (prec != NULL) && (succ != NULL));
  // does not seem to hold. Why?
  //assert((item->prec_vertex() == NULL) && (item->succ_vertex() == NULL));
  assert((prec->_chart == this) && (succ->_chart == this));

  _items.push_back(item);
  item->set_prec_vertex(prec);
  item->set_succ_vertex(succ);
  prec->starting_items().push_back(item);
  succ->ending_items().push_back(item);

  item->notify_chart_changed(this);

  return item;
}

/* currently unused
void
tChart::remove_item(tItem *item)
{
  assert(item != NULL);
  assert((item->prec_vertex() != NULL) && (item->succ_vertex() != NULL));
  assert(item->_chart == this);

  list<tItem *>::iterator start =
    find(item->prec_vertex()->starting_items().begin(),
         item->prec_vertex()->starting_items().end(),
         item);
  list<tItem *>::iterator end =
    find(item->succ_vertex()->ending_items().begin(),
         item->succ_vertex()->ending_items().end(),
         item);

  assert(start != item->prec_vertex()->starting_items().end() &&
         end != item->succ_vertex()->ending_items().end());

  item->prec_vertex()->starting_items().erase(start);
  item->succ_vertex()->ending_items().erase(end);
  item->set_prec_vertex(NULL);
  item->set_succ_vertex(NULL);
  _items.remove(item);

  item->notify_chart_changed(0);

  // items are usually released by the tItem::default_owner
}


void
tChart::remove_items(item_list items)
{
  for (item_iter it = items.begin(); it != items.end(); ++it) {
    remove_item(*it);
  }
}
*/

item_list
tChart::items(bool skip_blocked, bool skip_pending_inflrs, item_list skip)
{
  item_list result;
  filter_items(_items, skip_blocked, skip_pending_inflrs, skip, result);
  return result;
}

item_list
tChart::items(const tChartVertex *prec, const tChartVertex *succ,
              bool skip_blocked, bool skip_pending_inflrs, item_list skip) {
  item_list result;
  if (prec) {
    item_list candidates;
    // TODO: why not filtering while looping?
    filter_items(prec->starting_items(), skip_blocked,
                 skip_pending_inflrs, skip, candidates);
    for (item_iter it = candidates.begin(); it != candidates.end(); ++it)
      if (!succ || ((*it)->succ_vertex() == succ))
        result.push_back(*it);
  } else if (succ) {
    item_list candidates;
    filter_items(succ->ending_items(), skip_blocked,
                 skip_pending_inflrs, skip, candidates);
    for (item_iter it = candidates.begin(); it != candidates.end(); ++it)
      if (!prec || ((*it)->prec_vertex() == prec))
        result.push_back(*it);
  } else {
    filter_items(_items, skip_blocked, skip_pending_inflrs, skip, result);
  }
  return result;
}


// recursive helper
void
tChart::succeeding_items_rec(const tChartVertex *v, int min, int max,
                             bool skip_blocked, bool skip_pending_inflrs,
                             item_list skip,
                             int dist, item_list &result,
                             std::list<const tChartVertex*> &vertices) {
  vertices.push_back(v);
  if (dist > max)
    return;
  item_list items = v->starting_items();
  // add filtered succeeding items to the result list:
  if ((min <= dist) && (dist <= max))
    filter_items(items, skip_blocked, skip_pending_inflrs, skip, result);
  // schedule processing of all vertices not processed before:
  for (item_iter iit = items.begin(); iit != items.end(); ++iit) {
    tChartVertex *next = (*iit)->succ_vertex();
    if (find(vertices.begin(), vertices.end(), next) == vertices.end()) {
      succeeding_items_rec(next, min, max, skip_blocked, skip_pending_inflrs,
                           skip, dist+1, result, vertices);
    }
  }
}

item_list
tChart::succeeding_items(const tChartVertex *v, int min, int max,
                         bool skip_blocked, bool skip_pending_inflrs,
                         item_list skip) {
  item_list result;
  std::list<const tChartVertex*> vertices; // processed vertices
  succeeding_items_rec(v, min, max, skip_blocked, skip_pending_inflrs, skip, 0,
                       result, vertices);
  return result;
}

void
tChart::preceding_items_rec(const tChartVertex *v, int min, int max,
                            bool skip_blocked, bool skip_pending_inflrs,
                            item_list skip,
                            int dist, item_list &result,
                            std::list<const tChartVertex*> &vertices) {
  vertices.push_back(v);
  if (dist > max)
    return;
  item_list items = v->ending_items();
  // add filtered preceding items to the result list:
  if ((min <= dist) && (dist <= max))
    filter_items(items, skip_blocked, skip_pending_inflrs, skip, result);
  // schedule processing of all vertices not processed before:
  for (item_iter iit = items.begin(); iit != items.end(); ++iit) {
    tChartVertex *next = (*iit)->prec_vertex();
    if (find(vertices.begin(), vertices.end(), next) == vertices.end()) {
      preceding_items_rec(next, min, max, skip_blocked, skip_pending_inflrs,
                          skip, dist+1, result, vertices);
    }
  }
}

item_list
tChart::preceding_items(const tChartVertex *v, int min, int max,
                        bool skip_blocked, bool skip_pending_inflrs,
                        item_list skip) {
  item_list result;
  std::list<const tChartVertex*> vertices; // processed vertices
  preceding_items_rec(v, min, max, skip_blocked, skip_pending_inflrs, skip, 0,
      result, vertices);
  return result;
}

bool
tChart::connected() {
  if(_vertices.empty()) return true;
  int nr_start = 0;
  int nr_end = 0;
  bool has_unblocked_items = false; //no unblocked items -> logically empty
  for (std::vector<tChartVertex*>::const_iterator it = _vertices.begin();
       it != _vertices.end();
       ++it)
  {
    tChartVertex* vertex = *it;
    // check items preceding vertex: stop at first valid predecessor
    bool has_active_prec_items = false;
    const item_list& initems = vertex->ending_items();
    for (item_list::const_iterator item_it = initems.begin();
         item_it != initems.end() && ! has_active_prec_items;
         ++item_it) {
      has_active_prec_items = !(*item_it)->blocked();
    }
    // check items succeeding vertex:
    bool has_active_succ_items = false;
    const item_list &outitems = vertex->starting_items();
    for (item_list::const_iterator item_it = outitems.begin();
         item_it != outitems.end() && ! has_active_succ_items;
         ++item_it) {
      has_active_succ_items = !(*item_it)->blocked();
    }
    // check whether this is a start or an end vertex:
    if (!has_active_prec_items && has_active_succ_items) {
      ++nr_start;
    }
    if (has_active_prec_items && !has_active_succ_items) {
      ++nr_end;
    }
    has_unblocked_items = has_unblocked_items 
      || has_active_prec_items || has_active_succ_items; 
  }
  return (has_unblocked_items == false) || ((nr_start == 1) && (nr_end == 1));
}

void
tChart::print(std::ostream &out, tAbstractItemPrinter *printer,
    item_predicate toprint)
{
  tItemPrinter default_printer(out, LOG_ENABLED(logChart, INFO),
      LOG_ENABLED(logChart, DEBUG));
  if (printer == NULL) {
    printer = &default_printer;
  }
  item_list all_items = items();
  for (item_iter it = all_items.begin(); it != all_items.end(); ++it) {
    tItem *item = *it;
    if (toprint(*it)) {
      printer->print(item); out << endl;
    }
  }
}


/* helper function for tChartUtil::map_chart() */
inline static tChartVertex*
retrieve_chart_vertex(tChart &chart,
    hash_map<int,tChartVertex*> &vertex_map,
    int position)
{
  tChartVertex *v = vertex_map[position];
  if (v == 0) {
    v = chart.add_vertex(tChartVertex::create());
    vertex_map[position] = v;
  }
  return v;
}



// =====================================================
// class tChartUtil
// =====================================================

// helper function:
static bool
init_lpath(list_int*& lpath, string name, string &errors)
{
  free_list(lpath);
  const char *val = cheap_settings->value(name.c_str());
  lpath = path_to_lpath(val);
  if (val && (lpath == 0)) {
    errors.append("Invalid path in setting `" + name + "'.\n");
  }
  // TODO also check whether the type at the end of lpath has the required type
  return val && lpath;
}

// define static members:
list_int* tChartUtil::_token_form_path = 0;
list_int* tChartUtil::_token_id_path = 0;
list_int* tChartUtil::_token_from_path = 0;
list_int* tChartUtil::_token_to_path = 0;
list_int* tChartUtil::_token_stem_path = 0;
list_int* tChartUtil::_token_entry_path = 0;
list_int* tChartUtil::_token_rules_path = 0;
list_int* tChartUtil::_token_postags_path = 0;
list_int* tChartUtil::_token_posprobs_path = 0;
list_int* tChartUtil::_lexicon_tokens_path = 0;
list_int* tChartUtil::_lexicon_last_token_path = 0;
list_int* tChartUtil::_context_path = 0;
list_int* tChartUtil::_input_path = 0;
list_int* tChartUtil::_output_path = 0;
list_int* tChartUtil::_poscons_path = 0;

void
tChartUtil::initialize()
{
  string e; // errors
  string w; // warnings
  string m; // missing features for interpreting input fs

  if (!init_lpath(_token_form_path, "token-form-path", e))
    e += "Setting `token-form-path' required for interpreting input fs.\n";
  if (!init_lpath(_token_id_path, "token-id-path", e))
    m += "item ids, ";
  if (!init_lpath(_token_from_path, "token-from-path", e))
    m += "surface start positions, ";
  if (!init_lpath(_token_to_path, "token-to-path", e))
    m += "surface end positions, ";
  if (!init_lpath(_token_stem_path, "token-stem-path", e))
    ; //m += "stems, ";
  if (!init_lpath(_token_entry_path, "token-entry-path", e))
    ; //m += "lexical entries, ";
  if (!init_lpath(_token_rules_path, "token-rules-path", e))
    ; //m += "orthographemic rules, ";
  if (   !init_lpath(_token_postags_path, "token-postags-path", e)
      || !init_lpath(_token_posprobs_path, "token-posprobs-path", e))
    m += "parts of speech, ";
  if (!init_lpath(_lexicon_tokens_path, "lexicon-tokens-path", e))
    w += " [ no input fs in lexical items ] ";
  if (!init_lpath(_lexicon_last_token_path, "lexicon-last-token-path", e))
    w += " [ no input fs in lexical items ] ";
  if (!init_lpath(_context_path, "chart-mapping-context-path", e))
    e += "Setting `chart-mapping-context-path' required for chart mapping.\n";
  if (!init_lpath(_input_path, "chart-mapping-input-path", e))
    e += "Setting `chart-mapping-input-path' required for chart mapping.\n";
  if (!init_lpath(_output_path, "chart-mapping-output-path", e))
    e += "Setting `chart-mapping-output-path' required for chart mapping.\n";
  if (!init_lpath(_poscons_path, "chart-mapping-position-path", e))
    e += "Setting `chart-mapping-position-path' required for chart mapping.\n";

  if (!m.empty()) {
    m.erase(m.end()-2, m.end()); // erase last ", " from list of missing tags
    w += " [ no input fs mapping for " + m + " ] ";
  }
  if (!w.empty())
    LOG(logAppl, WARN, w);
  if (!e.empty())
    throw tError(e);
}

void
tChartUtil::check_validity()
{
  string e; // errors
  if (_lexicon_tokens_path && _lexicon_last_token_path) {
    fs f1(_lexicon_tokens_path, BI_LIST);
    fs f2(_lexicon_last_token_path, BI_TOP);
    if (!f1.valid())
      e += "Failed to get a value for the specified `lexicon-tokens-path' setting.";
    if (!f2.valid())
      e += "Failed to get a value for the specified `lexicon-last-token-path' setting.";
  }
  if (!e.empty())
    throw tError(e);
}

tInputItem*
tChartUtil::create_input_item(const fs &token_fs)
{
  if (!token_fs.valid())
    throw tError("Invalid token feature structure.");

  string id; // the external id (a concatenation of the ids from input_fs)
  string form;
  string stem;
  int cfrom = -1;
  int cto = -1;
  int vfrom = -1;
  int vto = -1;
  int token_class = WORD_TOKEN_CLASS; // default; maybe reset in the following
  list<type_t> infls; // inflectional rules to be applied if STEM_TOKEN_CLASS
  postags tagsnprobs; // part-of-speech tags and probabilities

  // extract information from input_fs:
  type_t form_type = token_fs.get_path_value(_token_form_path).type();
  if (form_type != BI_STRING) {
    form = get_printname(form_type);
  }
  if (_token_id_path) {
    list<fs> ids_list = token_fs.get_path_value(_token_id_path).get_list();
    for (list<fs>::iterator it = ids_list.begin();
         it != ids_list.end();
         ++it)
      id += (std::string)(id.empty() ? "" : ",") + (*it).printname();
  }
  if (_token_from_path) {
    try {
      cfrom = boost::lexical_cast<int>(
          token_fs.get_path_value(_token_from_path).printname());
    } catch(boost::bad_lexical_cast &) {
      // should only happen if the value is not set. ignore!
    }
  }
  if (_token_to_path) {
    try {
      cto = boost::lexical_cast<int>(
          token_fs.get_path_value(_token_to_path).printname());
    } catch(boost::bad_lexical_cast &) {
      // should only happen if the value is not set. ignore!
    }
  }
  if (_token_stem_path) {
    type_t t = token_fs.get_path_value(_token_stem_path).type();
    if (t != BI_STRING) { // if the stem is set to a string literal
      stem = get_printname(t);
      if (token_class != WORD_TOKEN_CLASS)
        throw tError("Tried to use both STEM and ENTRY for the same token.");
      token_class = STEM_TOKEN_CLASS;
    }
  }
  if (_token_entry_path) {
    // TODO deprecated! this only exists because of PIC
    type_t t = token_fs.get_path_value(_token_entry_path).type();
    if (t != BI_STRING) { // if the entry is set to a string literal
      string entry = get_printname(t);
      if (token_class != WORD_TOKEN_CLASS)
        throw tError("tried to use both STEM and ENTRY for the same token.");
      token_class = lookup_type(entry);
      if (token_class == T_BOTTOM)
        throw tError((std::string) "Unknown lexical entry '" + entry + "'");
    }
  }
  if (_token_rules_path) {
    fs rules = token_fs.get_path_value(_token_rules_path);
    std::list<fs> rules_list = rules.get_list();
    for (std::list<fs>::iterator rules_it = rules_list.begin();
        rules_it != rules_list.end(); ++rules_it) {
      infls.push_back((*rules_it).type());
    }
  }
  if (_token_postags_path && _token_posprobs_path) {
    list<fs> tags = token_fs.get_path_value(_token_postags_path).get_list();
    list<fs> probs = token_fs.get_path_value(_token_posprobs_path).get_list();
    if (tags.size() != probs.size())
      throw tError("Part-of-speech tag and probability lists are not aligned.");
    list<fs>::iterator ti, pi;
    for (ti = tags.begin(), pi = probs.begin();
         (ti != tags.end()) && (pi != probs.end());
         ++ti, ++pi)
    {
      if (is_string_instance(ti->type()) && is_string_instance(pi->type())) {
        string tag = (*ti).printname();
        try {
          double prob = lexical_cast<double>((*pi).printname());
          tagsnprobs.add(tag, prob);
        } catch (boost::bad_lexical_cast) {
          cerr << "Warning: Provided part-of-speech probability is not "
               << "a double (`" << (*ti).printname() << "'). Ignoring." << endl;
        }
      } else {
        cerr << "Warning: Provided part-of-speech tags and/or probabilities "
                "are no string literals. Ignoring." << endl;
      }
    }
  }

  if (!infls.empty() && (token_class == WORD_TOKEN_CLASS))
      throw tError("Encountered token with RULES but no STEM or ENTRY.");
  tInputItem *item = new tInputItem(id, vfrom, vto, cfrom, cto, form, stem,
      tPaths(), token_class, infls, tagsnprobs, modlist(), token_fs);

  return item;
}

fs
tChartUtil::create_input_fs(tInputItem* item)
{
  fs input_fs(_token_form_path, retrieve_string_instance(item->orth()));
  if (!input_fs.valid())
    throw tError("failed to create input fs");
  if (_token_id_path) {
    //
    // our input items /can/ carry a so-called 'external id', i.e. one supplied
    // as a property of the initial parser input (in YY format, say).  at the
    // same time, all (input) items receive a PET-internal unique identifier;
    // when using an internal tokenizer (say native REPP), these internal ids
    // are all we have.  hence, in mapping an input item to a token FS, give
    // precende to external ids, where available, but otherwise use the item
    // id proper.
    //
    string id = item->external_id();
    if(id.empty()) {
      ostringstream buffer;
      buffer << item->id();
      id = buffer.str();
    } // if
    fs id_f = retrieve_string_instance(id);
    fs ids_cons = fs(BI_CONS);
    ids_cons = unify(ids_cons, ids_cons.get_attr_value(BIA_FIRST), id_f);
    fs rest_f = ids_cons.get_attr_value(BIA_REST);

    // build diff-list and set coreference between LAST and REST:
    fs ids_f = fs(BI_DIFF_LIST);
    ids_f = unify(ids_f, ids_f.get_attr_value(BIA_LIST), ids_cons);
    dag_set_attr_value(ids_f.dag(), BIA_LAST, rest_f.dag());

    input_fs = unify(input_fs, input_fs.get_path_value(_token_id_path), ids_f);
  }
  if (_token_from_path) {
    fs f = retrieve_string_instance(item->startposition());
    input_fs = unify(input_fs, input_fs.get_path_value(_token_from_path), f);
  }
  if (_token_to_path) {
    fs f = retrieve_string_instance(item->endposition());
    input_fs = unify(input_fs, input_fs.get_path_value(_token_to_path), f);
  }
  if (_token_stem_path) {
    string stem = item->stem();
    if (!stem.empty()) {
      fs f = retrieve_string_instance(item->stem());
      input_fs = unify(input_fs, input_fs.get_path_value(_token_stem_path),f);
    }
  }
  if (_token_entry_path) {
    type_t t = item->tclass();
    if ((t != WORD_TOKEN_CLASS) && (t != STEM_TOKEN_CLASS)) {
      fs f = retrieve_string_instance(get_typename(t));
      input_fs = unify(input_fs,input_fs.get_path_value(_token_entry_path),f);
    }
  }
  if (_token_rules_path) {
    list_int *infl_l = item->inflrs();
    fs rules(BI_LIST);
    list_int *path = 0; // path to last element of the rules list
    while (infl_l) {
      fs rule(infl_l->val);
      fs cons(BI_CONS);
      cons = unify(cons, cons.get_attr_value(BIA_FIRST), rule);
      rules = unify(rules, rules.get_path_value(path), cons);
      path = append(path, BIA_REST);
      infl_l = infl_l->next;
    }
    fs nil(BI_NIL);
    rules = unify(rules, rules.get_path_value(path), nil);
    input_fs = unify(input_fs,input_fs.get_path_value(_token_rules_path),rules);
  }
  if (_token_postags_path && _token_posprobs_path) {
    // TODO I need fs::create_list(std::list<fs>)
    fs tags_f(BI_LIST);
    fs probs_f(BI_LIST);
    std::vector<std::string> tags;
    std::vector<double> probs;
    item->get_in_postags().tagsnprobs(tags, probs);
    std::vector<std::string>::iterator tags_it;
    std::vector<double>::iterator probs_it;
    list_int *path = 0;
    for (tags_it = tags.begin(), probs_it = probs.begin();
         tags_it != tags.end();
         ++tags_it, ++probs_it)
    {
      fs tag_f = fs(retrieve_string_instance(*tags_it));
      fs prob_f = fs(retrieve_string_instance(lexical_cast<string>(*probs_it)));
      fs tag_cons(BI_CONS);
      fs prob_cons(BI_CONS);
      tag_cons = unify(tag_cons, tag_cons.get_attr_value(BIA_FIRST), tag_f);
      prob_cons = unify(prob_cons, prob_cons.get_attr_value(BIA_FIRST), prob_f);
      tags_f = unify(tags_f, tags_f.get_path_value(path), tag_cons);
      probs_f = unify(probs_f, probs_f.get_path_value(path), prob_cons);
      path = append(path, BIA_REST);
    }
    fs nil(BI_NIL);
    tags_f = unify(tags_f, tags_f.get_path_value(path), nil);
    probs_f = unify(probs_f, probs_f.get_path_value(path), nil);
    free_list(path);
    fs f = fs(_token_postags_path, BI_LIST);
    input_fs = unify(input_fs, input_fs, f);
    f = fs(_token_posprobs_path, BI_LIST);
    input_fs = unify(input_fs, input_fs, f);
    input_fs = unify(input_fs, input_fs.get_path_value(_token_postags_path),
        tags_f);
    input_fs = unify(input_fs, input_fs.get_path_value(_token_posprobs_path),
        probs_f);
  }

  if (!input_fs.valid()) {
    throw tError("Construction of input feature structure for token "
        "\"" + item->orth() + "\" (external id \"" + item->external_id() +
        "\") failed.");
  }

  return input_fs;
}

/**
 * Establishes a topological order, mapping chart vertices to int values
 * indicating the precedence of a vertex (vertices with higher values come
 * before vertices with lower values).
 * \pre exactly one start and end node & no loops in the chart
 */
static void
topological_order(const tChartVertex *vertex, int &max_value,
    map<const tChartVertex*, int, greater<const void*> > &order,
    list<const tChartVertex*> &ordered)
{
  list<tItem*> items = vertex->starting_items();
  for (list<tItem*>::iterator it = items.begin(); it != items.end(); ++it) {
    if(!passive_unblocked(*it)) continue; 
    const tChartVertex *succ = (*it)->get_succ_vertex();
    if (order.find(succ) == order.end()) // if not visited
      topological_order(succ, max_value, order, ordered);
  }
  ++max_value;
  order[vertex] = max_value;
  ordered.push_front(vertex);
}

int
tChartUtil::assign_int_nodes(tChart &chart, item_list &processed)
{
  processed.clear();
  if(chart.vertices().empty()) return 0;
  list<tChartVertex*> vertices = chart.start_vertices();
  if (vertices.size() > 1) // TODO how do we deal with several start vertices?
    LOG(logChart, WARN,
        "Several start vertices present. Only using the first start vertex.");
  int max_value = -1; // will be set accordingly by topological_order()
  map<const tChartVertex*, int, greater<const void*> > order;
  list<const tChartVertex*> ordered;
  topological_order(vertices.front(), max_value, order, ordered);
  list<const tChartVertex*>::const_iterator vit;
  for (vit = ordered.begin(); vit != ordered.end(); ++vit) {
    const tChartVertex *prec_vertex = *vit;
    const list<tItem*> items = prec_vertex->starting_items();
    for (list<tItem*>::const_iterator it = items.begin(); it != items.end(); ++it) {
      tItem *item = *it;
      if(!passive_unblocked(*it)) continue; 
      item->set_start(max_value - order[prec_vertex]);
      item->set_end(max_value - order[item->get_succ_vertex()]);
      processed.push_back(item);
    }
  }
  return max_value;
}

void
tChartUtil::map_chart(inp_list &input_items, tChart &chart)
{
  chart.clear();
  hash_map<int, tChartVertex*> vertex_map; // maps char positions to vertices

  // convert each input item to a token feature structure:
  for (list<tInputItem*>::iterator item_it = input_items.begin();
       item_it != input_items.end();
       ++item_it)
  {
    // determine start and end positions (int vertices):
    int vfrom = (*item_it)->start();         // start vertex as an int
    int vto   = (*item_it)->end();           // end vertex as an int
    int cfrom = (*item_it)->startposition(); // external character position
    int cto   = (*item_it)->endposition();   // external character position

    // fix vfrom and vto in case that the position_mapper has not performed its
    // job yet:
    if (vfrom == -1 || vto == -1) {
      // TODO check: is this ok in all situations?
      vfrom = cfrom;
      vto = cto;
    }

    // get a chart vertex for the from and to positions:
    tChartVertex *from_v = retrieve_chart_vertex(chart, vertex_map, vfrom);
    tChartVertex *to_v   = retrieve_chart_vertex(chart, vertex_map, vto);

    // finally insert a new chart item into the chart
    chart.add_item(*item_it, from_v, to_v);
  }
}

int
tChartUtil::map_chart(tChart &chart, inp_list &input_items)
{
  item_list items;
  int nr_processed = tChartUtil::assign_int_nodes(chart, items);
  input_items.clear();
  for (item_iter it=items.begin(); it!=items.end(); ++it) {
    tInputItem *inp_item = dynamic_cast<tInputItem*>(*it);
    assert(inp_item);
    input_items.push_back(inp_item);
  }
  return nr_processed;
}

void
tChartUtil::map_chart(chart &in, tChart &out)
{
  hash_map<int,tChartVertex*> vertex_map;
  for (chart_iter it(in); it.valid(); ++it) {
    tItem *item = it.current();
    tChartVertex* prec = retrieve_chart_vertex(out, vertex_map, item->start());
    tChartVertex* succ = retrieve_chart_vertex(out, vertex_map, item->end());
    out.add_item(item, prec, succ);
  }
}

int
tChartUtil::map_chart(tChart &in, chart &out)
{
  item_list items;
  int nr_processed = tChartUtil::assign_int_nodes(in, items);
  out.reset(nr_processed);
  for (item_iter it=items.begin(); it!=items.end(); ++it) {
    out.add(*it);
  }
  return nr_processed;
}
