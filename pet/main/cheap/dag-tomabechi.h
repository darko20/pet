/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* typed dags a la tomabechi*/

#ifndef _DAG_H_
#define _DAG_H_

#define CONSTRAINT_CACHE

#include <list>
#include "list-int.h"

#define SMART_COPYING

#ifndef USEMMAP
#define MARK_PERMANENT
#endif

struct dag_node
{
  type_t type; type_t new_type;
 
  struct dag_arc *arcs;
  struct dag_arc *comp_arcs;
  struct dag_node *forward;
  struct dag_node *copy;

  int generation;

#ifdef MARK_PERMANENT
  bool permanent;
#endif
};

struct dag_arc
{
  int attr;
  struct dag_node *val;
  struct dag_arc *next;
};

#include "dag-alloc.h"
#include "dag-arced.h"

extern int unify_generation, unify_generation_max;

void dag_init(struct dag_node *dag, int s);

inline struct dag_node *dag_deref(struct dag_node *dag) { return dag; }
inline int dag_type(struct dag_node *dag) { return dag->type; }
inline void dag_set_type(struct dag_node *dag, int s) { dag->type = s; }

struct dag_arc *dag_find_attr(struct dag_arc *arc, int attr);
struct dag_node *dag_get_attr_value(struct dag_node *dag, int attr);

struct dag_node *dag_full_copy(struct dag_node *dag);
struct dag_node *dag_unify(struct dag_node *root, struct dag_node *dag1, struct dag_node *dag2, list_int *del);
bool dags_compatible(struct dag_node *dag1, struct dag_node *dag2);

void dag_subsumes(dag_node *dag1, dag_node *dag2, bool &forward, bool &backward);

struct dag_node *dag_partial_copy(dag_node *src, int attr);

// for debugging
bool dag_valid(dag_node *dag);

#include "failure.h"

list<class unification_failure *> dag_unify_get_failures(dag_node *dag1, dag_node *dag2, bool all_failures,
							 list_int *initial_path = 0, dag_node **result_root = 0);

list<list_int *> dag_paths(dag_node *dag, dag_node *search);

// non-permanent dags (for hyperactive parsing)

dag_node *dag_unify_np(dag_node *root, dag_node *dag1, dag_node *dag2);
void dag_get_qc_vector_np(struct qc_node *qc_paths, struct dag_node *dag, type_t *qc_vector);

struct dag_node *dag_nth_arg_np(struct dag_node *dag, int n);

void dag_print_safe(FILE *f, struct dag_node *dag, bool np);

// accessor functions for the `protected' slots -- inlined for efficiency

inline int dag_get_new_type(struct dag_node *dag)
{
  if(dag->generation == unify_generation) return dag->new_type; else return dag->type;
}

inline struct dag_arc *dag_get_comp_arcs(struct dag_node *dag)
{
  if(dag->generation == unify_generation) return dag->comp_arcs; else return 0;
}

inline struct dag_node *dag_get_forward(struct dag_node *dag)
{
  if(dag->generation == unify_generation) return dag->forward; else return dag;
}

inline struct dag_node *dag_get_copy(struct dag_node *dag)
{
  if(dag->generation == unify_generation) return dag->copy; else return 0;
}

inline void dag_set_new_type(struct dag_node *dag, int s)
{
  dag->new_type = s;
  if(dag->generation != unify_generation)
    {
      dag->generation = unify_generation;
      dag->comp_arcs = 0;
      dag->forward = dag;
      dag->copy = 0;
    }
}

inline void dag_set_comp_arcs(struct dag_node *dag, struct dag_arc *a)
{
  dag->comp_arcs = a;
  if(dag->generation != unify_generation)
    {
      dag->generation = unify_generation;
      dag->new_type = dag->type;
      dag->forward = dag;
      dag->copy = 0;
    }
}

inline void dag_set_forward(struct dag_node *dag, struct dag_node *c)
{
  dag->forward = c;
  if(dag->generation != unify_generation)
    {
      dag->generation = unify_generation;
      dag->comp_arcs = 0;
      dag->new_type = dag->type;
      dag->copy = 0;
    }
}

inline void dag_set_copy(struct dag_node *dag, struct dag_node *c)
{
  dag->copy = c;
  if(dag->generation != unify_generation)
    {
      dag->generation = unify_generation;
      dag->comp_arcs = 0;
      dag->forward = dag;
      dag->new_type = dag->type;
    }
}

inline void dag_invalidate_changes()
{
  if(unify_generation > unify_generation_max)
    unify_generation_max = unify_generation;

  unify_generation = ++unify_generation_max;
}

// we just `overload' the copy slot to make life for the printing stuff easier

inline int dag_get_visit(struct dag_node *dag)
{
  return (int) dag_get_copy(dag);
}

inline int dag_set_visit(struct dag_node *dag, int visit)
{
  dag->generation = unify_generation;
  return (int) (dag->copy = (struct dag_node *) visit);
}

inline void dag_invalidate_visited()
{
  dag_invalidate_changes();
}

class temporary_generation
{
 public:
  inline temporary_generation(int gen) : _save(unify_generation) 
    { unify_generation = gen; }
  inline ~temporary_generation()
    { unify_generation = _save; }

 private:
  int _save;
};

#endif
