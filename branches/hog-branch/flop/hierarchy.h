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

/** typedef for boost graph implementation */
typedef boost::adjacency_list
<
  boost::vecS, boost::vecS, boost::bidirectionalS
> tHierarchy;

/** typedef for boost vertex type */
typedef boost::graph_traits<tHierarchy>::vertex_descriptor tHierarchyVertex;
/** typedef for boost edge type */
typedef boost::graph_traits<tHierarchy>::edge_descriptor tHierarchyEdge;

/** The global type hierarchy */
extern tHierarchy hierarchy;

/** Check and complete the hierarchy to get a BCPO
 *  \param propagate_status if \c true, propagate the status values through the
 *            hierarchy as the last step
 */
bool process_hierarchy(bool propagate_status);

/** Add type with id \a s to the hierarchy */
void register_type(int s);
/** Establish an immediate subtype relation between \a sub and \a super. */
void subtype_constraint(int sub, int super);
/** Remove all sub- and supertype relations of type \a t. */
void undo_subtype_constraints(int t);

/** Return the immediate subtypes of \a t */
std::list<int> immediate_subtypes(int t);
/** Return the immediate supertypes of \a t */
std::list<int> immediate_supertypes(int t);

/** Print the hierarchy readably for debugging purposes */
void print_hierarchy(FILE *f);
/*@}*/

#endif
