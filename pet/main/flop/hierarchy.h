/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* type hierarchy */

#ifndef _HIERARCHY_H_
#define _HIERARCHY_H_

#include <boost/graph/adjacency_list.hpp>

typedef boost::adjacency_list
<
  boost::setS, boost::vecS, boost::bidirectionalS
> tHierarchy;

typedef boost::graph_traits<tHierarchy>::vertex_descriptor tHierarchyVertex;
typedef boost::graph_traits<tHierarchy>::edge_descriptor tHierarchyEdge;

extern tHierarchy hierarchy;

#endif
