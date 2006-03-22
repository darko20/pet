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

/** \file dag-tomabechi.h
 * Typed dags a la tomabechi, with partial/smart copying
 */

#ifndef _DAG_H_
#define _DAG_H_

/** Enable cache for typedags that are used during unification */
#define CONSTRAINT_CACHE

#include "pet-config.h"

#include <list>
#include "types.h"

/** Use partial copying if possible if \c SMART_COPYING is defined */
#define SMART_COPYING

#ifndef HAVE_MMAP
/** If \c MARK_PERMANENT is defined, permanent dags are marked with a boolean
 *  struct member.
 */
#define MARK_PERMANENT
#endif

#ifdef MARK_PERMANENT
/** When this flag is true, permanent dags are created. This occures almost
 *  only during grammar read/creation and copying of typedags for welltypedness
 *  unifications.
 */
extern bool create_permanent_dags;
#endif

/** Dag node representation for Tomabechi style unifier */
struct dag_node
{
  /** The type of the dag, except if the current generation is equal to
   *  dag_node::generation.
   */
  type_t type;
  /** The type of the dag, if the current generation is equal to
   *  dag_node::generation (a generation protected member).
   */
  type_t new_type;

  /** The arcs list of the node */
  struct dag_arc *arcs;
  /** The new or modified arcs during unification */
  struct dag_arc *comp_arcs;
  /** During unification, this points to the representative of this dag, if it
   *  is non-\c NULL (a generation protected member).
   */
  struct dag_node *forward;
  /** If non-\c NULL, this member contains a copy of the current dag node (a
   *  generation protected member).
   */
  struct dag_node *copy;

  /** \brief The generation of this dag node. Affects the members
   *  dag_node::new_type, dag_node::comp_arcs, dag_node::forward and
   *  dag_node::copy 
   */
  int generation;

#ifdef MARK_PERMANENT
  /** If true, this is a permanent dag, which means that is has to be copied,
   *  even though it might look as if it could be shared by smart copying.
   */
  bool permanent;
#endif
};

/** dag arc structure, a single linked list of arcs. */
struct dag_arc
{
  /** The attribute id */
  attr_t attr;
  /** The node this arc points to */
  struct dag_node *val;
  /** The next element of the arc list */
  struct dag_arc *next;
};

#include "dag-arced.h"

/** @name Generation Counters
 *  Two global generation counters, one does not suffice because of the
 *  welltypedness unifications with type dags, and multiple generations may be
 *  necessary to perform the (top level) unification.
 */
/*@{*/
extern int unify_generation;
extern int unify_generation_max;
/*@}*/

/** Initialize the constraint cache.
 * The constraint cache keeps type dags that have been used for welltypedness
 * unifications in an unsuccessful (top level) unification.
 */
void init_constraint_cache(type_t ntypes);
/** Free all data structures associated with the constraint cache, except for
 *  the dags, which are deleted by their respective allocators.
 */
void free_constraint_cache(type_t ntypes);
/** After this function is called, dags are created non-permanent by default.
 *
 * \todo This should be better integrated with the t_alloc and p_alloc stuff.
 */
void stop_creating_permanent_dags();

/** \brief Initialize the dag with default values, except for the type, which
 *  is set to \a s. This function is called after allocation, and would
 *  normally be called in the constructor.
 */
inline void dag_init(dag_node *dag, type_t s)
{
  dag->type = s;
  dag->arcs = 0;
  dag->generation = -1;

#ifdef MARK_PERMANENT
  dag->permanent = create_permanent_dags;
#endif
}

/** I've got no clue
 *  \todo would someone please explain this?
 */
inline dag_node *dag_deref(dag_node *dag) { return dag; }
/** Get the type of the dag */
inline type_t dag_type(dag_node *dag) { return dag->type; }
/** Set the type of the dag */
inline void dag_set_type(dag_node *dag, type_t s) { dag->type = s; }

/** Clone the dag (deep copy). */
dag_node *dag_full_copy(dag_node *dag);
/** Unify and two dags and copy the result, if successful.
 *  \param root the root node of the first dag 
 *  \param dag1 a subnode of \a root, or equal to it
 *  \param dag2 the second dag to be unified with \a dag1
 *  \param del a list of attribute ids. Arcs in the resulting root dag that
 *             have attributes contained in this list are deleted.
 *  \return a (partial) copy of the result, if successful, \c FAIL otherwise.
 */
dag_node *dag_unify(dag_node *root, dag_node *dag1,
                    dag_node *dag2, struct list_int *del);

/** Return \c true, if \a dag1 and \a dag2 are unifiable */
bool dags_compatible(dag_node *dag1, dag_node *dag2);

/** \brief bidirectional subsumption test. \a forward is \c true if \a dag1
 *  subsumes (is more general than) \a dag2, \a backward analogously.
 *  \pre the boolean variables bound to \a forward and \a backward have to be
 *  \c true when this function is called, otherwise, the direction will not be
 *  checked.
 */
void 
dag_subsumes(dag_node *dag1, dag_node *dag2, bool &forward, bool &backward);

/** Return a permanent (deep) copy of \a dag.
 * This function is similar to dag_full_copy(), except that it allocates
 * permanent dag nodes and arcs.
 */
dag_node *
dag_full_p_copy(dag_node *dag);

/** Recursively check if dag is valid for debugging */
bool dag_valid(dag_node *dag);

#ifdef QC_PATH_COMP
/** Return a list of unification_failure points by replaying a (failing)
 *  unification.
 * \attn the caller must free the unification_failure structures.
 * \param dag1 first dag to unify
 * \param dag2 second dag to unify
 * \param all_failures record all failures or only the first one
 * \param initial_path if \a dag1 is a substructure of some dag, the correct
 *                     paths are returned when passing the path to that
 *                     structure in this argument
 * \param result_root if the failure was caused because a cyclic structure was
 *                    produced, the cyclic result will be copied to this
 *                    variable, if it is non-NULL.
 */
list<class unification_failure *>
dag_unify_get_failures(dag_node *dag1, dag_node *dag2, bool all_failures,
                       struct list_int *initial_path = 0,
                       dag_node **result_root = 0);

/** Return all (non-cyclic) paths from \a dag to \a search */
list<struct list_int *> dag_paths(dag_node *dag, dag_node *search);

/** Return a list of unification_failure points by replaying a (failing)
 *  subsumption.
 * \attn the caller must free the unification_failure structures.
 * \param dag1 first dag to unify
 * \param dag2 second dag to unify
 * \param forward the \a forward argument to dag_subsumes()
 * \param backward the \a backward argument to dag_subsumes()
 * \param all_failures record all failures or only the first one
 */
list<class unification_failure *>
dag_subsumes_get_failures(dag_node *dag1, dag_node *dag2,
                          bool &forward, bool &backward,
                          bool all_failures);
#endif

/** @name Temporary Dags (for hyperactive parsing)
 */
/*@{*/
/** Unify two dags, but do not copy the result if unification is successful,
 *  just return the temporary dag, and \c FAIL, otherwise.
 *
 *  \param root the root node of the first dag 
 *  \param dag1 a subnode of \a root, or equal to it
 *  \param dag2 the second dag to be unified with \a dag1
 *
 *  The result will have valid data in the generation protected members of the
 *  dag nodes. With the result, another unification can be performed, and thus,
 *  unifiability between three dags can be checked without copying the first
 *  result.
 */
dag_node *dag_unify_temp(dag_node *root, dag_node *dag1, dag_node *dag2);

/** Extract a quick check vector from a temporary dag.
 *  This routine must also take into account the generation protected members
 *  of the dag nodes.
 */
void dag_get_qc_vector_temp(struct qc_node *qc_paths, dag_node *dag,
                            type_t *qc_vector);

/** Get the substructure under ARGS.REST*(n-1).FIRST , starting at the
 *  temporary dag node \a dag, if it exists, \c FAIL otherwise.
 */
dag_node *dag_nth_arg_temp(dag_node *dag, int n);

/** Print \a dag readably to \a f, if \a temporary is \c true, the dag will be
 *  treated as such, and the generation protected members will be considered
 *  too, to print its complete state.
 */
void dag_print_safe(FILE *f, dag_node *dag, bool temporary, 
                    int format = DAG_FORMAT_TRADITIONAL);
/*@}*/

/** Print \a dag to \a f in \em fegramed syntax. */
void dag_print_fed_safe(FILE *f, dag_node *dag);

/** Print \a dag to \a f in a special, compact syntax, originally intended for
 *  exchange with Java servers/clients.
 */
void dag_print_jxchg(ostream &f, dag_node *dag);

/** @name Generation Protected Slots
 * Accessor functions for the generation protected slots -- inlined for
 * efficiency.
 */
/*@{*/
inline type_t dag_get_new_type(dag_node *dag)
{
  return (dag->generation == unify_generation) ? dag->new_type : dag->type ;
}

inline struct dag_arc *dag_get_comp_arcs(dag_node *dag)
{
  if(dag->generation == unify_generation) return dag->comp_arcs; else return 0;
}

inline dag_node *dag_get_forward(dag_node *dag)
{
  if(dag->generation == unify_generation) return dag->forward; else return 0;
}

inline dag_node *dag_get_copy(dag_node *dag)
{
  if(dag->generation == unify_generation) return dag->copy; else return 0;
}

inline void dag_set_new_type(dag_node *dag, type_t s)
{
  dag->new_type = s;
  if(dag->generation != unify_generation)
    {
      dag->generation = unify_generation;
      dag->comp_arcs = 0;
      dag->forward = 0;
      dag->copy = 0;
    }
}

inline void dag_set_comp_arcs(dag_node *dag, struct dag_arc *a)
{
  dag->comp_arcs = a;
  if(dag->generation != unify_generation)
    {
      dag->generation = unify_generation;
      dag->new_type = dag->type;
      dag->forward = 0;
      dag->copy = 0;
    }
}

inline void dag_set_forward(dag_node *dag, dag_node *c)
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

inline void dag_set_copy(dag_node *dag, dag_node *c)
{
  dag->copy = c;
  if(dag->generation != unify_generation)
    {
      dag->generation = unify_generation;
      dag->comp_arcs = 0;
      dag->forward = 0;
      dag->new_type = dag->type;
    }
}
/*@}*/

/** Invalidate all generation protected slots */
inline void dag_invalidate_changes()
{
  if(unify_generation > unify_generation_max)
    unify_generation_max = unify_generation;

  unify_generation = ++unify_generation_max;
}

/** Has this node been visited?
 * we just `overload' the copy slot to make life for the printing stuff easier.
 */
inline ptr2int_t dag_get_visit(dag_node *dag)
{
  return (ptr2int_t) dag_get_copy(dag);
}

/** Mark this node as visited.
 * we just `overload' the copy slot to make life for the printing stuff easier.
 */
inline ptr2int_t dag_set_visit(dag_node *dag, ptr2int_t visit)
{
  dag->generation = unify_generation;
  return (ptr2int_t) (dag->copy = (dag_node *) visit);
}

/** Invalidate all `visited' marks */
inline void dag_invalidate_visited()
{
  dag_invalidate_changes();
}

/** A class to save the current global generation counter in a local
 * environment. The generation counter will be reset on destruction of an
 * object of this class.
 */
class temporary_generation
{
 public:
  /** Save the global generation counter and set its value to \a gen */
  inline temporary_generation(int gen) : _save(unify_generation) 
    { if(gen != 0) unify_generation = gen; }
  /** Restore the saved generation counter */
  inline ~temporary_generation()
    { unify_generation = _save; }

 private:
  int _save;
};

dag_node *dag_partial_copy1(dag_node *dag, type_t new_type);

/** Clone \a dag using \a del as a restrictor. 
 * At every occurence of a feature in \a del, the node below that feature will
 * be replaced by an empty dag node with the maximal appropriate type. In this
 * variant, the restrictor remains constant on every recursion level, which
 * guarantees minimal overhead. This is not possible for all kinds of
 * restrictors, so there is a variant that keeps restrictor states too.
 */
template< typename STATELESS_RESTRICTOR >
dag_node *
dag_partial_copy_stateless(dag_node *dag, const STATELESS_RESTRICTOR &del)
{
    dag_node *copy;
    
    copy = dag_get_copy(dag);
    
    if(copy == 0)
    {
        dag_arc *arc;
        
        copy = new_dag(dag->type);
        
        dag_set_copy(dag, copy);
        
        arc = dag->arcs;
        while(arc != 0)
        { 
            dag_add_arc(copy,
                        new_arc(arc->attr, 
                                (del.prune_arc(arc->attr) ?
                                 dag_partial_copy1(arc->val
                                                   , maxapp[arc->attr])
                                 : dag_partial_copy_stateless(arc->val, del))));
                                
            arc = arc->next;
        }
    }
    
    return copy;
}




/** Clone \a dag using the restrictor \a rst.
 *
 * If the restrictor tells to remove a node, the node will be replaced by an
 * empty dag node with the maximal appropriate type of the feature pointing to
 * it. The difference to the \c dag_partial_copy_stateless variant lies in the
 * creation of a new restrictor state on every recursion level. The restrictor
 * states should be lightweight objects as to produce minimal creation and
 * destruction overhead.
 */
template< typename R_STATE >
dag_node *dag_partial_copy_state(dag_node *dag, const R_STATE &rst)
{
  dag_node *copy;
    
  copy = dag_get_copy(dag);
    
  if(copy == 0)
    {
      if (rst.full())
        return dag_full_copy(dag);

      dag_arc *arc;
      dag_node *new_node;
        
      copy = new_dag(dag->type);
        
      dag_set_copy(dag, copy);

      arc = dag->arcs;
      while(arc != 0) {
        R_STATE new_state = rst.walk_arc(arc->attr);

        if (new_state.empty()) {
          new_node = dag_partial_copy1(arc->val, maxapp[arc->attr]);
        } else {
          new_node = dag_partial_copy_state(arc->val, new_state);
        }

        dag_add_arc(copy, new_arc(arc->attr, new_node));
          
        arc = arc->next;
        }
    }
      
  return copy;
}

#endif
