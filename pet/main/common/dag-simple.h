/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* simple typed dags */

#ifndef _DAG_H_
#define _DAG_H_

#include "list-int.h"
#include "dumper.h"

struct dag_node
{
  struct dag_node *forward;

  int type;
  struct dag_arc *arcs;

  struct dag_node *copy; int copy_generation;

#ifdef FLOP
  int visit; int visit_generation;
#endif
};

struct dag_arc
{
  int attr;
  struct dag_node *val;
  struct dag_arc *next;
};

#include "dag-alloc.h"

extern int copy_generation;
extern int copy_wf_generation;

inline struct dag_node *dag_get_copy(struct dag_node *dag, int generation)
{
  if(generation != dag->copy_generation)
    return 0;
  else
    return dag->copy;
}

inline struct dag_node *dag_set_copy(struct dag_node *dag, struct dag_node *copy, int generation)
{
  dag->copy_generation = generation;
  return dag->copy = copy;
}

inline void dag_invalidate_copy()
{
  if(copy_wf_generation > copy_generation)
    copy_generation = copy_wf_generation + 1;
  else
    copy_generation ++;

  copy_wf_generation = copy_generation;
}

#ifdef FLOP // only the expander needs a seperate visited field

extern int visit_generation;

inline int dag_get_visit(struct dag_node *dag)
{
  if(visit_generation != dag->visit_generation)
    return 0;
  else
    return dag->visit;
}

inline int dag_set_visit(struct dag_node *dag, int visit)
{
  dag->visit_generation = visit_generation;
  return dag->visit=visit;
}

inline void dag_invalidate_visited()
{
  visit_generation++;
}

#else

inline int dag_get_visit(struct dag_node *dag)
{
  if(copy_generation != dag->copy_generation)
    return 0;
  else
    return (int) dag->copy;
}

inline int dag_set_visit(struct dag_node *dag, int visit)
{
  dag->copy_generation = copy_generation;
  return (int) (dag->copy = (struct dag_node *) visit);
}

inline void dag_invalidate_visited()
{
  if(copy_wf_generation > copy_generation)
    copy_generation = copy_wf_generation + 1;
  else
    copy_generation ++;

  copy_wf_generation = copy_generation;
}

#endif

inline struct dag_node *dag_deref(struct dag_node *dag)
{
  while(dag->forward != dag)
    dag = dag->forward;

  return dag;
}

#ifdef FLOP
extern bool unify_reset_visited;
#endif

struct dag_node *make_nth_arg(int n, dag_node *v);

struct dag_node *dag_copy(struct dag_node *src);

struct dag_node *dag_unify1(struct dag_node *dag1, struct dag_node *dag2);
struct dag_node *dag_unify2(struct dag_node *dag1, struct dag_node *dag2);
struct dag_node *dag_unify3(struct dag_node *dag1, struct dag_node *dag2);

bool dags_compatible(struct dag_node *dag1, struct dag_node *dag2);
bool dag_cyclic(struct dag_node *dag);

void dag_init(struct dag_node *dag, int s);
struct dag_node *new_dag(int s);
struct dag_arc *new_arc(int attr, struct dag_node *val);

void add_arc(struct dag_node *dag, struct dag_arc *newarc);

struct dag_arc *dag_find_attr(struct dag_arc *arc, int attr);

bool dag_dump(dumper *f, struct dag_node *dag);

#endif
