/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* memory management for dags */

#include "dag.h"

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
