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

/** \file dag-simple.h
 * Simple typed dags.
 */

#ifndef _DAG_H_
#define _DAG_H_

/** Simple typed dags: the dag nodes */
struct dag_node
{
  /** The forward pointer to relate unified nodes during unification. This is
   *  like a union-find data structure.
   */
  struct dag_node *forward;

  /** The type ID of this node */
  int type;
  /** The outgoing arcs list (single linked) of this node, maybe \c NULL */
  struct dag_arc *arcs;
  
  /** @name Copy Slot
   * The copy slot, protected by a generation counter
   */
  /*@{*/
  struct dag_node *copy; int copy_generation;
  /*@}*/

#ifdef FLOP
  /** @name Visit Slot
   * A slot to mark visits throughout traversal, protected by a generation
   * counter.
   */
  /*@{*/
  int visit; int visit_generation;
  /*@}*/
#endif
};

/** Simple typed dags: the dag arcs */
struct dag_arc
{
  /** The attribute ID of this arc */
  int attr;
  /** The dag node pointed to by this arc */
  struct dag_node *val;
  /** The next arc in this arc list, maybe \c NULL */
  struct dag_arc *next;
};

#include "dag-alloc.h"

/** The generation counter for ordinary copies */
extern int copy_generation;
/** The generation counter used during well-formedness unifications */
extern int copy_wf_generation;

/** Get the copy slot of \a dag, checking the \a generation. 
 * \return NULL, if \a generation is not the current generation, the value of
 *         the copy slot otherwise.
 */
inline dag_node *dag_get_copy(dag_node *dag, int generation)
{
  if(generation != dag->copy_generation)
    return 0;
  else
    return dag->copy;
}

/** Set the copy slot of \a dag to \a copy.
 *  The generation counter of the slot is set to \a generation, thereby
 *  validating the slot.
 *  \return the \a copy node.
 */
inline dag_node *dag_set_copy(dag_node *dag, dag_node *copy, int generation)
{
  dag->copy_generation = generation;
  return dag->copy = copy;
}

/** Increase the global generation counters to invalidate all copy slots. */
inline void dag_invalidate_copy()
{
  if(copy_wf_generation > copy_generation)
    copy_generation = copy_wf_generation + 1;
  else
    copy_generation ++;

  copy_wf_generation = copy_generation;
}

#ifdef FLOP

/** @name Visit Slot
 * A seperate visited field generation counter that is necessary only for type
 * expansion.
 * The functionality is the same as with the copy generation counter.
 */
/*@{*/
extern int visit_generation;

inline int dag_get_visit(dag_node *dag)
{
  if(visit_generation != dag->visit_generation)
    return 0;
  else
    return dag->visit;
}

inline int dag_set_visit(dag_node *dag, int visit)
{
  dag->visit_generation = visit_generation;
  return dag->visit=visit;
}

inline void dag_invalidate_visited()
{
  visit_generation++;
}
/*@}*/

#else

/** @name Visit Slot
 * `overload' the copy slot to obtain the desired functionality for traversal 
 */
/*@{*/
inline int dag_get_visit(dag_node *dag)
{
  if(copy_generation != dag->copy_generation)
    return 0;
  else
    return (int) dag->copy;
}

inline int dag_set_visit(dag_node *dag, int visit)
{
  dag->copy_generation = copy_generation;
  return (int) (dag->copy = (dag_node *) visit);
}

inline void dag_invalidate_visited()
{
  if(copy_wf_generation > copy_generation)
    copy_generation = copy_wf_generation + 1;
  else
    copy_generation ++;

  copy_wf_generation = copy_generation;
}
/*@}*/

#endif

/** Follow the forward links to the node that represents this dag at the
 *  moment.
 */ 
inline dag_node *dag_deref(dag_node *dag)
{
  while(dag->forward != dag)
    dag = dag->forward;

  return dag;
}

/** Return \c true if \a dag has no arcs */
inline bool dag_framed(dag_node *dag)
{
  if(dag == 0) return false;
  dag = dag_deref(dag);
  return dag->arcs != 0;
}

#ifdef FLOP
/** special handling of the dag visit flag in type expansion */
extern bool unify_reset_visited;
#endif

/** make \a v the \a n th argument of a new dag containing only an incomplete
 *  list and return this dag.
 */
dag_node *make_nth_arg(int n, dag_node *v);

/** Clone \a src and return the copy */
dag_node *dag_copy(dag_node *src);

/** @name Wroblewski Unification 
   An implementation of unify1 and unify2 of Wroblewski(1987) with
   changes for welltyped unification.
   Unify2 is buggy in the paper (for certain cases of convergent arcs),
   fixing it requires a third, directed mode of unification where only
   one of the input dags is modified. This resembles `into' unification
   a la Ciortuz(2000). This directed mode has to fall back to
   destructive unification for nodes copied previously within the same
   unification context - otherwise we'd have to dereference the copy slot
   (since we could get chains of copies) */
/*@{*/
/** Destructive unification. */
dag_node *dag_unify1(dag_node *dag1, dag_node *dag2);
/** Non-destructive unification. */
dag_node *dag_unify2(dag_node *dag1, dag_node *dag2);
/** Semi-destructive unification */
dag_node *dag_unify3(dag_node *dag1, dag_node *dag2);
/*@}*/

/** Return \c true if \a dag contains a cycle */
bool dag_cyclic(dag_node *dag);

/** Dump \a dag onto \a f (external binary representation) */
bool dag_dump(dumper *f, dag_node *dag);

#endif
