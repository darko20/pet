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

/**
 * \file chart-mapping.h
 * Nonmonotonic rewrite rules that operate on a chart of typed feature
 * structures.
 */

#ifndef _CHART_MAPPING_H_
#define _CHART_MAPPING_H_

#include "pet-config.h"

#include "fs.h"
#include "fs-chart.h"
#include "list-int.h"
#include "types.h"

#include <list>
#include <map>
#include <set>
#include <vector>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/regex.hpp>
#ifdef HAVE_BOOST_REGEX_ICU_HPP
#include <boost/regex/icu.hpp>
#endif


// forward declarations:
class tChartMappingEngine;
class tChartMappingRuleArg;
class tChartMappingRule;
class tChartMappingAnchoringGraph;
class tChartMappingMatch;


/**
 * A container mapping paths in feature structures to regular expression
 * objects.
 */
#ifdef HAVE_BOOST_REGEX_ICU_HPP
typedef std::map<list_int*, boost::u32regex> tPathRegexMap;
#else
typedef std::map<list_int*, boost::regex> tPathRegexMap;
#endif



/**
 * Class that executes chart mapping rules.
 */
class tChartMappingEngine
{
public:
  /**
   * Construct a new object, using the specified chart mapping rules.
   */
  tChartMappingEngine(const std::list<tChartMappingRule*> & rules,
      const std::string &phase_name = "chart mapping");

  /**
   * Apply the chart mapping rules supplied during construction to the
   * specified chart.
   *
   * \param[in,out] chart    the chart to operate on
   */
  void
  process(class tChart &chart, class Resources &resources);

private:

  /**
   * The chart mapping rules to be used in this instance of
   * tChartMappingEngine.
   */
  const std::list<tChartMappingRule*> & _rules;

  /**
   * A string for identifying this instance in logging output,
   * e.g. "token mapping".
   */
  const std::string _phase_name;

  /**
   * write to log if enabled
   * \return loglevel
   */
  int doLogging(tChart &chart, const char *);
};



/**
 * Representation of a chart mapping rule argument. This is an element
 * of the CONTEXT, INPUT or OUTPUT lists of the rule.
 */
class tChartMappingRuleArg
{

public:

  /**
   * The different kinds of arguments.
   */
  enum Trait { CONTEXT_ARG, INPUT_ARG, OUTPUT_ARG };

  /**
   * Destructor.
   */
  virtual
  ~tChartMappingRuleArg();

  /**
   * Factory method that returns a new tChartMappingRuleArg object.
   * Enforces the creation of these object on the heap. The caller
   * (tChartMappingRule) is responsible for deleting the object.
   */
  static tChartMappingRuleArg*
  create(const std::string &name, tChartMappingRuleArg::Trait trait, int nr,
      tPathRegexMap &regexs);

  /**
   * The name of this rule argument. Argument's names are used to
   * refer to the relative position to other arguments and to refer
   * to regex captures.
   */
  const std::string&
  get_name() const;

  /**
   * Returns the start anchor of this argument, i.e. its name + ":s".
   */
  const std::string&
  get_start_anchor() const;

  /**
   * Returns the end anchor of this argument, i.e. its name + ":e".
   */
  const std::string&
  get_end_anchor() const;

  /**
   * Returns the trait of this chart mapping rule argument.
   * \see tChartMappingRuleArg::Trait
   */
  tChartMappingRuleArg::Trait
  get_trait() const;

  /**
   * Returns the number of this item within its list (see get_trait()).
   */
  int
  get_nr() const;

  /**
   * Returns a container mapping paths in the feature structure of this
   * argument to regular expression objects.
   */
  const tPathRegexMap&
  get_regexs() const;

private:

  /**
   * Constructor.
   * \see create()
   */
  tChartMappingRuleArg(const std::string &name,
      tChartMappingRuleArg::Trait trait, int nr, tPathRegexMap &regexs);

  /** No default copy constructor. */
  tChartMappingRuleArg(const tChartMappingRuleArg& arg);

  /** \see get_name() */
  std::string _name;

  /** \see get_start_anchor() */
  std::string _start_anchor;

  /** \see get_end_anchor() */
  std::string _end_anchor;

  /** \see get_trait() */
  tChartMappingRuleArg::Trait _trait;

  /** \see get_nr() */
  int _nr;

  /** \see get_regexs() */
  tPathRegexMap _regexs;

};



/**
 * A collection of chart mapping rule arguments.
 */
typedef std::vector<const tChartMappingRuleArg *> tRuleArgs;



/**
 * An anchoring graph describes positional dependencies of the arguments of a
 * chart-mapping rule in the chart. It can be thought of as a regular graph
 * pattern that a chart has to fit in when the chart-mapping rule fires.
 *
 * Vertices in the anchoring graph bidirectionally correspond to vertices in the
 * chart (but not every vertex in the chart corresponds to a vertex in the
 * anchoring graph). Edges correspond to items or paths of items in the chart.
 *
 * The start and end vertices of an argument (both in the anchoring graph and in
 * the chart) can be referred to by `anchors'. The start and end anchors of an
 * argument are named by the argument's name + ":s" or ":e", respectively.
 * Anchors are equivalence classes of vertices, so anchors of different
 * arguments may refer to the same vertex. There are two special anchors,
 * "^" and "$", which refer to the chart start and chart end during matching
 * (the chart start and end can be altered by a chart-mapping rule).
 *
 * Edges are annotated with the minimal and maximal distance between their two
 * vertices. That is, if there is an edge between \a v1 and \a v2 with
 * \a min == \a a and \a max == \a b, written as (\a a, \a b), then there exists
 * a path of minimally \a a and maximally \a b items between \a v1 and \a v2.
 * Edges are directed and thus encode the linear order between vertices.
 * Multiple paths of different distances between two vertices are allowed and
 * are encoded by multiple edges.
 *
 * There are three special edge types besides normal path edges: edges for
 * matching arguments, edges for output arguments, and tentative egdes. Edges
 * for matching arguments and output arguments are always annotated with (1,1).
 * Edges for output arguments are treated a bit differently than path edges and
 * matching argument edges since they describe a situation after all matching
 * arguments have been matched.
 *
 * Tentative edges are always annotated with (0,0). They signal a potential
 * vertex identity. Tentative edges are needed for realizing parallels-relations
 * between rule arguments. The vertices of those tentative edges that lead to a
 * maximal span of an argument will be merged by realize_tentative_edges(),
 * other tentative edges will be removed.
 */
class tChartMappingAnchoringGraph {

  friend class tChartMappingMatch; // TODO public interface would be better

public:

  /**
   * The only internal vertex property, the vertex index. Some algorithms
   * require the use of vertex indices, which are only available
   * in std::vector-based vertex containers. Since we want to have a
   * std::list-based container, we have to declare the vertex indices
   * explicitely.
   * \see index()
   */
  typedef boost::property<boost::vertex_index_t, std::size_t> tVertexProp;

  /**
   * An edge indicates an item path in the chart of the specified minimal and
   * maximal length between the two anchors.
   */
  struct tEdgeProp {
    int min;
    int max;
    enum tType { PATH, MATCHING, OUTPUT, TENTATIVE } type;
    tEdgeProp(int mi, int ma, tType t) : min(mi), max(ma), type(t)
    { }
  };

  // Only descriptors in listS or setS remain stable under all conditions!
  typedef boost::adjacency_list<boost::multisetS, boost::listS,
      boost::bidirectionalS, tVertexProp, tEdgeProp> tGraph;
  typedef boost::graph_traits<tGraph>::vertex_descriptor tVertex;
  typedef boost::graph_traits<tGraph>::vertex_iterator tVertexIt;
  typedef boost::graph_traits<tGraph>::edge_descriptor tEdge;
  typedef boost::graph_traits<tGraph>::edge_iterator tEdgeIt;
  typedef boost::graph_traits<tGraph>::in_edge_iterator tInEdgeIt;
  typedef boost::graph_traits<tGraph>::out_edge_iterator tOutEdgeIt;

  typedef std::vector<tVertex> tVertices;
  typedef tVertices::iterator tVerticesIt;
  typedef tVertices::reverse_iterator tVerticesRevIt;

  typedef std::list<std::string> tStrings;
  typedef tStrings::iterator tStringsIt;
  typedef std::map<std::string, tVertex> tStringVertexMap;
  typedef std::map<tVertex, tStrings > tVertexStringsMap;


  /**
   * Construct an anchoring graph, initialising the chart start and end
   * vertices.
   */
  tChartMappingAnchoringGraph();

  /**
   * Adds the argument to this anchoring graph. Matching arguments are
   * connected to the chart start and end.
   */
  void
  add_arg(const tChartMappingRuleArg *arg);

  /**
   * Merges the vertices identified by anchor \c a1 and \c a2.
   * \see merge_vertices(tVertex, tVertex)
   */
  void
  merge_vertices(std::string a1, std::string a2);

  /**
   * Ensures that there is an edge with the specified min and max values
   * between the vertices identified by the two anchors.
   * \see accomodate_edge(tVertex, tVertex, int, int, tEdgeProp::tType)
   */
  void
  accomodate_edge(std::string a1, std::string a2, int min, int max,
      tEdgeProp::tType t = tEdgeProp::PATH);

  /**
   * This function should be called after all arguments have been added
   * to the graph and all constraints have been accomodated. The function
   * realizes tenative edges, computes the transitive closure and finally
   * checks the consistency of the graph.
   */
  void
  finalize();

  /**
   * Returns the anchoring graph vertex for the specified anchor.
   */
  const tVertex
  get_vertex(std::string anchor) const;

  /**
   * Creates a debug print of the anchoring graph on the standard error stream.
   */
  void
  print() const;

private:

  /**
   * Copy constructor is forbidden.
   */
  tChartMappingAnchoringGraph(const tChartMappingAnchoringGraph&);

  /**
   * Adds a vertex for the specified anchor.
   */
  tVertex
  add_vertex(std::string a);

  /**
   * Merge vertex \c v2 into \c v1, bending all in-edges of \c v2 so that they
   * end in \c v1, and all out-edges of \c v2 so that they start in \c v1, and
   * updating the anchor references.
   */
  void
  merge_vertices(tVertex v1, tVertex v2);

  /**
   * Ensures that there is an edge with the specified min and max values
   * between the two vertices, if it doesn't contradict the current graph.
   * If an edge already exists and the new min and max values restrict the
   * values of the edge or vice versa, refines the min and max values of
   * that edge. Adds a new edge otherwise.
   * \throw tError if an edge shall be added that contradicts the current graph,
   *               i.e. where there is already a path in the opposite direction
   */
  void
  accomodate_edge(tVertex v1, tVertex v2, int min, int max,
      tEdgeProp::tType t = tEdgeProp::PATH);

  /**
   * (Re-) Indexes the graph. Some algorithms such as BGL's depth-first search
   * and topological sort require that an index between [0, number_vertices) is
   * provided for each vertex. The index property for each vertex is not
   * kept up-to-date when altering the vertex set, so make sure to call this
   * method before using one of these algorithms.
   */
  void
  index();

  /**
   * Anchors tentative vertex-identity edges resulting from parallels-relations.
   */
  void
  realize_tentative_edges(tVertices &v_order);

  /**
   * Computes the transitive closure of this graph, by either adding edges
   * or updating the weights of existing edges.
   */
  void
  transitive_closure(tVertices &v_order);

  /**
   * Checks whether the graph is acyclic and connected. Returns \c true if
   * the graph is consistent, otherwise an error is thrown.
   */
  bool
  check_consistency();

  /**
   * Gets a string representation of the specified vertex \a v.
   */
  std::string
  get_anchors_string(tVertex v) const;

  /** The internal graph representation. */
  tGraph _graph;

  /** Chart start vertex. */
  tVertex _vs;

  /** Chart end vertex. */
  tVertex _ve;

  /** A dictionary mapping anchors to vertices. */
  tStringVertexMap _vertices;

  /** A dictionary mapping vertices to anchors. */
  tVertexStringsMap _anchors;

  /** All argument names. */
  tStrings _arg_names;

};



/**
 * Representation of a chart mapping rule of the grammar.
 */
class tChartMappingRule
{

public:

  /**
   * Chart mapping rules come in two traits: token mapping rules (TMR) or
   * lexical filtering rules (LFR). TMRs operate on tokens, i.e. input
   * items; LFRs operate on lexical items.
   */
  enum Trait { TMR_TRAIT, LFR_TRAIT };

  /**
   * Factory method that returns a new tChartMappingRule object for the
   * specified type. If the feature structure of the given type is not
   * a valid chart mapping rule, 0 is returned.
   * \param[in] type
   * \param[in] trait
   * \return a tChartMappingRule for type \a type or 0
   */
  static tChartMappingRule*
  create(type_t type, tChartMappingRule::Trait trait);

  /**
   * Gets the type of this rule instance.
   */
  type_t
  get_type() const;

  /**
   * Gets the trait of this rule, namely whether it is a token mapping rule
   * (TMR) or a lexical filtering rule (LFR).
   */
  tChartMappingRule::Trait
  get_trait() const;

  /**
   * Gets the print name of this rule.
   * \see print_name(type_t)
   */
  const char*
  get_printname() const;

  /**
   * Returns a feature structure associated with this rule.
   */
  fs
  instantiate() const;

  /**
   * Returns an argument by its name or 0 if it is not found.
   */
  const tChartMappingRuleArg*
  get_arg(std::string name) const;

  /**
   * Returns all arguments of this rule (i.e., all CONTEXT, INPUT and
   * OUTPUT items).
   */
  const tRuleArgs &
  get_args() const;

  /**
   * Returns all arguments of this rule with the specified trait.
   */
  const tRuleArgs
  get_args(tChartMappingRuleArg::Trait trait) const;

  /**
   * Returns all matching arguments of this rule, in the order as they
   * should be matched.
   */
  const tRuleArgs &
  get_matching_args() const;

  /**
   * Returns all OUTPUT arguments of this rule, in the order as they
   * should be instantiated.
   * This rule manages the memory for these items.
   */
  const tRuleArgs &
  get_output_args() const;

  /**
   * Returns the anchoring graph.
   */
  const tChartMappingAnchoringGraph &
  get_anchoring_graph() const;

private:

  /**
   * Constructs a new tChartMappingRule object for the specified \c trait.
   * \param[in] type the type code of this rule in the type hierarchy
   * \param[in] trait determines in which phase this rule will be applied
   * \see create()
   */
  tChartMappingRule(type_t type, tChartMappingRule::Trait trait);

  /**
   * Evaluates the positional constraints specification.
   */
  void
  evaluate_poscons(std::string poscons_s);

  /** Identifier of the rule's type in PET's type system. */
  type_t _type;

  /** Signals which kind of items we're operating on. */
  tChartMappingRule::Trait _trait;

  /**
   * Feature structure representation of this rule. This fs is not
   * necessarily equal to the type's fs since we replace string literals
   * containing regular expressions with the general string type in order
   * to allow for successful unification.
   */
  fs _fs;

  /**
   * All arguments of this rule (CONTEXT, INPUT, OUTPUT).
   * This rule manages the memory for these items.
   */
  tRuleArgs _args;

  /**
   * All matching arguments of this rule, in the order as they
   * should be matched.
   * This rule manages the memory for these items.
   */
  tRuleArgs _matching_args;

  /**
   * All OUTPUT arguments of this rule, in the order as they
   * should be instantiated.
   * This rule manages the memory for these items.
   */
  tRuleArgs _output_args;

  /**
   * The anchoring graph, describing the positional constraints on the
   * rule arguments.
   */
  tChartMappingAnchoringGraph _anch_graph;

};



/**
 * A partially or fully matched instantiation of a tChartMappingRule .
 * It is similar to an active chart item in syntactic parsing in that it gives
 * access to all the arguments matched so far and lists the arguments to be
 * matched next. Unlike an active chart item in syntactic parsing it does not
 * represent a structure that can be embedded in other structures, i.e. it is
 * not an item. Once created, objects of this class cannot change anymore but
 * they can give rise to new tChartMappingMatch objects by matching the next
 * argument of the rule.
 */
class tChartMappingMatch
{

public:

  /**
   * Create a (completely unmatched) tChartMappingMatch instance for the
   * specified \a rule.
   * The returned object is owned by the caller of this method.
   */
  static tChartMappingMatch*
  create(const tChartMappingRule *rule, const tChart &chart);

  /** Is this rule match complete? */
  bool
  is_complete() const;

  /** Returns the rule for this rule match. */
  const tChartMappingRule*
  get_rule() const;

  /**
   * Returns the feature structure representation of this match.
   */
  fs
  get_fs();

  /**
   * Returns the feature structure for the specified argument within the
   * feature structure representation of this match.
   */
  fs
  get_arg_fs(const tChartMappingRuleArg *arg);

  /**
   * Returns the next argument that should be matched or \c 0 if the there
   * is no such argument.
   */
  const tChartMappingRuleArg*
  get_next_matching_arg() const;

  /**
   * Returns the item bound to the specified argument \c arg.
   */
  tItem*
  get_item(const tChartMappingRuleArg* arg);

  /**
   * Returns all items bound to the arguments of this rule.
   */
  item_list
  get_items();

  /**
   * Returns all items bound to the arguments with the specified \c trait.
   */
  item_list
  get_items(tChartMappingRuleArg::Trait trait);

  /**
   * Find all items from the chart that could serve as an item for the next
   * match by evaluating the positional constraints of the next rule argument
   * \a arg and this match.
   */
  item_list
  get_suitable_items(tChart &chart, const tChartMappingRuleArg *arg);

  /**
   * Tries to match the specified item \a item with the argument \a arg of
   * this match. Returns \c 0 if such a match is not possible.
   * This function uses a regex-enabled unification. Matched (sub-)strings
   * are added to the \c captures map, using
   *     "${"
   *   + \c get_next_matching_arg().name()
   *   + ":"
   *   + the path whose value has been matched
   *   + ":"
   *   + the number of the matched substring
   *   + "}"
   * as the key. For example, if the argument's name
   * is "I2" and contains a regular expression "/(bar)rks/" in path "FORM",
   * then the matched substring "bar" is stored under key "${I2:FORM:1}".
   */
  tChartMappingMatch*
  match(tItem *item, const tChartMappingRuleArg *arg, int loglevel);

  /**
   * Applies the rule of this completed match to the chart, that is
   * INPUT items are deleted and OUTPUT items are created. Created OUTPUT
   * items can be retrieved by get_item() and get_items() once this method
   * has been called.
   * \pre this match is completed
   * \return \c true iff the chart has changed by applying this rule
   */
  bool
  fire(tChart &chart, int loglevel);

private:

  /** Map that maps arguments to items. */
  typedef std::map<const tChartMappingRuleArg*, tItem*> tArgItemMap;

  /** Map that maps anchoring graph vertices to chart vertices. */
  typedef std::map<tChartMappingAnchoringGraph::tVertex, tChartVertex*>
    tAnchoringToChartVertexMap;

  /**
   * Constructor.
   * \see create()
   * \see retrieve()
   */
  tChartMappingMatch(const tChartMappingRule *rule,
      const tChartMappingMatch *previous,
      const fs &f,
      const tArgItemMap &arg_item_map,
      const std::map<std::string, std::string> &captures,
      const tAnchoringToChartVertexMap &vertex_binding,
      unsigned int next_arg_idx);

  /**
   * Create a (partially or fully matched) tChartMappingMatch instance for
   * the specified parameters.
   * The returned object is owned by the caller of this method.
   * \see match()
   */
  tChartMappingMatch*
  create(const fs &f,
      const tArgItemMap &arg_item_map,
      const std::map<std::string, std::string> &captures,
      const tAnchoringToChartVertexMap &vertex_binding,
      unsigned int next_arg_idx);

  /**
   * Return the OUTPUT fs for the specified OUTPUT argument \c arg,
   * replacing all substitution variables in the strings of the fs.
   * TODO if we replace substitution variables when constructing a
   *      tChartMappingMatch, then this method becomes useless.
   */
  fs
  build_output_fs(const tChartMappingRuleArg *arg);

  /** The rule which is used for building this rule match. */
  const tChartMappingRule *_rule;

  /** The anchoring graph of the rule. */
  const tChartMappingAnchoringGraph &_anch_graph;

  /** The previous match (0 if this is an empty match). */
  const tChartMappingMatch * const _previous;

  /** The current instantiation of the rule match. */
  fs _fs;

  /**
   * A map mapping arguments to corresponding items in the chart. With
   * every new match retrieved by match(), this list map increases by
   * the last matched argument/item pair. This list also contains the
   * OUTPUT argument/item pairs for completed matches, provided that
   * the rule has already fired (\see fire()).
   */
  tArgItemMap _arg_item_map;

  /** The string captures registered so far. */
  std::map<std::string, std::string> _captures;

  /**
   * Map that maps anchoring graph vertices to chart vertices.
   */
  tAnchoringToChartVertexMap _vertex_binding;

  /** Index (in the rule's _matching_args list) of the next argument to be
   * matched. */
  unsigned int _next_arg_idx;

};

#endif /*_CHART_MAPPING_H_*/
