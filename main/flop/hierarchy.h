/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/** \file hierarchy.h
 * type hierarchy, encoded in a boost directed graph
 */

#ifndef _HIERARCHY_H_
#define _HIERARCHY_H_

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/topological_sort.hpp>

/** typedef for boost graph implementation */
typedef boost::adjacency_list
<
  boost::setS, boost::vecS, boost::bidirectionalS
> tHierarchy;

/** typedef for boost vertex type */
typedef boost::graph_traits<tHierarchy>::vertex_descriptor tHierarchyVertex;
/** typedef for boost edge type */
typedef boost::graph_traits<tHierarchy>::edge_descriptor tHierarchyEdge;

/** The global type hierarchy */
extern tHierarchy hierarchy;

#endif
