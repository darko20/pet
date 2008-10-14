/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
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

#include "chart-mapping.h"

#include "builtins.h"
#include "cheap.h"
#include "errors.h"
#include "item.h"
#include "item-printer.h"
#include "grammar.h"
#include "hashing.h"

#include <stdio.h>
#include <iostream>

#include <deque>
#include <map>
#include <string>

#include <boost/lexical_cast.hpp>

#ifdef HAVE_ICU
#include "unicode.h"
#endif

using namespace std;
using HASH_SPACE::hash_map;
using boost::lexical_cast;



// =====================================================
// class tChartMappingEngine
// =====================================================

class not_contained_in_list
{
private:
  item_list _items;
public:
  not_contained_in_list(item_list items) : _items(items) { }
  bool operator() (tItem*& item) {
    for (item_iter item_it = _items.begin(); item_it != _items.end(); item_it++)
      if (*item_it == item)
        return false;
    return true;
  }
};

/**
 * A signature for a chart mapping match. The first element in the quadruple
 * is the rule of the match, the second is the previous match, the third is
 * the item to be matched next, and the fourth is the argument used to match
 * the item.
 */
typedef boost::tuple<const tChartMappingRule*,
                     tChartMappingMatch*,
                     tItem*,
                     const tChartMappingRuleArg*> tChartMappingMatchSig;

struct sig_hash {
  inline size_t operator() (const tChartMappingMatchSig &s) const
  {
    return reinterpret_cast<size_t>(s.get<0>()) +
           reinterpret_cast<size_t>(s.get<1>()) +
           reinterpret_cast<size_t>(s.get<2>()) +
           reinterpret_cast<size_t>(s.get<3>());
  }
};

typedef hash_map<tChartMappingMatchSig, tChartMappingMatch*, sig_hash>
  tChartMappingMatchCache;

/**
 * Find all items from the chart that could serve as an item for the next
 * match by evaluating the positional constraints of the next rule argument
 * \a next_arg and the current match \a match.
 */
static inline item_list
get_suitable_items(tChart &chart, tChartMappingMatch *match,
    const tChartMappingRuleArg *next_arg)
{
  // matched items shall not be matched twice:
  item_list skip = match->matched_items();

  // select all items from the chart that have not been matched already:
  item_list items = chart.items(true, true, skip);

  // weed out all items that don't fulfil the positional constraints:
  typedef std::list<tChartMappingPosCons> tPosConsList;
  tPosConsList poscons = next_arg->_poscons;
  for (tPosConsList::iterator cons_it = poscons.begin();
       cons_it != poscons.end();
       cons_it++)
  {
    tItem* matched_item = match->matched_item((*cons_it).arg->name());
    if (!matched_item) {
      throw tError((string)"Positional constraint in rule " +
        match->get_rule()->printname()+" refers to an unmatched item.");
    }
    item_list restr; // restrictor set
    switch ((*cons_it).rel) {
      case tChartMappingPosCons::same_cell:
        restr = chart.same_cell_items(matched_item, true, true, skip); break;
      case tChartMappingPosCons::succeeding:
        restr = chart.succeeding_items(matched_item, true, true, skip); break;
      case tChartMappingPosCons::all_succeeding:
        restr = chart.all_succeeding_items(matched_item, true, true, skip); break;
    }
    items.remove_if(not_contained_in_list(restr));
  }

  return items;
}

static tChartMappingMatch*
get_new_completed_match(tChart &chart, tChartMappingMatch *match,
    tChartMappingMatchCache &cache)
{
  assert(!match->is_complete());

  // loop over all suitable items for the next match:
  const tChartMappingRule *rule = match->get_rule();
  const tChartMappingRuleArg *arg = match->get_next_arg();
  item_list suitable_items = get_suitable_items(chart, match, arg);
  for (item_iter it = suitable_items.begin(); it!=suitable_items.end(); it++) {
    tItem *item = *it;
    tChartMappingMatch *next_match;
    bool new_match;

    // build the next match with item and arg or retrieve it from the cache:
    tChartMappingMatchSig sig(rule, match, item, arg);
    new_match = (cache.find(sig) == cache.end());
    next_match = new_match ? (cache[sig]=match->match(item, arg)) : cache[sig];

    // logging:
    if ((opt_chart_mapping & 256) && new_match) {
      // TODO implement tItem::to_string() or tItem::printname()??
      string itemstr = "chart item " + lexical_cast<string>(item->id());
      tInputItem* inp_item = dynamic_cast<tInputItem*>(item);
      if (inp_item)
        itemstr += " (input item `" + inp_item->form() + "')";
      fprintf(stderr, "[cm] %s %s, arg %s with %s\n",
              (next_match ? "MATCHED" : "checked"), rule->printname(),
              arg->name().c_str(), itemstr.c_str());
    }

    // base case and recursion:
    if (next_match) {
      if (next_match->is_complete()) {
        if (new_match)
          return next_match;
      } else {
        tChartMappingMatch *next_completed_match =
          get_new_completed_match(chart, next_match, cache);
        if (next_completed_match)
          return next_completed_match;
      }
    }
  }

  return 0; // there were no suitable items or no further completed match
}

/**
 * Modifies the chart as specified by the rule for the completed item
 * \a match.
 * \pre match.is_complete() == \c true
 * \return \c true if the chart has been changed
 */
static inline bool
build_output(tChart &chart, tChartMappingMatch &match)
{
  assert(match.is_complete());

  bool chart_changed = false;

  tChartVertex *first_v = 0;
  tChartVertex *last_v = 0;
  tChartMappingRuleTrait trait = match.get_rule()->trait();

  // determine first and last vertex from INPUT or CONTEXT items:
  item_list inps = match.matched_input_items();
  item_list cons = match.matched_context_items();
  if (inps.size() > 0) {
    first_v = inps.front()->prec_vertex();
    last_v = inps.back()->succ_vertex();
  } else {
    assert(cons.size() > 0);
    first_v = cons.front()->prec_vertex();
    last_v = cons.back()->succ_vertex();
  }

  // add all OUTPUT items:
  std::vector<tItem*> outs; // needed for book-keeping
  std::list<fs> out_fss = match.output_fss();
  tChartVertex *prec = first_v;
  tChartVertex *succ = 0;
  bool same_cell_output = match.get_rule()->_same_cell_output; // TODO generalize!!! ugly!!!
  for (std::list<fs>::iterator it = out_fss.begin(); it!=out_fss.end(); it++) {
    fs out_fs = *it;
    tItem *out_item = 0;
    succ = (same_cell_output || (*it == out_fss.back())) ?
        last_v : chart.add_vertex(tChartVertex::create());
    // try to find an existing identical item for reuse:
    list<tItem*> items = prec->starting_items();
    list<tItem*>::iterator item_it;
    for (item_it = items.begin(); item_it != items.end(); item_it++) {
      if (((*item_it)->succ_vertex() == succ) && !(*item_it)->blocked()) {
        fs item_fs = (*item_it)->get_fs();
        bool forward = true, backward = true;
        subsumes(out_fs, item_fs, forward, backward);
        if (forward && backward) { // identical item
          out_item = *item_it;
          inps.remove(out_item); // don't remove this identical items later
        }
      }
    }
    // if output item is not reused, create a new one and add it:
    if (out_item == 0) {
      if (trait == CM_INPUT_TRAIT) {
        out_item = tChartUtil::create_input_item(out_fs);
      } else if (trait == CM_LEX_TRAIT) {
        throw tError("tChartMappingEngine: Construction of lexical items not "
            "allowed.");
      }
      chart.add_item(out_item, prec, succ);
      chart_changed = true;
    }
    // book-keeping:
    outs.push_back(out_item);
    // set the next preceding vertex:
    if (!same_cell_output)
      prec = succ;
  }

  // freeze all INPUT items in the chart:
  for (item_iter it = inps.begin(); it != inps.end(); it++)
    (*it)->freeze(false);
  chart_changed = chart_changed || !inps.empty();

  // logging:
  if (opt_chart_mapping & 1 || opt_chart_mapping & 16) {
    std::string item_ids = "";
    std::vector<tChartMappingRuleArg*> args = match.get_rule()->args();
    std::vector<tChartMappingRuleArg*>::iterator arg_it;
    for (arg_it = args.begin(); arg_it != args.end(); arg_it++) {
      string name = (*arg_it)->name();
      int id = match.matched_item(name)->id();
      item_ids += name + ":" + lexical_cast<string>(id) + " ";
    }
    int i;
    std::vector<tItem*>::iterator item_it;
    for (item_it = outs.begin(), i = 1; item_it != outs.end(); item_it++, i++) {
      item_ids += "O" + lexical_cast<string>(i) + ":"
                  + lexical_cast<string>((*item_it)->id()) + " "; // TODO make OUTPUT rule item name accessible
    }
    fprintf(stderr, "[cm] `%s' fired: %s\n",
        match.get_rule()->printname(), item_ids.c_str());

    if(opt_chart_mapping & 16) {
      tItemPrinter ip(cerr, false, true);
      for(std::vector<tItem *>::iterator item = outs.begin();
          item != outs.end();
          ++item) {
        ip.print(*item); cerr << endl;
      } // for
    } // if
  } // if

  return chart_changed;
}

void
tChartMappingEngine::apply_rules(tChart &chart)
{
  // logging:
  if (verbosity >= 4 || opt_chart_mapping & 1) {
    // get max id for items in the chart:
    // TODO for some reason the ids of the input chart might come in any order
    //      fix this and then look at the last item only
    //      not sure whether this is still the case)
    item_list items = chart.items();
    item_list::reverse_iterator it;
    int max_id = -1;
    for (it = items.rbegin(); it != items.rend(); it++)
      if ((*it)->id() > max_id)
        max_id = (*it)->id();
    fprintf(stderr, "[cm] greatest item id before chart mapping: "
        "%d\n", max_id);
  }

  // cache storing each match we've created:
  tChartMappingMatchCache cache;

  // chart-mapping loop:
  bool chart_changed;
  do {
    chart_changed = false;
    list<tChartMappingRule*>::const_iterator rule_it;
    for (rule_it = _rules.begin(); rule_it != _rules.end(); rule_it++) {
      tChartMappingRule *rule = *rule_it;
      tChartMappingMatchSig rule_sig(rule, 0, 0, 0);
      tChartMappingMatch *empty_match = (cache.find(rule_sig) == cache.end()) ?
        (cache[rule_sig] = tChartMappingMatch::create(rule)) : cache[rule_sig];
      tChartMappingMatch *completed = 0;
      do {
        completed = get_new_completed_match(chart, empty_match, cache);
        if (completed) // 0 if there is no further completed match
          chart_changed = build_output(chart, *completed) || chart_changed;
      } while (completed);
    } // for each rule
  } while (false); // quit after the first round of rule applications
  // we are still undecided whether we want to have several rounds, i.e.:
  //} while (chart_changed); // loop until fixpoint reached

  // release all created matches:
  tChartMappingMatchCache::iterator match_it;
  for (match_it = cache.begin(); match_it != cache.end(); match_it++)
    delete match_it->second; // delete 0 is safe according C++ Standard

  // logging:
  if (verbosity >= 4 || opt_chart_mapping & 1) {
    // get max id for items in the chart:
    // TODO for some reason the ids of the input chart might come in any order
    //      fix this and then look at the last item only
    item_list items = chart.items();
    item_list::reverse_iterator it;
    int max_id = -1;
    for (it = items.rbegin(); it != items.rend(); it++)
      if ((*it)->id() > max_id)
        max_id = (*it)->id();
    fprintf(stderr, "[cm] greatest item id after chart mapping: "
        "%d\n", max_id);
  }
}



// =====================================================
// class tChartMappingRule
// =====================================================

/**
 * Helper function that looks up each path in the given argument feature
 * structure that ends in a string, inspects whether this string contains a
 * regular expression, if so it stores that regex with the path in the
 * \a regexs map and replaces the regex in the feature structure with the
 * general string type to allow for unification with any string literal.
 */
static void
modify_arg_fs(fs arg_fs, tPathRegexMap &regexs)
{
  // check for each path in arg ending in a string whether it contains a
  // regex. if so, store it in a map and replace it with the general type
  // BI_STRING to allow for unification with any string literal:
  std::list<list_int*> regex_paths = arg_fs.find_paths(BI_STRING);
  std::list<list_int*>::iterator it;
  for (it = regex_paths.begin(); it != regex_paths.end(); it++)
  {
    list_int *regex_path = *it;
    string arg_val = get_typename(arg_fs.get_path_value(regex_path).type());
#ifdef HAVE_BOOST_REGEX_ICU_HPP
    UnicodeString uc_arg_val = Conv->convert(arg_val);
    static UnicodeString rex_start("\"/", -1, US_INV);
    static UnicodeString rex_end  ("/\"", -1, US_INV);
    if (uc_arg_val.startsWith(rex_start) && uc_arg_val.endsWith(rex_end)) {
      uc_arg_val.setTo(uc_arg_val, 2, uc_arg_val.length() - 4);
      regexs[regex_path] = boost::make_u32regex(uc_arg_val);
      arg_fs.get_path_value(regex_path).set_type(BI_STRING);
    } else {
      free_list(regex_path);
    }
#else
    string::iterator begin = arg_val.begin();
    string::iterator end = arg_val.end();
    if ((*(begin++) == '"') && (*(begin++) == '/') &&
        (*(--end)   == '"') && (*(--end)   == '/'))
    {
      arg_val.assign(begin, end); // use the regex string only
      regexs[regex_path] = boost::regex(arg_val);
      arg_fs.get_path_value(regex_path).set_type(BI_STRING);
    } else {
      free_list(regex_path);
    }
#endif
  }
}

tChartMappingRule::tChartMappingRule(type_t type,
    tChartMappingRuleTrait trait)
: _type(type), _trait(trait), _same_cell_output(false)
{
  // get the feature structure representation of this rule. this must
  // be a deep copy of the type's feature structure since we need to
  // modify this structure:
  _fs = copy(fs(_type));

  // get arguments (context and input items of this rule):
  list<fs> cons = _fs.get_path_value(tChartUtil::context_path()).get_list();
  list<fs> inps = _fs.get_path_value(tChartUtil::input_path()).get_list();

  // create argument representations for this rule:
  try {
    int i;
    std::list<fs>::iterator arg_it;
    for (arg_it = cons.begin(), i = 1; arg_it != cons.end(); arg_it++, i++)
    {
      tPathRegexMap regexs;
      modify_arg_fs(*arg_it, regexs);
      string name = "C" + lexical_cast<std::string>(i);
      _args.push_back(tChartMappingRuleArg::create(name, false, i, regexs));
    }
    for (arg_it = inps.begin(), i = 1; arg_it != inps.end(); arg_it++, i++)
    {
      tPathRegexMap regexs;
      modify_arg_fs(*arg_it, regexs);
      string name = "I" + lexical_cast<std::string>(i);
      _args.push_back(tChartMappingRuleArg::create(name, true, i, regexs));
    }
  } catch (boost::regex_error e) {
    throw tError("Could not compile regex for rule "+get_typename(type)+".");
  }

  // evaluate positional constraints:
  std::vector<tChartMappingRuleArg*> ro_args; // reordered arguments
  fs poscons_fs = _fs.get_path_value(tChartUtil::poscons_path());
  if (poscons_fs.type() != BI_STRING) // there are positional constraints
  {
    string poscons_s = poscons_fs.printname();
    boost::regex re;
    // strip whitespace where it is allowed to appear:
    re = "[ \t\r\n]*(([CIO][0-9]+|<|<<|=|,)*)";
    poscons_s = boost::regex_replace(poscons_s, re, "$1");
    // check format:
    re ="(([CIO][0-9]+)([<=][CIO][0-9]+)*)?"
        "(,([CIO][0-9]+)((<|<<|=)[CIO][0-9]+)*)*";
    if (!boost::regex_match(poscons_s, re)) {
      throw tError("ill-formed positional constraints for chart mapping rule "
          + get_typename(type));
    }
    // evaluate current constraint (consuming poscons_s):
    boost::smatch m;
    re = "([CIO][0-9]+)((<|<<|=)([CIO][0-9]+))?(,?)(.*)";
    while (boost::regex_match(poscons_s, m, re)) {
      string ri1_name = m[1].str();
      string ri2_name = m[4].str();
      string rel_name = m[3].str();
      // store extracted constraint:
      tChartMappingRuleArg *arg1 = arg(ri1_name);
      tChartMappingRuleArg *arg2 = arg(ri2_name);
      if (arg1 && arg2) { // arguments referred to are not required to exist
        tChartMappingPosCons cons;
        cons.arg = arg1;
        if (rel_name == "=")
          cons.rel = tChartMappingPosCons::same_cell;
        else if (rel_name == "<")
          cons.rel = tChartMappingPosCons::succeeding;
        else if (rel_name == "<<")
          cons.rel =  tChartMappingPosCons::all_succeeding;
        else // prevent compiler warning
          throw tError("Unknown positional relation.");
        arg2->_poscons.push_back(cons);
      } else if ((ri1_name[0] == 'O') && (ri2_name[0] == 'O') && (rel_name[0] == '=')) { // TODO evaluate this properly
        _same_cell_output = true;
      }
      if (arg1 && (find(ro_args.begin(), ro_args.end(), arg1) == ro_args.end()))
        ro_args.push_back(arg1);
      if (arg2 && (find(ro_args.begin(), ro_args.end(), arg2) == ro_args.end()))
        ro_args.push_back(arg2);
      // consume current poscons, reusing 3rd match if sth but a comma follows:
      poscons_s = (((m[5].length()==0) && (m[6].length()>0)) ? m[4].str() : "")
          + m[6].str();
    }
  }

  // ro_args now contains all args referred to in the positional constraints.
  // add all missing args, and set args to the reordered args:
  vector<tChartMappingRuleArg*>::iterator it;
  for (it = _args.begin(); it != _args.end(); it++)
    if (find(ro_args.begin(), ro_args.end(), *it) == ro_args.end())
      ro_args.push_back(*it);
  _args = ro_args;

  // TODO assert that INPUT items are contiguous
}

tChartMappingRuleArg*
tChartMappingRule::arg(std::string name)
{
  typedef std::vector<tChartMappingRuleArg*>::iterator tArgIter;
  for (tArgIter it = _args.begin(); it != _args.end(); it++) {
    if ((*it)->name() == name)
      return *it;
  }
  return 0;
}



// =====================================================
// class tChartMappingMatch
// =====================================================

/**
 * Tries to match the specified item \a item with the argument \a arg.
 * Returns \c 0 if such a match is not possible.
 * This function uses a regex-enabled unification. Matched (sub-)strings
 * are added to the \a captures map, using "${" + \a get_next_arg().name() +
 * ":" + the path whose value has been matched + ":" + the number of the
 * matched substring + "}" as the key. For example, if the argument's name
 * is "I2" and contains a regular expression "/(bar)rks/" in path "FORM",
 * then the matched substring "bar" is stored under key "${I2:FORM:1}".
 */
tChartMappingMatch*
tChartMappingMatch::match(tItem *item, const tChartMappingRuleArg *arg)
{
  assert(!is_complete());

  fs root_fs = get_fs();
  fs arg_fs = get_arg_fs(arg);
  fs item_fs = item->get_fs();

  // 1. try to unify argument with item (ignoring regexs for now):
  fs result_fs = unify(root_fs, arg_fs, item_fs);
  if (!result_fs.valid())
    return 0;

  // 2. check for each regex path whether it matches in item:
  const tPathRegexMap &regexs = arg->regexs();
  std::map<std::string, std::string> captures;
  for (tPathRegexMap::const_iterator it = regexs.begin();
       it != regexs.end();
       it++)
  {
    // get string:
    list_int *regex_path = it->first;
    fs value_fs = item_fs.get_path_value(regex_path);
    if (!value_fs.valid()) // the regex_path need not exist in the item fs
      return 0;
    type_t t = value_fs.type();
    string str = get_printname(t);

    // get regex and match string with regex (if any):
#ifdef HAVE_BOOST_REGEX_ICU_HPP
    boost::u32regex regex = it->second;
    boost::u16match regex_matches;
    UnicodeString ucstr = Conv->convert(str);
    if ((t == BI_STRING) || !boost::u32regex_match(ucstr, regex_matches, regex))
      return 0;
#else
    boost::regex regex = it->second;
    boost::smatch regex_matches;
    if ((t == BI_STRING) || !boost::regex_match(str, regex_matches, regex))
      return 0;
#endif

    // get string representation of regex_path:
    std::string str_path;
    while (regex_path) {
      str_path += attrname[regex_path->val];
      regex_path = regex_path->next;
      if (regex_path)
        str_path += ".";
    }

    // store matched strings:
    for (unsigned int i = 1; i < regex_matches.size(); i++) {
      // store vanilla string:
      string capture_name = arg->name() + ":" + str_path + ":" +
        boost::lexical_cast<std::string>(i);
#ifdef HAVE_BOOST_REGEX_ICU_HPP
      string att = Conv->convert(Conv->convert("${"+capture_name+"}"));
      string att_lc = Conv->convert(Conv->convert("${lc("+capture_name+")}"));
      string att_uc = Conv->convert(Conv->convert("${uc("+capture_name+")}"));
      UnicodeString ucval(regex_matches[i].first, regex_matches.length(i));
      string val = Conv->convert(ucval);
      string val_lc = Conv->convert(ucval.toLower());
      string val_uc = Conv->convert(ucval.toUpper());
#else
      string att = "${" + capture_name + "}";
      string att_lc = "${lc(" + capture_name + ")}";
      string att_uc = "${uc(" + capture_name + ")}";
      string val = regex_matches[i].str();
      string val_lc = val;
      string val_uc = val;
      for (string::iterator it = val_lc.begin(); it != val_lc.end(); it++)
        *it = tolower(*it);
      for (string::iterator it = val_uc.begin(); it != val_uc.end(); it++)
        *it = toupper(*it);
#endif
      captures[att] = val;
      captures[att_lc] = val_lc;
      captures[att_uc] = val_uc;
    }
    if (verbosity >= 10 || opt_chart_mapping & 2) {
#ifdef HAVE_BOOST_REGEX_ICU_HPP
      string rex_str = Conv->convert(regex.str().c_str()); // UChar32* -> string
#else
      string rex_str = regex.str();
#endif
      fprintf(stderr, "[cm] regex_match(/%s/, \"%s\")\n",
          rex_str.c_str(), str.c_str());
    }
  }

  return create(result_fs, item, arg, captures, _next_arg_idx + 1);
}

std::list<fs>
tChartMappingMatch::output_fss()
{
  // determine map of all captures:
  typedef hash_map<std::string, std::string, standard_string_hash> string_map_t;
  string_map_t captures;
  for (const tChartMappingMatch *curr = this; curr; curr = curr->_previous) {
    captures.insert(curr->_captures.begin(), curr->_captures.end());
  }

  // get list of OUTPUT fs
  list<fs> outs = _fs.get_path_value(tChartUtil::output_path()).get_list();

  // replace regex captures in each OUTPUT fs:
  std::list<fs>::iterator fs_it;
  int i;
  for (fs_it = outs.begin(), i = 0; fs_it != outs.end(); fs_it++, i++) {
    // since we want to replace the regex reference with the string capture,
    // we need a deep copy here (otherwise problems with fs structure sharing):
    fs out = *fs_it = copy(*fs_it);
    std::list<list_int*> paths = out.find_paths(BI_STRING);
    for (std::list<list_int*>::iterator path_it = paths.begin();
         path_it != paths.end();
         path_it++)
    {
      list_int *path = *path_it;
      string str = type_name(out.get_path_value(path).type());
      for (string_map_t::iterator anchor_it = captures.begin();
           anchor_it != captures.end();
           anchor_it++)
      {
        string anchor = (*anchor_it).first;
        string replacement = (*anchor_it).second;
        int pos = str.find(anchor);
        if (pos >= 0)
          str.replace(pos, anchor.length(), replacement);
      }
      out.get_path_value(path).set_type(retrieve_type(str));
      free_list(path);
    }
    assert(out.valid());
  }

  return outs;
}
