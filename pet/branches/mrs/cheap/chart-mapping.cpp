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
#include "pet-config.h"

#include "builtins.h"
#include "cheap.h"
#include "configs.h"
#include "errors.h"
#include "item.h"
#include "item-printer.h"
#include "grammar.h"
#include "hashing.h"
#include "logging.h"

#include <algorithm>
#include <cassert>
#include <climits>
#include <iostream>
#include <deque>
#include <map>
#include <string>

#include <boost/format.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include <boost/regex.hpp>
#include <boost/regex/icu.hpp>

#ifdef HAVE_ICU
#include "unicode.h"
#endif

using namespace std;
using HASH_SPACE::hash_map;
using boost::lexical_cast;
using boost::format;



// =====================================================
// Module initialization
// =====================================================

/**
 * Initializes the option(s) for this module.
 */
static bool init() {
  managed_opt("opt_chart_mapping",
              "token mapping and lexical filtering",
              0);
  return true;
}

/**
 * Variable that enforces that init() is executed when the class is loaded.
 * (Workaround for missing static blocks in C++.)
 */
static bool initialized = init();



// =====================================================
// helpers
// =====================================================

class not_contained_in_list
{
private:
  item_list _items;
public:
  not_contained_in_list(item_list items) : _items(items) { }
  bool operator() (tItem*& item) {
    for (item_iter item_it = _items.begin(); item_it != _items.end(); ++item_it)
      if (*item_it == item)
        return false;
    return true;
  }
};



// =====================================================
// class tChartMappingEngine
// =====================================================

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
  inline size_t operator() (const tChartMappingMatchSig &s) const {
    return reinterpret_cast<size_t>(s.get<0>()) +
           reinterpret_cast<size_t>(s.get<1>()) +
           reinterpret_cast<size_t>(s.get<2>()) +
           reinterpret_cast<size_t>(s.get<3>());
  }
};

typedef hash_map<tChartMappingMatchSig, tChartMappingMatch*, sig_hash>
  tChartMappingMatchCache;

static tChartMappingMatch*
get_new_completed_match(tChart &chart, tChartMappingMatch *match,
    tChartMappingMatchCache &cache, int loglevel) {
  assert(!match->is_complete());

  // loop over all suitable items for the next match:
  const tChartMappingRule *rule = match->get_rule();
  const tChartMappingRuleArg *arg = match->get_next_matching_arg();
  item_list suitable_items = match->get_suitable_items(chart, arg);
  for (item_iter it = suitable_items.begin(); it!=suitable_items.end(); ++it) {
    tItem *item = *it;
    tChartMappingMatch *next_match;
    bool new_match;

    // build the next match with item and arg or retrieve it from the cache:
    // TODO invalidate cached matches with items anchored at ^ or $
    //      if chart start and end are affected by this rule
    tChartMappingMatchSig sig(rule, match, item, arg);
    new_match = (cache.find(sig) == cache.end());
    next_match = new_match ? (cache[sig]=match->match(item, arg, loglevel)) : cache[sig];

    // logging:
    if (new_match && (LOG_ENABLED(logChartMapping, DEBUG) || (loglevel & 256))) {
      // TODO implement tItem::to_string() or tItem::printname()??
      string itemstr = "chart item " + lexical_cast<string>(item->id());
      tInputItem* inp_item = dynamic_cast<tInputItem*>(item);
      if (inp_item)
        itemstr += " (`" + inp_item->form() + "')";
      cerr << format("[cm] %s %s, arg %s with %s\n")
              % (next_match ? "MATCHED" : "checked") % rule->get_printname()
              % arg->get_name() % itemstr;
    }

    // base case and recursion:
    if (next_match) {
      if (next_match->is_complete()) {
        if (new_match) {
          return next_match;
        }
      } else {
        tChartMappingMatch *next_completed_match =
          get_new_completed_match(chart, next_match, cache, loglevel);
        if (next_completed_match) {
          return next_completed_match;
        }
      }
    }
  }

  return 0; // there were no suitable items or no further completed match
}

tChartMappingEngine::tChartMappingEngine(
    const std::list<tChartMappingRule*> &rules, const std::string &phase_name)
: _rules(rules), _phase_name(phase_name)
{
  // nothing to do
}

int tChartMappingEngine::doLogging(tChart &chart)
{
  /// \todo loglevel should be configured with logging system
  int loglevel = get_opt_int("opt_chart_mapping");
  if (LOG_ENABLED(logChartMapping, NOTICE) || loglevel & 1) {
    /* get max id for items in the chart:
     * \todo for some reason the ids of the input chart might come in any order
     *      fix this and then look at the last item only
     *      not sure whether this is still the case)
     */
    item_list items = chart.items();
    item_list::reverse_iterator it;
    int max_id = -1;
    for (it = items.rbegin(); it != items.rend(); ++it)
      if ((*it)->id() > max_id)
        max_id = (*it)->id();
    cerr << format("[cm] greatest item id before %s: %d\n")
      % _phase_name % max_id;
  }
  return loglevel;
}

void tChartMappingEngine::process(tChart &chart)
{
  assert(chart.connected());
  int loglevel = doLogging(chart);

  // cache storing each match we've created:
  tChartMappingMatchCache cache;

  // chart-mapping loop:
  bool chart_changed;
  do {
    chart_changed = false;
    list<tChartMappingRule*>::const_iterator rule_it;
    for (rule_it = _rules.begin(); rule_it != _rules.end(); ++rule_it) {
      tChartMappingRule *rule = *rule_it;
      tChartMappingMatchSig rule_sig(rule, 0, 0, 0);
      tChartMappingMatch *empty_match = (cache.find(rule_sig) == cache.end()) ?
          (cache[rule_sig] = tChartMappingMatch::create(rule, chart))
        : cache[rule_sig];
      tChartMappingMatch *completed = 0;
      do {
        completed = get_new_completed_match(chart, empty_match, cache, loglevel);
        if (completed) { // 0 if there is no further completed match
          chart_changed = completed->fire(chart, loglevel) || chart_changed;
        }
      } while (completed);
    } // for each rule
  } while (false); // quit after the first round of rule applications
  // we are still undecided whether we want to have several rounds, i.e.:
  //} while (chart_changed); // loop until fixpoint reached

  // release all created matches:
  tChartMappingMatchCache::iterator match_it;
  for (match_it = cache.begin(); match_it != cache.end(); ++match_it)
    delete match_it->second; // delete 0 is safe according C++ Standard

  // check whether the chart is still wellformed:
  if (!chart.connected()) {
    throw tError("Chart is not well-formed after chart mapping. "
      "This is probably a bug in the grammar.");
  }
  doLogging(chart);
}



// =====================================================
// class tChartMappingRuleArg
// =====================================================

tChartMappingRuleArg::tChartMappingRuleArg(const std::string &name,
    tChartMappingRuleArg::Trait trait, int nr, tPathRegexMap &regexs)
: _name(name),
  _start_anchor(name + ":s"),
  _end_anchor(name + ":e"),
  _trait(trait),
  _nr(nr),
  _regexs(regexs) {
  // done
}


tChartMappingRuleArg::~tChartMappingRuleArg() {
  for (tPathRegexMap::iterator it = _regexs.begin(); it != _regexs.end(); ++it)
    free_list(it->first);
}

tChartMappingRuleArg*
tChartMappingRuleArg::create(const std::string &name,
    tChartMappingRuleArg::Trait trait, int nr, tPathRegexMap &regexs) {
  return new tChartMappingRuleArg(name, trait, nr, regexs);
}

const std::string&
tChartMappingRuleArg::get_name() const {
  return _name;
}

const std::string&
tChartMappingRuleArg::get_start_anchor() const {
  return _start_anchor;
}

const std::string&
tChartMappingRuleArg::get_end_anchor() const {
  return _end_anchor;
}

tChartMappingRuleArg::Trait
tChartMappingRuleArg::get_trait() const {
  return _trait;
}

int
tChartMappingRuleArg::get_nr() const {
  return _nr;
}

const tPathRegexMap&
tChartMappingRuleArg::get_regexs() const {
  return _regexs;
}



// =====================================================
// class tChartMappingAnchoringGraph
// =====================================================

tChartMappingAnchoringGraph::tChartMappingAnchoringGraph() {
  _vs = add_vertex("^");
  _ve = add_vertex("$");
  accomodate_edge(_vs, _ve, 0, INT_MAX);
}

void
tChartMappingAnchoringGraph::add_arg(const tChartMappingRuleArg *arg) {
  _arg_names.push_back(arg->get_name());

  tVertex v1 = add_vertex(arg->get_start_anchor());
  tVertex v2 = add_vertex(arg->get_end_anchor());
  tChartMappingAnchoringGraph::tEdgeProp::tType t = (arg->get_trait()
      == tChartMappingRuleArg::OUTPUT_ARG) ? tEdgeProp::OUTPUT
      : tEdgeProp::MATCHING;
  accomodate_edge(v1, v2, 1, 1, t);

  // matching arguments are connected to the chart start and end
  if (t == tEdgeProp::MATCHING) {
    accomodate_edge(_vs, v1, 0, INT_MAX);
    accomodate_edge(v2, _ve, 0, INT_MAX);
  }
}

tChartMappingAnchoringGraph::tVertex
tChartMappingAnchoringGraph::add_vertex(std::string a) {
  assert(_vertices.find(a) == _vertices.end());
  tVertex v = boost::add_vertex(_graph);
  _vertices[a] = v;
  _anchors[v].push_back(a);
  return v;
}

void
tChartMappingAnchoringGraph::merge_vertices(std::string a1, std::string a2) {
  assert(_vertices.find(a1) != _vertices.end());
  assert(_vertices.find(a2) != _vertices.end());
  tVertex v1 = _vertices[a1];
  tVertex v2 = _vertices[a2];
  merge_vertices(v1, v2);
}

void
tChartMappingAnchoringGraph::merge_vertices(tVertex v1, tVertex v2) {
  assert(v1 != v2);

  // delete all edges between v1 and v2 or vice versa
  {
    tOutEdgeIt ei, ei_end, ei_next;
    for (int i = 0; i < 2; ++i) {
      boost::tie(ei, ei_end) = boost::edge_range(v1, v2, _graph);
      for (ei_next = ei; ei != ei_end; ei = ei_next) {
        ++ei_next; // remains stable if graph is altered
        tEdge edge = *ei;
        if (_graph[edge].min == 0) {
          boost::remove_edge(edge, _graph);
        } else {
          throw tError("Cannot merge vertices " + get_anchors_string(v1) +
              " and " + get_anchors_string(v2) + " since a path of minimal "
              "length " + boost::lexical_cast<string>(_graph[edge].min) +
              " is required here.");
        }
      }
      std::swap(v1, v2);
    }
  }

  // bend all in-edges of v2 such that they end in v1:
  // We need to store the next iterator since deleting the item of the
  // current iterator invalidates the iterator itself.
  {
    tInEdgeIt ei, ei_end, ei_next;
    boost::tie(ei, ei_end) = boost::in_edges(v2, _graph);
    for (ei_next = ei; ei != ei_end; ei = ei_next) {
      ++ei_next; // remains stable if graph is altered
      tEdge edge = *ei;
      tVertex v0 = boost::source(edge, _graph);
      int min = _graph[edge].min;
      int max = _graph[edge].max;
      tEdgeProp::tType type = _graph[edge].type;
      boost::remove_edge(edge, _graph);
      accomodate_edge(v0, v1, min, max, type);
    }
  }

  // bend all out-edges of v2 such that they start in v1:
  // We need to store the next iterator since deleting the item of the
  // current iterator invalidates the iterator itself.
  {
    tOutEdgeIt ei, ei_end, ei_next;
    boost::tie(ei, ei_end) = boost::out_edges(v2, _graph);
    for (ei_next = ei; ei != ei_end; ei = ei_next) {
      ++ei_next; // remains stable if graph is altered
      tEdge edge = *ei;
      tVertex v3 = boost::target(edge, _graph);
      int min = _graph[edge].min;
      int max = _graph[edge].max;
      tEdgeProp::tType type = _graph[edge].type;
      boost::remove_edge(edge, _graph);
      accomodate_edge(v1, v3, min, max, type);
    }
  }

  // adapt _vertices and _anchors:
  for (tStringsIt it = _anchors[v2].begin(); it != _anchors[v2].end(); ++it) {
    _vertices[*it] = v1;
    _anchors[v1].push_back(*it);
  }
  _anchors.erase(v2);

  boost::remove_vertex(v2, _graph);
}

void
tChartMappingAnchoringGraph::accomodate_edge(std::string a1, std::string a2,
    int min, int max, tEdgeProp::tType t) {
  if (_vertices.find(a1) == _vertices.end())
      throw tError("Anchor `" + a1 + "' unknown.");
  if (_vertices.find(a2) == _vertices.end())
    throw tError("Anchor `" + a2 + "' unknown.");
  tVertex v1 = _vertices[a1];
  tVertex v2 = _vertices[a2];
  accomodate_edge(v1, v2, min, max, t);
}

void
tChartMappingAnchoringGraph::accomodate_edge(tVertex v1, tVertex v2,
    int min, int max, tEdgeProp::tType t) {

  assert(v1 != v2);

  // try to find an edge to restrict:
  if ((t == tEdgeProp::PATH) || (t == tEdgeProp::TENTATIVE)) {
    tOutEdgeIt ei, ei_end, ei_next;
    boost::tie(ei, ei_end) = boost::edge_range(v1, v2, _graph);
    for (ei_next = ei; ei != ei_end; ei = ei_next) {
      ++ei_next; // remains stable if graph is altered
      tEdge e = *ei;
      if (t == tEdgeProp::PATH) {
        int new_min = std::max(min, _graph[e].min);
        int new_max = std::min(max, _graph[e].max);
        if (new_min <= new_max) {
          _graph[e].min = new_min;
          _graph[e].max = new_max;
          return;
        }
      } else if (_graph[e].type == tEdgeProp::TENTATIVE) { // == t
        return;
      }
    }
  }

  // add new edge (unless an edge was restricted before):
  boost::add_edge(v1, v2, tEdgeProp(min, max, t), _graph);
}

void
tChartMappingAnchoringGraph::index() {
  boost::property_map<tGraph, boost::vertex_index_t>::type
      index_map = boost::get(boost::vertex_index, _graph);
  int max_vertex_index = 0;
  tVertexIt vi, vi_end;
  for (boost::tie(vi, vi_end) = boost::vertices(_graph); vi != vi_end; ++vi)
    boost::put(index_map, *vi, max_vertex_index++);
}

void
tChartMappingAnchoringGraph::realize_tentative_edges(tVertices &v_order) {
  // TODO detect that "O1@I1,O1@I2" is not allowed

  // iterate over all arguments:
  for (tStringsIt it = _arg_names.begin(); it != _arg_names.end(); ++it) {
    tVertex v[2];
    v[0] = _vertices[*it + ":s"];
    v[1] = _vertices[*it + ":e"];

    // realize tentative edges of v1 and v2:
    for (int i = 0; i < 2; ++i) {
      tVertex outer_v = boost::graph_traits<tGraph>::null_vertex();
      tInEdgeIt ei, ei_end, ei_next;
      boost::tie(ei, ei_end) = boost::in_edges(v[i], _graph);
      for (ei_next = ei; ei != ei_end; ei = ei_next) {
        ++ei_next; // remains stable if graph is altered
        tEdge e = *ei;
        if (_graph[e].type == tEdgeProp::TENTATIVE) { // tentative edge
          tVertex v0 = boost::source(e, _graph);
          tVerticesIt vo_it, vo_end;
          if (i == 0) { // v[i] is start node of argument
            vo_it = v_order.end() - 1;
            vo_end = v_order.begin() - 1;
          } else { // v[i] is end node of argument
            vo_it = v_order.begin();
            vo_end = v_order.end();
          }
          for ( ; vo_it != vo_end; std::advance(vo_it, i*2 - 1)) {
            if ((*vo_it == outer_v) || (*vo_it == v0)) {
              outer_v = *vo_it;
              break;
            }
          }
          boost::remove_edge(e, _graph);
        }
      }
      if (outer_v != boost::graph_traits<tGraph>::null_vertex()) {
        merge_vertices(outer_v, v[i]);
        v_order.erase(std::find(v_order.begin(), v_order.end(), v[i]));
      }
    }
  }
}

void
tChartMappingAnchoringGraph::transitive_closure(tVertices &v_order) {
  // The only (min,max)-constraints that are specified in the rules are
  // (0,0) and (0,*)-constraints; the rest are (1,1)-contraints for the items
  // to be matched and (min,max)-constraints computed by transitive closure
  // by an "upwards"-propagation of (min,max)-constraints. It is in particular
  // not allowed (yet?) to posit arbitrary (min,max)-constraints between
  // arbitrary arguments. This would require a propagation of
  // (min,max)-constraints "downwards".

  tVerticesRevIt v1_it, v2_it;
  for (v1_it = v_order.rbegin(); v1_it != v_order.rend(); ++v1_it) {
    for (v2_it = v1_it + 1; v2_it != v_order.rend(); ++v2_it) {
      tVertex v1 = *v1_it;
      tVertex v2 = *v2_it;

      // accomodate edge (v1, v2) if there is an edge (v1, v) and (v, v2)
      tInEdgeIt e2i, e2i_end;
      boost::tie(e2i, e2i_end) = boost::in_edges(v2, _graph);
      for ( ; e2i != e2i_end; ++e2i) {
        tEdge e2 = *e2i;
        if (_graph[e2].type == tEdgeProp::OUTPUT)
          continue;
        tVertex v = boost::source(e2, _graph);

        // it seems that the end iterator returned by edge_range is
        // invalidated once the edge set of the graph is changed. therefore
        // storing the edges to be accomodated in the graph temporarily
        typedef boost::tuple<tVertex, tVertex, int, int> tStoredEdge;
        typedef std::vector<tStoredEdge> tStoredEdges;
        tStoredEdges s;

        tOutEdgeIt e1i, e1i_end;
        boost::tie(e1i, e1i_end) = boost::edge_range(v1, v, _graph);
        for ( ; e1i != e1i_end; ++e1i) {
          tEdge e1 = *e1i;
          if (_graph[e1].type == tEdgeProp::OUTPUT)
            continue;
          int min = _graph[e1].min + _graph[e2].min;
          int max = ((_graph[e1].max==INT_MAX)||(_graph[e2].max==INT_MAX)) ?
              INT_MAX : _graph[e1].max + _graph[e2].max;
          s.push_back(tStoredEdge(v1, v2, min, max));
        }

        // accomodate the stored edges (since adding them in the loop before
        // would invalidate e1i_end
        for (tStoredEdges::iterator i = s.begin(); i != s.end(); ++i) {
          tStoredEdge se = *i;
          accomodate_edge(se.get<0>(), se.get<1>(), se.get<2>(), se.get<3>());
        }
      }
    }
  }
}

/**
 * A visitor class for counting the number of components of a graph and
 * checking whether it is acyclic.
 */
struct tGraphChecker: public boost::dfs_visitor<>
{
  tGraphChecker(bool &has_cycle, int &component_count)
  : _has_cycle(has_cycle), _component_count(component_count) {
    assert(_has_cycle == false);
    assert(_component_count == 0);
  }

  template<class Vertex, class Graph>
  void start_vertex(Vertex, Graph&) {
    ++_component_count;
  }

  template <class Edge, class Graph>
  void back_edge(Edge, Graph&) {
    _has_cycle = true;
  }

protected:
  bool &_has_cycle;
  int &_component_count;
};

bool
tChartMappingAnchoringGraph::check_consistency() {
  index();
  bool has_cycle = false;
  int nr_components = 0;
  tGraphChecker checker(has_cycle, nr_components);
  boost::depth_first_search(_graph, boost::visitor(checker));
  if (has_cycle)
    throw tError("Contradicting requirements.");
  if (nr_components != 1)
    throw tError("Some arguments are not anchored.");
  return true;
}

void
tChartMappingAnchoringGraph::finalize() {
  index();
  tVertices v_order; // edge (u,v) in the graph --> v before u
  try {
    boost::topological_sort(_graph, std::back_inserter(v_order));
  } catch (boost::not_a_dag e) {
    throw tError("Contradicting requirements.");
  }
  realize_tentative_edges(v_order);
  transitive_closure(v_order);
  check_consistency();
}

const tChartMappingAnchoringGraph::tVertex
tChartMappingAnchoringGraph::get_vertex(std::string anchor) const {
  return _vertices.find(anchor)->second;
}

void
tChartMappingAnchoringGraph::print() const {
  cerr << "nodes:" << endl;
  tVertexIt vi, vend;
  for (boost::tie(vi, vend) = boost::vertices(_graph); vi != vend; ++vi)
    cerr << get_anchors_string(*vi) << endl;

  cerr << endl;

  cerr << "edges:" << endl;
  tEdgeIt ei, ei_end;
  for (boost::tie(ei, ei_end) = boost::edges(_graph); ei != ei_end; ++ei) {
    tEdge edge = *ei;
    string src_anchors = get_anchors_string(boost::source(edge, _graph));
    string tgt_anchors = get_anchors_string(boost::target(edge, _graph));
    string type;
    switch (_graph[edge].type) {
      case tEdgeProp::MATCHING:
        type = "matching"; break;
      case tEdgeProp::OUTPUT:
        type = "output"; break;
      case tEdgeProp::PATH:
        type = "path"; break;
      case tEdgeProp::TENTATIVE:
        type = "tentative"; break;
    }
    cerr << boost::format("(%s, %s, %d, %d, %s)") % src_anchors % tgt_anchors
        % _graph[edge].min % _graph[edge].max
        % type
        << endl;
  }
}

std::string
tChartMappingAnchoringGraph::get_anchors_string(tVertex v) const {
  string result = "(";
  tVertexStringsMap::const_iterator it1 = _anchors.find(v);
  if (it1 != _anchors.end()) {
    const tStrings &anchors = it1->second;
    tStrings::const_iterator it2;
    for (it2 = anchors.begin(); it2 != anchors.end(); ++it2) {
      result.append(*it2);
      if (*it2 != anchors.back())
        result.append(",");
    }
  }
  result.append(")");
  return result;
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
  for (it = regex_paths.begin(); it != regex_paths.end(); ++it) {
    list_int *regex_path = *it;
    string arg_val = get_typename(arg_fs.get_path_value(regex_path).type());
#ifdef HAVE_BOOST_REGEX_ICU_HPP
    UnicodeString uc_arg_val = Conv->convert(arg_val);
    static UnicodeString rex_start("\"^", -1, US_INV);
    static UnicodeString rex_end  ("$\"", -1, US_INV);
    int32_t len = uc_arg_val.length();
    if ((len >= 4) && uc_arg_val.startsWith(rex_start)
        && uc_arg_val.endsWith(rex_end)) {
      uc_arg_val.setTo(uc_arg_val, 2, len - 4);
      regexs[regex_path] = boost::make_u32regex(uc_arg_val);
      arg_fs.get_path_value(regex_path).set_type(BI_STRING);
    } else {
      free_list(regex_path);
    }
#else
    string::iterator begin = arg_val.begin();
    string::iterator end = arg_val.end();
    if ((arg_val.length() >= 4) &&
        (*(begin++) == '"') && (*(begin++) == '^') &&
        (*(--end)   == '"') && (*(--end)   == '$')) {
      arg_val.assign(begin, end); // use the regex string only
      regexs[regex_path] = boost::regex(arg_val);
      arg_fs.get_path_value(regex_path).set_type(BI_STRING);
    } else {
      free_list(regex_path);
    }
#endif
  }
}

tChartMappingRule::tChartMappingRule(type_t type, Trait trait)
: _type(type), _trait(trait) {
  // get the feature structure representation of this rule. this must
  // be a deep copy of the type's feature structure since we need to
  // modify this structure:
  _fs = copy(fs(_type));

  // create argument representations for this rule:
  typedef boost::tuple<const list_int*,tChartMappingRuleArg::Trait,string> tSig;
  list<tSig> sigs;
  sigs.push_back(tSig(tChartUtil::context_path(),
      tChartMappingRuleArg::CONTEXT_ARG, "C"));
  sigs.push_back(tSig(tChartUtil::input_path(),
      tChartMappingRuleArg::INPUT_ARG, "I"));
  sigs.push_back(tSig(tChartUtil::output_path(),
      tChartMappingRuleArg::OUTPUT_ARG, "O"));
  for (list<tSig>::iterator it = sigs.begin(); it != sigs.end(); ++it) {
    list<fs> arg_fss = _fs.get_path_value((*it).get<0>()).get_list();
    tChartMappingRuleArg::Trait trait = (*it).get<1>();
    string prefix = (*it).get<2>();
    int i;
    std::list<fs>::iterator fs_it;
    for (fs_it = arg_fss.begin(), i = 1; fs_it != arg_fss.end(); ++fs_it, ++i) {
      try {
        tPathRegexMap regexs;
        modify_arg_fs(*fs_it, regexs);
        string name = prefix + lexical_cast<std::string> (i);
        tChartMappingRuleArg *arg = tChartMappingRuleArg::create(name, trait,
            i, regexs);
        _args.push_back(arg);
        if (trait == tChartMappingRuleArg::OUTPUT_ARG)
          _output_args.push_back(arg);
        else
          _matching_args.push_back(arg);
      } catch (boost::regex_error e) {
        throw tError("Could not compile regex for rule " + get_typename(type)
            + ".");
      }
    }
  }

  // parse positional constraints specification:
  string poscons_s;
  fs poscons_fs = _fs.get_path_value(tChartUtil::poscons_path());
  if (poscons_fs.type() != BI_STRING) // there are positional constraints
    poscons_s = poscons_fs.printname();
  evaluate_poscons(poscons_s);
}

tChartMappingRule*
tChartMappingRule::create(type_t type, tChartMappingRule::Trait trait)
{
  try {
    return new tChartMappingRule(type, trait);
  } catch (tError error) {
    cerr << error.getMessage() << endl;
  }
  return 0;
}

type_t
tChartMappingRule::get_type() const
{
  return _type;
}

tChartMappingRule::Trait
tChartMappingRule::get_trait() const
{
  return _trait;
}

const char*
tChartMappingRule::get_printname() const
{
  return print_name(_type);
}

fs
tChartMappingRule::instantiate() const
{
  return copy(_fs);
}

const tChartMappingRuleArg*
tChartMappingRule::get_arg(std::string name) const
{
  typedef std::vector<const tChartMappingRuleArg*>::const_iterator tArgIter;
  for (tArgIter it = _args.begin(); it != _args.end(); ++it) {
    if ((*it)->get_name() == name)
      return *it;
  }
  return 0;
}

const std::vector<const tChartMappingRuleArg*> &
tChartMappingRule::get_args() const
{
  return _args;
}

const std::vector<const tChartMappingRuleArg*>
tChartMappingRule::get_args(tChartMappingRuleArg::Trait trait) const
{
  std::vector<const tChartMappingRuleArg*> args;
  std::vector<const tChartMappingRuleArg*>::const_iterator arg_it;
  for (arg_it = _args.begin(); arg_it != _args.end(); ++arg_it) {
    if ((*arg_it)->get_trait() == trait)
      args.push_back(*arg_it);
  }
  return args;
}

const std::vector<const tChartMappingRuleArg*> &
tChartMappingRule::get_matching_args() const
{
  return _matching_args;
}

const std::vector<const tChartMappingRuleArg*> &
tChartMappingRule::get_output_args() const
{
  return _output_args;
}

const tChartMappingAnchoringGraph &
tChartMappingRule::get_anchoring_graph() const {
  return _anch_graph;
}

void
tChartMappingRule::evaluate_poscons(string poscons_s) {
  // type for positional constraints:
  // <arg1, arg2, min_distance, max_distance, parallels, original spec>
  typedef boost::tuple<string, string, int, int, bool, string> tPosConstraint;
  typedef std::list<tPosConstraint> tPosConstraints;
  tPosConstraints poscons;

  // parse positional constraints specification (consuming poscons_s):
  poscons_s.append(",");
  boost::smatch m;
  boost::regex re("[ \t\r\n]*"   // optional white space
      "(?:"                      // begin optional term chain
      "(\\^|\\$|[CIO][0-9]+)"    // arg1_name ($1)
      "[ \t\r\n]*"               // optional white space
      "(?:"                      // begin optional rel_name arg2_name
      "(<|<<|>|>>|@)"            // rel_name ($2)
      "[ \t\r\n]*"               // optional white space
      "(\\^|\\$|[CIO][0-9]+)"    // arg2_name ($3)
      "[ \t\r\n]*"               // optional white space
      "([^,]*)"                  // optional term chain tail ($4)
      ")?"                       // end optional rel_name arg2_name
      ")?"                       // end optional arg1_name rel_name arg2_name
      ","                        // comma is required
      "[ \t\r\n]*"               // optional white space
      "(.*)");                   // optional rest ($5)
  while (boost::regex_match(poscons_s, m, re)) {
    string a1 = m[1].str();
    string a2 = m[3].str();
    string rel = m[2].str();
    string tail = m[4].str();
    string rest = m[5].str();
    string spec = a1+rel+a2;
    if (!a1.empty() && !get_arg(a1) && (a1 != "^") && (a1 != "$")) {
      throw tError("Unknown argument `" + a1 + "' in positional constraints"
          " of chart mapping rule `" + get_printname() + "'.");
    }
    if (!a2.empty() && !get_arg(a2) && (a2 != "^") && (a2 != "$")) {
      throw tError("Unknown argument `" + a2 + "' in positional constraints"
          " of chart mapping rule `" + get_printname() + "'.");
    }
    if (!a1.empty() && !a2.empty()) {
      if (rel == "@") {
        poscons.push_back(tPosConstraint(a1, a2, 0, 0, true, spec));
      } else if (rel == "<") {
        poscons.push_back(tPosConstraint(a1, a2, 0, 0, false, spec));
      } else if (rel == ">") {
        poscons.push_back(tPosConstraint(a2, a1, 0, 0, false, spec));
      } else if (rel == "<<") {
        poscons.push_back(tPosConstraint(a1, a2, 0, INT_MAX, false, spec));
      } else if (rel == ">>") {
        poscons.push_back(tPosConstraint(a2, a1, 0, INT_MAX, false, spec));
      }
    }
    // consume current constraint:
    poscons_s = (tail.empty() ? "" : a2 + tail + ",") + rest;
  }
  if (!poscons_s.empty()) {
    poscons_s.erase(poscons_s.find(","));
    throw tError("Ill-formed positional constraints for chart mapping rule `"
        + std::string(get_printname()) + "': " + poscons_s);
  }

  // initialize anchoring graph by adding all arguments:
  tRuleArgs::iterator ra_it;
  for (ra_it = _args.begin(); ra_it != _args.end(); ++ra_it)
    _anch_graph.add_arg(*ra_it);

  // add positional constraints to anchoring graph
  string spec;
  try {
    while (!poscons.empty()) {
      tPosConstraint constraint = poscons.front();
      poscons.pop_front();
      string a1 = constraint.get<0>();
      string a2 = constraint.get<1>();
      int min = constraint.get<2>();
      int max = constraint.get<3>();
      bool parallels = constraint.get<4>();
      spec = constraint.get<5>();
      assert((min >= 0) && (max >= 0));
      if (max > 0) {                  // => succeeds-relation
        _anch_graph.accomodate_edge(a1 + (a1.size() > 1 ? ":e" : ""),
            a2 + (a2.size() > 1 ? ":s" : ""), min, max);
      } else if (!parallels) {        // => vertex identity
        _anch_graph.merge_vertices(a1 + (a1.size() > 1 ? ":e" : ""),
            a2 + (a2.size() > 1 ? ":s" : ""));
      } else {                        // => parallels-relation (min==max==0)
        if ((a1.size() == 1) || (a2.size() == 1))
          throw tError("Cannot anchor `^' and `$' with parallels-relation.");
        _anch_graph.accomodate_edge(a2 + ":s", a1 + ":s", 0, 0,
            tChartMappingAnchoringGraph::tEdgeProp::TENTATIVE);
        _anch_graph.accomodate_edge(a2 + ":e", a1 + ":e", 0, 0,
            tChartMappingAnchoringGraph::tEdgeProp::TENTATIVE);
      }
    }
    spec = "";
    _anch_graph.finalize();
  } catch (tError e) {
    ostringstream os;
    os << boost::format("Error in chart mapping rule `%s'%s: %s")
                        % get_printname()
                        % (spec.empty() ? "" : (":" + spec))
                        % e.getMessage();
    throw tError(os.str());
  }
}




// =====================================================
// class tChartMappingMatch
// =====================================================

tChartMappingMatch::tChartMappingMatch(const tChartMappingRule *rule,
    const tChartMappingMatch *previous,
    const fs &f,
    const tArgItemMap &arg_item_map,
    const std::map<std::string, std::string> &captures,
    const tAnchoringToChartVertexMap &vertex_binding,
    unsigned int next_arg_idx)
: _rule(rule),
  _anch_graph(rule->get_anchoring_graph()),
  _previous(previous),
  _fs(f),
  _arg_item_map(arg_item_map),
  _captures(captures),
  _vertex_binding(vertex_binding),
  _next_arg_idx(next_arg_idx)
{
  // done
}

tChartMappingMatch*
tChartMappingMatch::create(const tChartMappingRule *rule, const tChart &chart) {
  tAnchoringToChartVertexMap vertex_binding;
  const tChartMappingAnchoringGraph &anch_graph = rule->get_anchoring_graph();
  vertex_binding[anch_graph.get_vertex("^")] = chart.start_vertices().front();
  vertex_binding[anch_graph.get_vertex("$")] = chart.end_vertices().front();
  return new tChartMappingMatch(rule, 0, rule->instantiate(), tArgItemMap(),
      std::map<std::string, std::string>(), vertex_binding, 0);
}

tChartMappingMatch*
tChartMappingMatch::create(const fs &f,
    const tArgItemMap &arg_item_map,
    const std::map<std::string, std::string> &captures,
    const tAnchoringToChartVertexMap &vertex_binding,
    unsigned int next_arg_idx) {
  return new tChartMappingMatch(_rule, this, f, arg_item_map, captures,
      vertex_binding, next_arg_idx);
}

bool
tChartMappingMatch::is_complete() const {
  return (_next_arg_idx == _rule->get_matching_args().size());
}

const tChartMappingRule*
tChartMappingMatch::get_rule() const {
  return _rule;
}

fs
tChartMappingMatch::get_fs() {
  return _fs;
}

fs
tChartMappingMatch::get_arg_fs(const tChartMappingRuleArg *arg) {
  const list_int *path;
  switch (arg->get_trait()) {
    case tChartMappingRuleArg::CONTEXT_ARG:
      path = tChartUtil::context_path();
      break;
    case tChartMappingRuleArg::INPUT_ARG:
      path = tChartUtil::input_path();
      break;
    case tChartMappingRuleArg::OUTPUT_ARG:
      path = tChartUtil::output_path();
      break;
    default:
      path = 0; // just to avoid complaints by the compiler
  }
  return _fs.nth_value(path, arg->get_nr());
}


const tChartMappingRuleArg*
tChartMappingMatch::get_next_matching_arg() const {
  return is_complete() ? 0 : _rule->get_matching_args()[_next_arg_idx];
}

tItem*
tChartMappingMatch::get_item(const tChartMappingRuleArg* arg) {
  tItem *item = 0;
  for (const tChartMappingMatch *curr = this; curr; curr = curr->_previous) {
    item = curr->_arg_item_map.find(arg)->second;
    if (item)
      break;
  }
  return item;
}

item_list
tChartMappingMatch::get_items() {
  item_list items;
  for (const tChartMappingMatch *curr = this; curr; curr = curr->_previous) {
    for (tArgItemMap::const_iterator it = curr->_arg_item_map.begin();
      it != curr->_arg_item_map.end(); ++it) {
        items.push_back(it->second);
    }
  }
  return items;
}

item_list
tChartMappingMatch::get_items(tChartMappingRuleArg::Trait trait) {
  item_list items;
  for (const tChartMappingMatch *curr = this; curr; curr = curr->_previous) {
    for (tArgItemMap::const_iterator it = curr->_arg_item_map.begin();
      it != curr->_arg_item_map.end(); ++it) {
        if ((*it).first->get_trait() == trait)
          items.push_back(it->second);
    }
  }
  return items;
}

item_list
tChartMappingMatch::get_suitable_items(tChart &chart,
    const tChartMappingRuleArg *arg) {

  assert(arg != 0);

  // matched items shall not be matched twice:
  item_list skip = get_items();

  // start and end anchoring graph vertices and corresponding chart vertices:
  tChartMappingAnchoringGraph::tVertex av_arg_s, av_arg_e, av_next;
  tChartVertex *cv_arg_s, *cv_arg_e, *cv_next;
  av_arg_s = _anch_graph.get_vertex(arg->get_start_anchor());
  av_arg_e = _anch_graph.get_vertex(arg->get_end_anchor());
  cv_arg_s = _vertex_binding[av_arg_s]; // sets 0 if not found
  cv_arg_e = _vertex_binding[av_arg_e]; // sets 0 if not found

  // TODO make this much more efficient!!! (intersecting item lists is not good)
  item_list items = chart.items(cv_arg_s, cv_arg_e, true, true, skip);
  if (!cv_arg_s) {
    tChartMappingAnchoringGraph::tInEdgeIt in_it, in_it_end;
    boost::tie(in_it, in_it_end) = boost::in_edges(av_arg_s, _anch_graph._graph);
    for (; (in_it != in_it_end) && !items.empty(); ++in_it) {
      tChartMappingAnchoringGraph::tEdge edge = *in_it;
      if (_anch_graph._graph[edge].type
          != tChartMappingAnchoringGraph::tEdgeProp::OUTPUT) {
        av_next = boost::source(edge,_anch_graph._graph);
        cv_next = _vertex_binding[av_next];
        if (cv_next) { // there is an edge to a bound vertex
          int min = _anch_graph._graph[edge].min;
          int max = _anch_graph._graph[edge].max;
          item_list restr = chart.succeeding_items(cv_next, min, max);
          items.remove_if(not_contained_in_list(restr));
        }
      }
    }
  }
  if (!cv_arg_e) {
    tChartMappingAnchoringGraph::tOutEdgeIt out_it, out_it_end;
    boost::tie(out_it, out_it_end) = boost::out_edges(av_arg_e, _anch_graph._graph);
    for (; (out_it != out_it_end) && !items.empty(); ++out_it) {
      tChartMappingAnchoringGraph::tEdge edge = *out_it;
      if (_anch_graph._graph[edge].type
          != tChartMappingAnchoringGraph::tEdgeProp::OUTPUT) {
        av_next = boost::target(edge,_anch_graph._graph);
        cv_next = _vertex_binding[av_next];
        if (cv_next) { // there is an edge to a bound vertex
          int min = _anch_graph._graph[edge].min;
          int max = _anch_graph._graph[edge].max;
          item_list restr = chart.preceding_items(cv_next, min, max);
          items.remove_if(not_contained_in_list(restr));
        }
      }
    }
  }

  return items;
}

tChartMappingMatch*
tChartMappingMatch::match(tItem *item, const tChartMappingRuleArg *arg, int loglevel) {
  assert(!is_complete());

  fs root_fs = get_fs();
  fs arg_fs = get_arg_fs(arg);
  fs item_fs = item->get_fs();

  // 1. try to unify argument with item (ignoring regexs for now):
  fs result_fs = unify(root_fs, arg_fs, item_fs);
  if (!result_fs.valid())
    return 0;

  // 2. check for each regex path whether it matches in item:
  const tPathRegexMap &regexs = arg->get_regexs();
  std::map<std::string, std::string> captures(_captures);
  for (tPathRegexMap::const_iterator it = regexs.begin();
       it != regexs.end();
       ++it) {
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

    // store new regex captures:
    for (unsigned int i = 1; i < regex_matches.size(); ++i) {
      // store vanilla string:
      string capture_name = arg->get_name() + ":" + str_path + ":" +
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
      for (string::iterator it = val_lc.begin(); it != val_lc.end(); ++it)
        *it = tolower(*it);
      for (string::iterator it = val_uc.begin(); it != val_uc.end(); ++it)
        *it = toupper(*it);
#endif
      captures[att] = val;
      captures[att_lc] = val_lc;
      captures[att_uc] = val_uc;
    }
    if (LOG_ENABLED(logChartMapping, DEBUG) || loglevel & 2) {
#ifdef HAVE_BOOST_REGEX_ICU_HPP
      string rex_str = Conv->convert(regex.str().c_str()); // UChar32* -> string
#else
      string rex_str = regex.str();
#endif
      cerr << format("[cm] regex_match(/%s/, \"%s\")\n") % rex_str % str;
    }
  }

  // add current arg/item-binding:
  tArgItemMap arg_item_map(_arg_item_map);
  arg_item_map[arg] = item;

  // add current anchoring-graph/chart vertex bindings:
  tAnchoringToChartVertexMap vertex_binding(_vertex_binding);
  tChartMappingAnchoringGraph::tVertex av_s, av_e;
  av_s = _anch_graph.get_vertex(arg->get_start_anchor());
  av_e = _anch_graph.get_vertex(arg->get_end_anchor());
  assert(!vertex_binding[av_s] || (vertex_binding[av_s]==item->prec_vertex()));
  assert(!vertex_binding[av_e] || (vertex_binding[av_e]==item->succ_vertex()));
  vertex_binding[av_s] = item->prec_vertex();
  vertex_binding[av_e] = item->succ_vertex();

  return create(result_fs, arg_item_map, captures, vertex_binding,
      _next_arg_idx + 1);
}

bool
tChartMappingMatch::fire(tChart &chart, int loglevel) {
  assert(is_complete());

  bool chart_changed = false;
  tChartMappingRule::Trait rule_trait = get_rule()->get_trait();
  item_list inps = get_items(tChartMappingRuleArg::INPUT_ARG);
  item_list cons = get_items(tChartMappingRuleArg::CONTEXT_ARG);

  // add all OUTPUT items:
  tRuleArgs out_args = get_rule()->get_args(tChartMappingRuleArg::OUTPUT_ARG);
  tRuleArgs::iterator oa_it;
  for (oa_it = out_args.begin(); oa_it != out_args.end(); ++oa_it) {
    const tChartMappingRuleArg *out_arg = *oa_it;
    fs out_fs = build_output_fs(out_arg);
    tChartMappingAnchoringGraph::tVertex av_s, av_e;
    av_s = _anch_graph.get_vertex(out_arg->get_start_anchor());
    av_e = _anch_graph.get_vertex(out_arg->get_end_anchor());
    tChartVertex *cv_s = _vertex_binding[av_s];
    tChartVertex *cv_e = _vertex_binding[av_e];
    tItem *out_item = 0;

    // try to find an existing identical item for reuse:
    if (cv_s && cv_e) { // prec and succ are existing chart vertices
      item_list items = chart.items(cv_s, cv_e, true);
      for (item_iter i_it = items.begin(); i_it != items.end(); ++i_it) {
        fs item_fs = (*i_it)->get_fs();
        bool forward = true, backward = true;
        subsumes(out_fs, item_fs, forward, backward);
        if (forward && backward) { // identical item
          out_item = *i_it;
          inps.remove(out_item); // don't remove this identical item later
        }
      }
    } else { // create chart vertices for prec and/or succ
      if (!cv_s) {
        cv_s = chart.add_vertex(tChartVertex::create());
        _vertex_binding[av_s] = cv_s;
      }
      if (!cv_e) {
        cv_e = chart.add_vertex(tChartVertex::create());
        _vertex_binding[av_e] = cv_e;
      }
    }

    // if output item is not reused, create a new one and add it:
    if (out_item == 0) {
      if (rule_trait == tChartMappingRule::TMR_TRAIT) {
        out_item = tChartUtil::create_input_item(out_fs);
      } else if (rule_trait == tChartMappingRule::LFR_TRAIT) {
        throw tError("tChartMappingEngine: Construction of lexical items not "
            "allowed.");
      }
      chart.add_item(out_item, cv_s, cv_e);
      chart_changed = true;
    }

    // remember arg/item binding:
    _arg_item_map[out_arg] = out_item;

  } // for all out_args

  // freeze all INPUT items in the chart:
  for (item_iter it = inps.begin(); it != inps.end(); ++it)
    (*it)->freeze(false);
  chart_changed = chart_changed || !inps.empty();

  // logging:
  if (LOG_ENABLED(logChartMapping, INFO) || loglevel & 1 || loglevel & 16) {
    ostringstream item_ids_os;
    std::vector<const tChartMappingRuleArg*> args = get_rule()->get_args();
    std::vector<const tChartMappingRuleArg*>::iterator arg_it;
    for (arg_it = args.begin(); arg_it != args.end(); ++arg_it) {
      const tChartMappingRuleArg *arg = *arg_it;
      tItem *item = get_item(arg);
      int id = item->id();
      item_ids_os << arg->get_name() << ":" << lexical_cast<string>(id) << " ";
    }
    cerr << format("[cm] %s fired: %s\n") % get_rule()->get_printname()
        % item_ids_os.str();
    if (LOG_ENABLED(logChartMapping, DEBUG) || loglevel & 16) {
      args = get_rule()->get_args(tChartMappingRuleArg::OUTPUT_ARG);
      tItemPrinter ip(cerr, false, true);
      for (arg_it = args.begin(); arg_it != args.end(); ++arg_it) {
        ip.print(get_item(*arg_it)); cerr << endl;
      } // for
    } // if
  } // if

  return chart_changed;
}

fs
tChartMappingMatch::build_output_fs(const tChartMappingRuleArg *arg) {
  // since we want to replace the substitution varialbes with the regex
  // string capture, we need a deep copy here (otherwise problems with fs
  // structure sharing):
  fs out_fs = copy(get_arg_fs(arg));

  // replace regex substitution variables:
  std::list<list_int*> paths = out_fs.find_paths(BI_STRING);
  std::list<list_int*>::iterator path_it;
  for (path_it = paths.begin(); path_it != paths.end(); ++path_it) {
    list_int *path = *path_it;
    string str = type_name(out_fs.get_path_value(path).type());
    map<std::string, std::string>::iterator it;
    for (it = _captures.begin(); it != _captures.end(); ++it) {
      string variable = (*it).first;
      string replacement = (*it).second;
      int pos = str.find(variable);
      if (pos >= 0)
        str.replace(pos, variable.length(), replacement);
    }
    out_fs.get_path_value(path).set_type(retrieve_type(str));
    free_list(path);
  }
  assert(out_fs.valid());

  return out_fs;
}
