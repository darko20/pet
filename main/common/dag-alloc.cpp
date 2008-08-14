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

/* memory management for dags */

#include "dag.h"
#include "dag-alloc.h"

int allocated_nodes = 0, allocated_arcs = 0;

long dag_alloc_dynamic_mem()
{
  return sizeof(dag_node) * allocated_nodes + sizeof(dag_arc) * allocated_arcs;
}

long dag_alloc_static_mem()
{
  return t_alloc.max_usage();
}

void dag_alloc_clear_stats()
{
  allocated_nodes = allocated_arcs = 0;
  t_alloc.reset_max_usage();
}

struct dag_node *dag_alloc_p_nodes(int n)
{
  allocated_nodes += n;
  return (dag_node *) p_alloc.allocate(sizeof(dag_node) * n);
}

struct dag_arc *dag_alloc_p_arcs(int n)
{
  allocated_arcs += n;
  return (dag_arc *) p_alloc.allocate(sizeof(dag_arc) * n);
}

void dag_alloc_may_shrink()
{
  t_alloc.may_shrink();
}

void dag_alloc_reset()
{
  t_alloc.reset();
}
