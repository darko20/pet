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

/** \file dag-alloc.h
 * memory allocation for dags
 */

#ifndef _DAG_ALLOC_H_
#define _DAG_ALLOC_H_

#include "dag.h"
#include "chunk-alloc.h"

/** An allocation marker to do stack based allocation/deallocation of whole
 *  memory areas.
 *  \see chunk_alloc_state
 */
struct dag_alloc_state
{
  struct chunk_alloc_state state;
};

/** @name Memory usage statistics
 * These functions are used to estimate PET's memory usage.
 */
/*@{*/
/**
 * Reports how much memory (in bytes) is required for storing all permanent
 * or temporary dags that have been created so far (as if memory was not
 * reused). The value can be reset by dag_alloc_clear_stats().
 * \todo is documentation this correct?? 
 */
long dag_alloc_dynamic_mem();
/**
 * Reports how much memory (in bytes) is currently allocated for storing
 * temporary dags. The value can be reset by dag_alloc_clear_stats(), which
 * has only a visible effect if allocated memory may shrink.
 * \todo is documentation this correct?? 
 */
long dag_alloc_static_mem();
/** Reset the statistics of static and dynamic memory usage */
void dag_alloc_clear_stats();
/*@}*/

/** Tell the allocator to release memory if it is convenient */
void dag_alloc_may_shrink();
/** Release all memory allocated for dags */
void dag_alloc_reset();

/** Statistics about allocated nodes */
extern int allocated_nodes;
/** Statistics about allocated arcs */
extern int allocated_arcs;

/** @name Temporary dag storage
 * This is storage that is allocated for dags during parsing. At the beginning
 * of a series of unifications, an allocation mark is acquired. Then, a number
 * of dynamically allocated dag arcs and nodes is created during
 * unification. If unification fails at some point, the memory up to the last
 * mark is released. Otherwise, copying the feature structure requests more
 * memory.
 */
/*@{*/
/** Allocate a dag node in temporary storage */
inline struct dag_node *dag_alloc_node()
{
  allocated_nodes++;
  return (dag_node *) t_alloc.allocate(sizeof(dag_node));
}

/** Allocate a dag arc in temporary storage */
inline struct dag_arc *dag_alloc_arc()
{
  allocated_arcs++;
  return (dag_arc *) t_alloc.allocate(sizeof(dag_arc));
}

/** Acquire an allocation marker in \a s to be able to release a bunch of
 *  useless memory in one step.
 */
inline void dag_alloc_mark(struct dag_alloc_state &s)
{
  t_alloc.mark(s.state);
}

/** Release all memory up the mark given by \a s */
inline void dag_alloc_release(struct dag_alloc_state &s)
{
  t_alloc.release(s.state);
}
/*@}*/

/** @name Permanent dag storage
 * This is memory allocated during initialization of the grammar. The dags in
 * this memory belong to the data base. They will only be released when the 
 * grammar as a whole is released, which is typically at the end of the process
 */
/*@{*/
/** Allocate a dag node in permanent storage */
inline struct dag_node *dag_alloc_p_node()
{
  allocated_nodes++;
  return (dag_node *) p_alloc.allocate(sizeof(dag_node));
}

/** Allocate a dag arc in permanent storage */
inline struct dag_arc *dag_alloc_p_arc()
{
  allocated_arcs++;
  return (dag_arc *) p_alloc.allocate(sizeof(dag_arc));
}

/** Allocate \a n dag nodes in permanent storage */
struct dag_node *dag_alloc_p_nodes(int n);
/** Allocate \a n dag arcs in permanent storage */
struct dag_arc *dag_alloc_p_arcs(int n);
/*@}*/

#endif
