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

/* tomabechi quasi destructive graph unification  */
/* inspired by the LKB implementation */

#include "dag.h"
#include "tsdb++.h"
#include "options.h"
#include "logging.h"
#include <iomanip>

using namespace std;

int unify_generation = 0;
int unify_generation_max = 0;

#ifdef MARK_PERMANENT
bool create_permanent_dags = true;
#endif

void stop_creating_permanent_dags() {
#ifdef MARK_PERMANENT
  create_permanent_dags = false;
#endif
}

inline bool dag_permanent(dag_node *dag) {
#ifdef MARK_PERMANENT
  return dag->permanent;
#else
  return is_p_addr(dag);
#endif
}

#ifdef EXTENDED_STATISTICS
int unify_nr_newtype, unify_nr_comparc, unify_nr_forward, unify_nr_copy;
#endif

template<bool record_failure> class recfail {
public:
  static bool dag_subsumes1(dag_node *dag1, dag_node *dag2,
                            bool &forward, bool &backward);

  static dag_node *dag_unify1(dag_node *dag1, dag_node *dag2);

  static dag_node * dag_unify2(dag_node *dag1, dag_node *dag2);

  static inline bool unify_arcs1(dag_arc *arcs, dag_arc *arcs1,
                                 dag_arc *comp_arcs1, dag_arc **new_arcs1);

  static inline bool dag_unify_arcs(dag_node *dag1, dag_node *dag2);

  static dag_node * dag_cyclic_arcs(dag_arc *arc);

  static dag_node * dag_cyclic_rec(dag_node *dag);
};

dag_node *dag_get_attr_value(dag_node *dag, attr_t attr) {
  dag_arc *arc = dag_find_attr(dag->arcs, attr);
  if(arc == 0) return FAIL;

  return arc->val;
}

bool dag_set_attr_value(dag_node *dag, attr_t attr, dag_node *val) {
  dag_arc *arc;

  arc = dag_find_attr(dag->arcs, attr);
  if(arc == 0) return false;
  arc->val = val;
  return true;
}

dag_node *dag_copy(dag_node *src, list_int *del);

//bool
//dag_subsumes1(dag_node *dag1, dag_node *dag2, bool &forward, bool &backward);

dag_node *
dag_unify(dag_node *root, dag_node *dag1, dag_node *dag2, list_int *del) {
  dag_node *res;

  unification_cost = 0;

  if((res = recfail<false>::dag_unify1(dag1, dag2)) != FAIL) {
    stats.copies++;
    res = dag_copy(root, del);
  }

  dag_invalidate_changes();

  return res;
}

dag_node *dag_unify_temp(dag_node *root, dag_node *dag1, dag_node *dag2) {
  unification_cost = 0;
  if(recfail<false>::dag_unify1(dag1, dag2) == FAIL)
    return FAIL;
  else
    return root;
}

#include "failure.h"

/** Global flag deciding if all or only the first failure during
 *  unification/subsumption will be recorded.
 */
static bool all_failures = false;

/** The last unification failure, if not equal to \c NULL */
failure *last_failure = 0;

/** contains the reverse feature path to the current dag node during
 *  unification.
 */
list_int *path_reverse = 0;

/** A list of all failures occured when \c all_failures is \c true */
list<failure *> failures;

bool dags_compatible(dag_node *dag1, dag_node *dag2) {
  bool res = true;

  unification_cost = 0;

  if(recfail<false>::dag_unify1(dag1, dag2) == FAIL)
    res = false;

#ifdef STRICT_LKB_COMPATIBILITY
  else
    res = !recfail<false>::dag_cyclic_rec(dag1);
#endif

  dag_invalidate_changes();

  return res;
}

//
// recording of failures
//

void clear_failure() {
  if(last_failure) {
    delete last_failure;
    last_failure = 0;
  }
}

void save_or_clear_failure() {
  if(all_failures) {
    if(last_failure) {
      failures.push_back(last_failure);
      last_failure = 0;
    }
  }
  else
    clear_failure();
}

void start_recording_failures() {
  clear_failure();
}

failure * stop_recording_failures() {
  return last_failure;
}

dag_node *dag_cyclic_copy(dag_node *src, list_int *del);

list<failure *>
dag_unify_get_failures(dag_node *dag1, dag_node *dag2, bool get_all_failures,
                       list_int *initial_path, dag_node **result_root) {
  all_failures = get_all_failures;

  clear_failure();
  failures.clear();
  unification_cost = 0;

  if(path_reverse != 0)
    fprintf(ferr, "dag_unify_get_failures: path_reverse not empty\n");
  path_reverse = reverse(initial_path);

  recfail<true>::dag_unify1(dag1, dag2);

  if(last_failure) {
    failures.push_back(last_failure);
    last_failure = 0;
  }

#ifdef COMPLETE_FAILURE_REPORTING

  dag_node *cycle;
  if(result_root != 0) {
    *result_root = dag_cyclic_copy(*result_root, 0);

    // result might be cyclic
    free_list(path_reverse); path_reverse = 0;
    if((cycle = dag_cyclic_rec(*result_root)) != 0) {
        dag_invalidate_changes();
        failures.push_back(new failure(failure::CYCLE, path_reverse
                                       , unification_cost
                                       , -1, -1, cycle
                                       , *result_root));
      }
  }
  else {
    // result might be cyclic
    if((cycle = dag_cyclic_rec(dag1)) != 0) {
        failures.push_back(new failure(failure::CYCLE, path_reverse
                                       , unification_cost));
      }
  }

#endif

  free_list(path_reverse); path_reverse = 0;

  dag_invalidate_changes();

  return failures; // caller is responsible for free'ing the paths
}

list<failure *>
dag_subsumes_get_failures(dag_node *dag1, dag_node *dag2,
                          bool &forward, bool &backward,
                          bool get_all_failures) {
  all_failures = get_all_failures;

  clear_failure();
  failures.clear();
  unification_cost = 0;

  if(path_reverse != 0)
    fprintf(ferr, "dag_subsumes_get_failures: path_reverse not empty\n");

  path_reverse = 0;

  recfail<true>::dag_subsumes1(dag1, dag2, forward, backward);

  if(last_failure) {
    failures.push_back(last_failure);
    last_failure = 0;
  }

  free_list(path_reverse); path_reverse = 0;

  dag_invalidate_changes();

  return failures; // caller is responsible for free'ing the paths
}

inline bool dag_has_arcs(dag_node *dag) {
  return dag->arcs || dag_get_comp_arcs(dag);
}

template<bool record_failure> dag_node *
recfail<record_failure>::dag_unify1(dag_node *dag1, dag_node *dag2) {
  unification_cost++;

  dag1 = dag_deref1(dag1);
  dag2 = dag_deref1(dag2);

  if(dag_get_copy(dag1) == INSIDE) {
    stats.cycles++;

#ifdef COMPLETE_FAILURE_REPORTING
    if(record_failure) {
      // _fix_me_ this is not right
      // _fix_doc_ But what is the problem ??
      if(!all_failures) {
        save_or_clear_failure();
        last_failure =
          new failure(failure::CYCLE, path_reverse, unification_cost);
        return FAIL;
      }
      else
        return dag1; // continue with cyclic structure
    }
    else
#endif
      return FAIL;
  }

  if(dag1 != dag2) {
    if(recfail<record_failure>::dag_unify2(dag1, dag2) == FAIL)
      return FAIL;
  }

  return dag1;
}

//
// constraint stuff
//

dag_node *new_p_dag(type_t s) {
  dag_node *dag = dag_alloc_p_node();
  dag_init(dag, s);

#ifdef MARK_PERMANENT
  dag->permanent = true;
#endif

  return dag;
}

dag_arc *new_p_arc(attr_t attr, dag_node *val, dag_arc *next = NULL) {
  dag_arc *newarc = dag_alloc_p_arc();
  newarc->attr = attr;
  newarc->val = val;
  newarc->next = next;
  return newarc;
}

dag_node *dag_full_p_copy(dag_node *dag) {
  dag_node *copy;
  copy = dag_get_copy(dag);

  if(copy == 0) {
    dag_arc *arc;
    copy = new_p_dag(dag->type);
    dag_set_copy(dag, copy);

    arc = dag->arcs;
    while(arc != 0) {
      dag_add_arc(copy, new_p_arc(arc->attr, dag_full_p_copy(arc->val)));
      arc = arc->next;
    }
  }

  return copy;
}

int total_cached_constraints = 0;

/** Cache for type dags that have been allocated but not used due to
 *  unification failure. Only type dags containing arcs are cached here, so we
 *  do not need to worry about dynamic types, since their type dags don't
 *  contain arcs
 */
constraint_info **constraint_cache = NULL;

/** Initialize the constraint cache to the given \a size (must be the number
 *  of static types).
 */
void init_constraint_cache(type_t size) {
  if (constraint_cache) { delete[] constraint_cache; }
  constraint_cache = new constraint_info *[size];
  for(type_t i = 0; i < size; i++) constraint_cache[i] = NULL;
}

/** Free the constraint cache */
void free_constraint_cache(type_t size) {
  for(type_t i = 0; i < size; i++) {
    constraint_info *c = constraint_cache[i];
    while(c != NULL) {
      constraint_info *n = c->next;
      delete c;
      c = n;
    }
  }

  delete[] constraint_cache;
}

/** Return an unused type dag for type \a s.
 *  Either an unused dag can be found in the cache and is returned or another
 *  (permanent) copy is made, added to the cache, and returned.
 */
struct dag_node *cached_constraint_of(type_t s) {
  constraint_info *c = constraint_cache[s];

  while(c && c->gen == unify_generation)
    c = c->next;

  if(c == 0) {
    total_cached_constraints++;
    c = new constraint_info;
    c->next = constraint_cache[s];
    /* Create a new permanent type dag (by coping) in a constraint_info bucket
     * for use with the constraint_cache.
     *
     * The copy is made with an increased generation counter because otherwise
     * the copy slots would not be invalidated and structures from the
     * unification could interfere.
     *
     * But If all type dag unifications come from
     * the constraint cache, which makes full copies of the type dags, the
     * original dag from the data base should never be touched. \todo What fact
     * am i missing here?
     *
     * If it is right that the permanent dags never get touched by this scheme,
     * we could use it for rule and lexicon entry dags too and get rid of the
     * permanent storage problem.
     *
     * All this permanent storage thing is there to
     * support partial structure sharing
     */
    temporary_generation save(++unify_generation_max);
    // \todo check if this works for a big corpus
    c -> dag = dag_full_p_copy(type_dag(s));
    //c->dag = dag_full_copy(type_dag(s));
    constraint_cache[s] = c;
  }

  c->gen = unify_generation;

  return c->dag;
}

inline bool
dag_make_wellformed(type_t new_type, dag_node *dag1, type_t s1, dag_node *dag2,
                    type_t s2) {
  if((s1 == new_type && s2 == new_type) ||
     (!dag_has_arcs(dag1) && !dag_has_arcs(dag2)) ||
     (s1 == new_type && dag_has_arcs(dag1)) ||
     (s2 == new_type && dag_has_arcs(dag2)))
    return true;

  bool res = true;
  // dynamic types have no arcs, so the next test is always false for dynamic
  // types, which is why we don't need cached constraints for them
  if(type_dag(new_type)->arcs) {
    // switch of failure registration for wellformedness unification
    res = recfail<false>::
      dag_unify1(dag1, cached_constraint_of(new_type)) != FAIL;
  }

  return res;
}

/** Unify all arcs in \a arcs against the arcs in \a arcs1 and \a comp_arcs1.
 * Unification with an arc in \a comp_arcs1 is only performed when no
 * corresponding arc exists in \a arcs1.
 * \return \c false if a unification failure occured, \c true otherwise, and in
 * this case return the unified arcs in \a new_arcs1
 */
template<bool record_failure> inline bool recfail<record_failure>::
unify_arcs1(dag_arc *arcs, dag_arc *arcs1,
            dag_arc *comp_arcs1, dag_arc **new_arcs1) {
  dag_node *node;
  dag_arc *arc;
  while(arcs != 0) {
    if(((arc = dag_find_attr(arcs1, arcs->attr)) != NULL) ||
       ((arc = dag_find_attr(comp_arcs1, arcs->attr)) != NULL)) {
      node = arc->val;
      if(record_failure)
        path_reverse = cons(arcs->attr, path_reverse);
      if(recfail<record_failure>::dag_unify1(node, arcs->val) == FAIL) {
        if(record_failure) {
          path_reverse = pop_rest(path_reverse);
          if(!all_failures) return false;
        }
        else
          return false;
      }
      if(record_failure)
        path_reverse = pop_rest(path_reverse);
    }
    else { // not in the arcs of the first dag
      *new_arcs1 = dag_cons_arc(arcs->attr, arcs->val, *new_arcs1);
    }

    arcs = arcs->next;
  }
  return true;
}

/** Iterate over the \c arcs and \c comp_arcs of \c dag2 and unify them with
 * the \c arcs and \c comp_arcs of \c dag1
 *
 * As far as i see, arcs and comp_arcs should always be disjoint. That is the
 * reason why this method and the copying work OK without doing a) too much
 * work and b) producing duplicate arcs with equal labels.
 */
template<bool record_failure> inline bool recfail<record_failure>::
dag_unify_arcs(dag_node *dag1, dag_node *dag2) {
  dag_arc *arcs1, *comp_arcs1, *new_arcs1;

  arcs1 = dag1->arcs;
  new_arcs1 = comp_arcs1 = dag_get_comp_arcs(dag1);

  // What is the reason that an arc can not appear in the arcs and in the
  // comp_arcs of a dag? If that would be the case, two arcs with the same
  // feature could be in new_arcs1
  if(! recfail<record_failure>::
     unify_arcs1(dag2->arcs, arcs1, comp_arcs1, &new_arcs1))
    return false;

  if(! recfail<record_failure>::
     unify_arcs1(dag_get_comp_arcs(dag2), arcs1, comp_arcs1, &new_arcs1))
    return false;

  if(new_arcs1 != 0) {
    dag_set_comp_arcs(dag1, new_arcs1);
  }

  return true;
}

template<bool record_failure> dag_node * recfail<record_failure>::
dag_unify2(dag_node *dag1, dag_node *dag2) {
  type_t s1, s2, new_type;

  new_type = glb((s1 = dag_get_new_type(dag1)), (s2 = dag_get_new_type(dag2)));

  if(new_type == T_BOTTOM) {
    if(record_failure) {
      save_or_clear_failure();
      last_failure = new failure(failure::CLASH, path_reverse, unification_cost
                                 , s1, s2);
      if(!all_failures)
        return FAIL;
      else
        new_type = s1;
    }
    else
      return FAIL;
  }

#if 0
  if(LOG_ENABLED(logSemantic, DEBUG)) {
    if(new_type != s1 || new_type != s2) {
      if((dag_has_arcs(dag1) && featset[s1] != featset[new_type])
         || (dag_has_arcs(dag2) && featset[s2] != featset[new_type])) {
        if((dag_has_arcs(dag1) && featset[s1] == featset[new_type])
           || (dag_has_arcs(dag2) && featset[s2] == featset[new_type]))
          LOG(logSemantic, DEBUG, "glb: one compatible set");
        else
          LOG(logSemantic, DEBUG,
              "glb: " << type_name(s1) << dag_has_arcs(dag1) ? "[]" : "" 
              << "(" << featset[s1] << ")"
              << type_name(s2) << dag_has_arcs(dag2) ? "[]" : "",
              << "(" << featset[s2] << ") -> "
              << type_name(new_type) << "(" << featset[new_type] << ")");
      } else
        LOG(logSemantic, DEBUG, "glb: compatible feature sets");
    } else {
      LOG(logSemantic, DEBUG, "glb: type unchanged");
    }
  }
#endif

  // _fix_me_ maybe check if actually changed
  dag_set_new_type(dag1, new_type);

  if(unify_wellformed) {
    if(!dag_make_wellformed(new_type, dag1, s1, dag2, s2)) {
#ifdef COMPLETE_FAILURE_REPORTING
      if(record_failure) {
        save_or_clear_failure();
        last_failure = new failure(failure::CONSTRAINT, path_reverse
                                   , unification_cost, s1, s2);

        if(!all_failures)
          return FAIL;
      } else
#endif
        return FAIL;
    }

    dag1 = dag_deref1(dag1);
  }

  if(!dag_has_arcs(dag2)) { // try the cheapest (?) solution first
    dag_set_forward(dag2, dag1);
  } else
    if(!dag_has_arcs(dag1)) {
      dag_set_new_type(dag2, new_type);
      dag_set_forward(dag1, dag2);
    }
    else {
      dag_set_copy(dag1, INSIDE);
      dag_set_forward(dag2, dag1);

      if(!dag_unify_arcs(dag1, dag2)) {
        dag_set_copy(dag1, 0);
        return FAIL;
      }

      dag_set_copy(dag1, 0);
    }

  return dag1;
}

dag_node *dag_full_copy(dag_node *dag) {
  dag_node *copy = dag_get_copy(dag);

  if(copy == 0) {
    copy = new_dag(dag->type);
    dag_set_copy(dag, copy);

    dag_arc *arc = dag->arcs;
    while(arc != 0) {
      dag_add_arc(copy, new_arc(arc->attr, dag_full_copy(arc->val)));
      arc = arc->next;
    }
  }

  return copy;
}


/** Return a new node with no arcs and type \a new_type as copy of \a dag.
 * This function is called when the restrictor cuts at node \a dag.
 */
struct dag_node *dag_partial_copy1(dag_node *dag, type_t new_type) {
  dag_node *copy = dag_get_copy(dag);

  if(copy == 0) {
    copy = new_dag(new_type);
    dag_set_copy(dag, copy);
  }
  else {
    copy->arcs = NULL;
    copy->type = new_type;
  }

  return copy;
}

void
dag_subsumes(dag_node *dag1, dag_node *dag2, bool &forward, bool &backward) {
  unification_cost = 0;
  recfail<false>::dag_subsumes1(dag1, dag2, forward, backward);
  dag_invalidate_changes();
}


template<bool record_failure> bool recfail<record_failure>::
dag_subsumes1(dag_node *dag1, dag_node *dag2, bool &forward, bool &backward) {
  unification_cost++;

  dag_node *c1 = dag_get_forward(dag1),
    *c2 = dag_get_copy(dag2);

  if(forward) {
    if(c1 == 0) {
      dag_set_forward(dag1, dag2);
    }
    else if(c1 != dag2) {
      forward = false;
#ifdef COMPLETE_FAILURE_REPORTING
      if (record_failure){
        save_or_clear_failure();
        last_failure = new failure(failure::COREF, path_reverse, 1);
      }
#endif
    }
  }

  if(backward) {
    if(c2 == 0) {
      dag_set_copy(dag2, dag1);
    }
    else if(c2 != dag1) {
      backward = false;
#ifdef COMPLETE_FAILURE_REPORTING
      if (record_failure){
        save_or_clear_failure();
        last_failure = new failure(failure::COREF, path_reverse, 1);
      }
#endif
    }
  }

  if(forward == false && backward == false) {
    if(!record_failure)
      return false;
    if (!all_failures)
      return false;
  }

  if(dag1->type != dag2->type) {
    bool st_12, st_21;
    subtype_bidir(dag1->type, dag2->type, st_12, st_21);
    if(!st_12) {
      backward = false;
      if (record_failure) {
        save_or_clear_failure();
        last_failure = new failure(failure::CLASH, path_reverse, 1,
                                   dag2->type, dag1->type);
      }
    }
    if(!st_21) {
      forward = false;
      if (record_failure) {
        save_or_clear_failure();
        last_failure = new failure(failure::CLASH, path_reverse, 1,
                                   dag1->type, dag2->type);
      }
    }

    if(forward == false && backward == false) {
      if (! record_failure)
        return false;
      if (!all_failures)
        return false;
    }
  }

  if(dag1->arcs || dag2->arcs) {
    // Partial expansion makes this slightly more complicated. We
    // need to visit a structure even when the corresponding part
    // has been unfilled so that we find all coreferences.
    dag_node *tmpdag1 = dag1, *tmpdag2 = dag2;

    if(!dag1->arcs && type_dag(dag1->type)->arcs)
      tmpdag1 = cached_constraint_of(dag1->type);

    if(!dag2->arcs && type_dag(dag2->type)->arcs)
      tmpdag2 = cached_constraint_of(dag2->type);

    dag_arc *arc1 = tmpdag1->arcs;

    while(arc1) {
      dag_arc *arc2 = dag_find_attr(tmpdag2->arcs, arc1->attr);

      if(arc2) {
        if(record_failure)
          path_reverse = cons(arc1->attr, path_reverse);
        if(!dag_subsumes1(arc1->val, arc2->val, forward, backward)) {

          // We will not get here when all_failures is on,
          // since we never return false. So we can always return
          // from here, unconditionally.
          if(record_failure)
            path_reverse = pop_rest(path_reverse);
          return false;
        }
        if(record_failure)
          path_reverse = pop_rest(path_reverse);
      }

      arc1 = arc1->next;
    }
  }

  return true;
}

static list<list_int *> paths_found;

void dag_paths_rec(dag_node *dag, dag_node *search) {
  dag_node *copy;

  if(dag == search)
    paths_found.push_back(reverse(path_reverse));

  copy = dag_get_copy(dag);
  if(copy == 0) {
    dag_arc *arc;

    dag_set_copy(dag, INSIDE);

    arc = dag->arcs;
    while(arc != 0) {
      path_reverse = cons(arc->attr, path_reverse);
      dag_paths_rec(arc->val, search);
      path_reverse = pop_rest(path_reverse);
      arc = arc->next;
    }
  }
}

list<list_int *> dag_paths(dag_node *dag, dag_node *search) {
  paths_found.clear();

  if(path_reverse != 0) {
    free_list(path_reverse); path_reverse = 0;
  }

  dag_paths_rec(dag, search);

  dag_invalidate_changes();
  return paths_found; // caller is responsible for free'ing paths
}

/* like naive dag_copy, but copies cycles */

dag_node *dag_cyclic_copy(dag_node *src, list_int *del) {
  dag_node *copy;

  src = dag_deref1(src);
  copy = dag_get_copy(src);

  if(copy != 0)
    return copy;

  copy = new_dag(dag_get_new_type(src));
  dag_set_copy(src, copy);

  dag_arc *new_arcs = 0;
  dag_arc *arc;
  dag_node *v;

  arc = src->arcs;
  while(arc) {
    if(!contains(del, arc->attr)) {
      if((v = dag_cyclic_copy(arc->val, 0)) == FAIL)
        return FAIL;

      new_arcs = dag_cons_arc(arc->attr, v, new_arcs);
    }
    arc = arc->next;
  }

  arc = dag_get_comp_arcs(src);
  while(arc) {
    if(!contains(del, arc->attr))
      {
        if((v = dag_cyclic_copy(arc->val, 0)) == FAIL)
          return FAIL;

        new_arcs = dag_cons_arc(arc->attr, v, new_arcs);
      }
    arc = arc->next;
  }

  copy->arcs = new_arcs;

  return copy;
}

#ifdef SMART_COPYING

/* (almost) non-redundant copying (Tomabechi/Malouf/Caroll) from the LKB */

/** Return \c true if the arc list starting at \a arc contains an arc with
 *  feature \a attr.
 *
inline bool arcs_contain(dag_arc *arc, attr_t attr) {
  while(arc && (arc->attr != attr)) arc = arc->next; return (arc != NULL);
}
*/

dag_node *dag_copy(dag_node *src, list_int *del) {
  unification_cost++;

  src = dag_deref1(src);
  dag_node *copy = dag_get_copy(src);

  if(copy == FAIL)
    copy = NULL;
  else
    if(copy == INSIDE) {
      stats.cycles++;
      return FAIL;
    } else
      if (copy != NULL) return copy;

  type_t type = dag_get_new_type(src);
  dag_arc *new_arcs = dag_get_comp_arcs(src);
  if(src->arcs == 0 && new_arcs == 0) {     // `atomic' node
    if (dag_permanent(src) || type != src->type)
      copy = new_dag(type);
    else
      copy = src;
    dag_set_copy(src, copy);
    return copy;
  }

  dag_set_copy(src, INSIDE);
  bool copy_p = dag_permanent(src) || type != src->type || new_arcs != NULL;

  dag_node *v;
  dag_arc *arc = src->arcs, *next, *result_arcs = NULL;
  // copy arcs - could reuse unchanged tail here - too hairy for now
  while(arc) {
    //assert(!arcs_contain(new_arcs, arc->attr));
    if(!contains(del, arc->attr)) {
      if((v = dag_copy(arc->val, NULL)) == FAIL) return FAIL;

      if(arc->val != v) {
        if (! copy_p) {
          // clone all arcs before arc
          dag_arc *old = src->arcs;
          while(old != arc) {
            if(!contains(del, old->attr))
              result_arcs = dag_cons_arc(old->attr, old->val, result_arcs);
            old = old->next;
          }
          copy_p = true;
        }
        result_arcs = dag_cons_arc(arc->attr, v, result_arcs);
      } else {
        if (copy_p) // clone arc
          result_arcs = dag_cons_arc(arc->attr, arc->val, result_arcs);
      }
    }
    arc = arc->next;
  }

  // copy comp-arcs - top level arcs can be reused
  arc = new_arcs;
  while(arc) {
    next = arc->next;
    if(!contains(del, arc->attr)) {
      if((arc->val = dag_copy(arc->val, NULL)) == FAIL) return FAIL;
      arc->next = result_arcs;
      result_arcs = arc;
    }
    arc = next;
  }

  if(copy_p) {
    copy = new_dag(type);
    copy->arcs = result_arcs;
  } else {
    copy = src;
  }

  dag_set_copy(src, copy);

  return copy;
}

#else

/* plain vanilla copying (as in the paper) */

dag_node *dag_copy(dag_node *src, list_int *del) {
  unification_cost++;

  src = dag_deref1(src);
  dag_node *copy = dag_get_copy(src);

  if(copy == INSIDE) {
    stats.cycles++;
    return FAIL;
  }

  if(copy != 0)
    return copy;

  dag_set_copy(src, INSIDE);

  dag_arc *new_arcs = 0;
  dag_node *v;

  dag_arc *arc = src->arcs;
  while(arc) {
    if(!contains(del, arc->attr)) {
      if((v = dag_copy(arc->val, 0)) == FAIL)
        return FAIL;

      new_arcs = dag_cons_arc(arc->attr, v, new_arcs);
    }
    arc = arc->next;
  }

  arc = dag_get_comp_arcs(src);
  while(arc) {
    if(!contains(del, arc->attr)) {
      if((v = dag_copy(arc->val, 0)) == FAIL)
        return FAIL;

      new_arcs = dag_cons_arc(arc->attr, v, new_arcs);
    }
    arc = arc->next;
  }

  copy = new_dag(dag_get_new_type(src));
  copy->arcs = new_arcs;

  dag_set_copy(src, copy);

  return copy;
}

#endif

//
// temporary dags
//

inline dag_node *dag_get_attr_value_temp(dag_node *dag, attr_t attr) {
  dag_arc *arc = dag_find_attr(dag_get_comp_arcs(dag), attr);
  if(arc) return arc->val;

  arc = dag_find_attr(dag->arcs, attr);
  if(arc) return arc->val;

  return FAIL;
}

// This is basically the prior dag_nth_arg_temp with the only difference
// being that the attribute can be specified. Made inline so that there will
// be no difference in performance.
inline struct dag_node *
dag_nth_element_temp(struct dag_node *dag, attr_t attr, int n) {
  int i;
  struct dag_node *arg;

  if((arg = dag_get_attr_value_temp(dag_deref1(dag), attr)) == FAIL)
    return FAIL;

  for(i = 1; (i < n
              && arg != NULL
              && arg != FAIL
              && !subtype(dag_get_new_type(arg), BI_NIL))
        ; i++)
    arg = dag_get_attr_value_temp(dag_deref1(arg), BIA_REST);

  if(i != n)
    return FAIL;

  arg = dag_get_attr_value_temp(dag_deref1(arg), BIA_FIRST);

  return arg;
}

struct dag_node *
dag_nth_element_temp(struct dag_node *dag, list_int *path, int n)
{
  // follow the path:
  if (dag == FAIL)
    return FAIL;
  int attr = first(path);
  while (rest(path)) {
    dag = dag_get_attr_value(dag, attr);
    path = rest(path);
    attr = first(path);
  }
  return dag_nth_element_temp(dag, attr, n); // inline call
}

struct dag_node *dag_nth_arg_temp(struct dag_node *dag, int n) {
  return dag_nth_element_temp(dag, BIA_ARGS, n); // inline call
}

void dag_get_qc_vector_temp(qc_node *path, dag_node *dag, type_t *qc_vector)
{
  if(dag == FAIL) return;

  dag = dag_deref1(dag);

  if(path->qc_pos > 0)
    qc_vector[path->qc_pos - 1] = dag_get_new_type(dag);

  // _fix_me_
  //  if(dag->arcs == 0 && dag_get_comp_arcs(dag) == 0)
  //    return;

  for(qc_arc *arc = path->arcs; arc != 0; arc = arc->next)
    dag_get_qc_vector_temp(arc->val,
                           dag_get_attr_value_temp(dag, arc->attr),
                           qc_vector);
}

template<bool record_failure> dag_node * recfail<record_failure>::
dag_cyclic_arcs(dag_arc *arc) {
  dag_node *v;

  while(arc) {
    if(record_failure)
      path_reverse = cons(arc->attr, path_reverse);

    if((v = dag_cyclic_rec(arc->val)) != 0)
      return v;

    if(record_failure)
      path_reverse = pop_rest(path_reverse);

    arc = arc->next;
  }

  return 0;
}

// this shall also work on temporary structures
template<bool record_failure> dag_node * recfail<record_failure>::
dag_cyclic_rec(dag_node *dag) {
  dag = dag_deref1(dag);

  dag_node *v = dag_get_copy(dag);

  if(v == 0) {
    // not yet seen
    unification_cost++;

    dag_set_copy(dag, INSIDE);

    if((v = dag_cyclic_arcs(dag->arcs)) != 0
       || (v = dag_cyclic_arcs(dag_get_comp_arcs(dag))) != 0)
      return v;

    dag_set_copy(dag, FAIL);
  }
  else if(v == INSIDE) {
    // cycle found
    return dag;
  }

  return NULL;
}

bool dag_cyclic(dag_node *dag) {
  bool res;

  res = (recfail<false>::dag_cyclic_rec(dag) != NULL);
  dag_invalidate_changes();
  return res;
}

//
// de-shrinking: make fs totally wellformed
//

map<int, bool> node_expanded;

dag_node *dag_expand_rec(dag_node *dag);

bool dag_expand_arcs(dag_arc *arcs) {
  while(arcs) {
    if(dag_expand_rec(arcs->val) == FAIL)
      return false;

    arcs = arcs->next;
  }

  return true;
}

dag_node *dag_expand_rec(dag_node *dag) {
  dag = dag_deref1(dag);

  if(node_expanded[(size_t) dag])
    return dag;

  node_expanded[(size_t) dag] = true;

  type_t new_type = dag_get_new_type(dag);

  // some kind of delta expansion is necessary here, since the typedags have
  // been unfilled, which can result in missing features.
  const list< type_t > &supers = all_supertypes(new_type);
  for (list< type_t >::const_iterator it = supers.begin()
         ; it != supers.end(); it++) {
    type_t super = *it ;
    // No need to check for dynamic types since their typedags have no arcs
    if(type_dag(super)->arcs && type_dag(super)->type == super) {
      if(recfail<false>::dag_unify1(dag, cached_constraint_of(super)) == FAIL){
        fprintf(ferr, "expansion failed @ 0x%x for `%s'\n",
                (size_t) dag, type_name(new_type));
        return FAIL;
      }
    }
  }
  dag = dag_deref1(dag);

  if(!dag_expand_arcs(dag->arcs))
    return FAIL;

  if(!dag_expand_arcs(dag_get_comp_arcs(dag)))
    return FAIL;

  return dag;
}

dag_node *dag_expand(dag_node *dag) {
  node_expanded.clear();
  dag_node *res = dag_expand_rec(dag);
  if(res != FAIL)
    res = dag_copy(res, 0);
  dag_invalidate_changes();
  return res;
}

//
// debugging code
//

bool dag_valid_rec(dag_node *dag) {
  if(dag == 0 || dag == INSIDE || dag == FAIL) {
    LOG(logSemantic, DEBUG,
        "(1) dag is 0x" << std::setbase(16) << (size_t) dag);
    return false;
  }

  dag = dag_deref1(dag);

  if(dag == 0 || dag == INSIDE || dag == FAIL) {
    LOG(logSemantic, DEBUG,"(2) dag is 0x" << std::setbase(16) << (size_t) dag);
    return false;
  }

  dag_node *v = dag_get_copy(dag);

  if(v == 0) {
    // not yet seen
    dag_arc *arc = dag->arcs;

    dag_set_copy(dag, INSIDE);

    while(arc) {
      if(! is_attr(arc->attr)) {
        LOG(logSemantic, DEBUG, "(3) invalid attr: " << arc->attr 
            << ", val: 0x" << std::setbase(16) << (size_t) arc->val);
        return false;
      }

      if(dag_valid_rec(arc->val) == false) {
        LOG(logSemantic, DEBUG, 
            "(4) invalid value under " << attrname[arc->attr]);
        return false;
      }

      arc = arc->next;
    }

    dag_set_copy(dag, FAIL);
  }
  else if(v == INSIDE) {
    // cycle found
    LOG(logSemantic, DEBUG, "(5) invalid dag: cyclic");
    return false;
  }

  return true;
}

bool dag_valid(dag_node *dag) {
  bool res;
  res = dag_valid_rec(dag);
  dag_invalidate_changes();
  return res;
}

void dag_initialize()
{
  // nothing to do
}

