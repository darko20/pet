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

/* memory allocation for dags */

#ifndef _DAG_ALLOC_H_
#define _DAG_ALLOC_H_

#include "dag.h"
#include "chunk-alloc.h"

struct dag_alloc_state
{
  struct chunk_alloc_state state;
};

long dag_alloc_dynamic_mem();
long dag_alloc_static_mem();
void dag_alloc_clear_stats();

void dag_alloc_may_shrink();
void dag_alloc_reset();

extern int allocated_nodes, allocated_arcs;

//
// temporary dag storage
//

inline struct dag_node *dag_alloc_node()
{
  allocated_nodes++;
  return (dag_node *) t_alloc.allocate(sizeof(dag_node));
}

inline struct dag_arc *dag_alloc_arc()
{
  allocated_arcs++;
  return (dag_arc *) t_alloc.allocate(sizeof(dag_arc));
}

inline void dag_alloc_mark(struct dag_alloc_state &s)
{
  t_alloc.mark(s.state);
}

inline void dag_alloc_release(struct dag_alloc_state &s)
{
  t_alloc.release(s.state);
}

//
// permanent dag storage
//

inline struct dag_node *dag_alloc_p_node()
{
  allocated_nodes++;
  return (dag_node *) p_alloc.allocate(sizeof(dag_node));
}

inline struct dag_arc *dag_alloc_p_arc()
{
  allocated_arcs++;
  return (dag_arc *) p_alloc.allocate(sizeof(dag_arc));
}

struct dag_node *dag_alloc_p_nodes(int n);
struct dag_arc *dag_alloc_p_arcs(int n);

#endif
