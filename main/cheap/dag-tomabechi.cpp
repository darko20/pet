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

#include "pet-system.h"
#include "dag.h"
#include "types.h"
#include "tsdb++.h"
#include "options.h"

dag_node *INSIDE = (dag_node *) -2;

int unify_generation = 0;
int unify_generation_max = 0;

#ifdef MARK_PERMANENT
bool create_permanent_dags = true;
#endif

// _fix_me_
// This should be better integrated with the t_alloc and p_alloc stuff.
void stop_creating_permanent_dags()
{
#ifdef MARK_PERMANENT
  create_permanent_dags = false;
#endif
}

inline bool dag_permanent(dag_node *dag)
{
#ifdef MARK_PERMANENT
  return dag->permanent;
#else
  return is_p_addr(dag);
#endif
}

#ifdef EXTENDED_STATISTICS
int unify_nr_newtype, unify_nr_comparc, unify_nr_forward, unify_nr_copy;
#endif

dag_node *dag_cyclic_rec(dag_node *dag);

inline dag_node *
dag_deref1(dag_node *dag)
{
    dag_node *res;
    
    res = dag_get_forward(dag);
    
    while(res)
    {
        dag = res;
        res = dag_get_forward(dag);
    }
    
    return dag;
}

dag_arc *dag_find_attr(dag_arc *arc, int attr)
{
  while(arc)
    {
      if(arc->attr == attr)
	return arc;
      arc = arc->next;
    }

  return 0;
}

dag_node *dag_get_attr_value(dag_node *dag, int attr)
{
  dag_arc *arc;
  
  arc = dag_find_attr(dag->arcs, attr);
  if(arc == 0) return FAIL;
  
  return arc->val;
}

bool dag_set_attr_value(dag_node *dag, int attr, dag_node *val)
{
  dag_arc *arc;
  
  arc = dag_find_attr(dag->arcs, attr);
  if(arc == 0) return false;
  arc->val = val;
  return true;
}


dag_node *dag_unify1(dag_node *dag1, dag_node *dag2);
dag_node *dag_unify2(dag_node *dag1, dag_node *dag2);
dag_node *dag_copy(dag_node *src, list_int *del);

bool
dag_subsumes1(dag_node *dag1, dag_node *dag2, bool &forward, bool &backward);

dag_node *dag_unify(dag_node *root, dag_node *dag1, dag_node *dag2, list_int *del)
{
  dag_node *res;

  unification_cost = 0;

  if((res = dag_unify1(dag1, dag2)) != FAIL)
    {
      stats.copies++;
      res = dag_copy(root, del);
    }

  dag_invalidate_changes();

  return res;
}

dag_node *dag_unify_np(dag_node *root, dag_node *dag1, dag_node *dag2)
{
  unification_cost = 0;
  if(dag_unify1(dag1, dag2) == FAIL)
    return FAIL;
  else
    return root;
}

#ifdef QC_PATH_COMP
static bool unify_record_failure = false;
static bool unify_all_failures = false;

unification_failure *failure = 0;
list_int *unify_path_rev = 0;
list<unification_failure *> failures;
#endif

bool dags_compatible(dag_node *dag1, dag_node *dag2)
{
  bool res = true;
  
  unification_cost = 0;

  if(dag_unify1(dag1, dag2) == FAIL)
    res = false;

#ifdef STRICT_LKB_COMPATIBILITY
  else
    res = !dag_cyclic_rec(dag1);
#endif

  dag_invalidate_changes();

  return res;
}

#ifdef QC_PATH_COMP
//
// recording of failures
//

void clear_failure()
{
  if(failure)
    {
      delete failure;
      failure = 0;
    }
}

void save_or_clear_failure()
{
  if(unify_all_failures)
    {
      if(failure)
	{
	  failures.push_back(failure);
	  failure = 0;
	}
    }
  else
    clear_failure();
}
#endif

dag_node *dag_cyclic_copy(dag_node *src, list_int *del);

#ifdef QC_PATH_COMP
list<unification_failure *> dag_unify_get_failures(dag_node *dag1, dag_node *dag2, bool all_failures,
						   list_int *initial_path, dag_node **result_root)
{
  unify_record_failure = true;
  unify_all_failures = all_failures;

  clear_failure();
  failures.clear();
  unification_cost = 0;

  if(unify_path_rev != 0) fprintf(ferr, "dag_unify_get_failures: unify_path_rev not empty\n");
  unify_path_rev = reverse(initial_path);

  dag_unify1(dag1, dag2);

  if(failure)
    {
      failures.push_back(failure);
      failure = 0;
    }
  
#ifdef COMPLETE_FAILURE_REPORTING
  dag_node *cycle;
  if(result_root != 0)
    {
      *result_root = dag_cyclic_copy(*result_root, 0);
      
      // result might be cyclic
      free_list(unify_path_rev); unify_path_rev = 0;
      if((cycle = dag_cyclic_rec(*result_root)) != 0)
	{
	  dag_invalidate_changes();
	  failures.push_back(New unification_failure(unification_failure::CYCLE, unify_path_rev,
						unification_cost, -1, -1, cycle, *result_root));
	}
    }
  else
    {
      // result might be cyclic
      if((cycle = dag_cyclic_rec(dag1)) != 0)
	{
	  failures.push_back(New unification_failure(unification_failure::CYCLE, unify_path_rev,
						unification_cost));
	}
    }
#endif

  free_list(unify_path_rev); unify_path_rev = 0;

  dag_invalidate_changes();

  unify_record_failure = false;
  return failures; // caller is responsible for free'ing the paths
}

list<unification_failure *>
dag_subsumes_get_failures(dag_node *dag1, dag_node *dag2,
                          bool &forward, bool &backward,
                          bool all_failures)
{
    unify_record_failure = true;
    unify_all_failures = all_failures;
    
    clear_failure();
    failures.clear();
    unification_cost = 0;

    if(unify_path_rev != 0)
        fprintf(ferr, "dag_subsumes_get_failures: unify_path_rev not empty\n");

    unify_path_rev = 0;

    dag_subsumes1(dag1, dag2, forward, backward);

    if(failure)
    {
        failures.push_back(failure);
        failure = 0;
    }
  
    free_list(unify_path_rev); unify_path_rev = 0;

    dag_invalidate_changes();

    unify_record_failure = false;
    return failures; // caller is responsible for free'ing the paths
}

#endif

inline bool dag_has_arcs(dag_node *dag)
{
  return dag->arcs || dag_get_comp_arcs(dag);
}

dag_node *dag_unify1(dag_node *dag1, dag_node *dag2)
{
  unification_cost++;
  
  dag1 = dag_deref1(dag1);
  dag2 = dag_deref1(dag2);

  if(dag_get_copy(dag1) == INSIDE)
    {
      stats.cycles++;

#ifdef QC_PATH_COMP
#ifdef COMPLETE_FAILURE_REPORTING
      if(unify_record_failure)
        {
          // _fix_me_ this is not right
	  if(!unify_all_failures)
	    {
	      save_or_clear_failure();
	      failure = New unification_failure(unification_failure::CYCLE, unify_path_rev, unification_cost);
	      return FAIL;
	    }
	  else
	    return dag1; // continue with cyclic structure
        }
      else
#endif
#endif
	return FAIL;
    }

  if(dag1 != dag2)
    {
      if(dag_unify2(dag1, dag2) == FAIL)
	return FAIL;
    }
  
  return dag1;
}

//
// constraint stuff
//

dag_node *new_p_dag(int s)
{
  dag_node *dag = dag_alloc_p_node();
  dag_init(dag, s);

#ifdef MARK_PERMANENT
  dag->permanent = true;
#endif

  return dag;
}

dag_arc *new_p_arc(int attr, dag_node *val)
{
  dag_arc *newarc = dag_alloc_p_arc();
  newarc->attr = attr;
  newarc->val = val;
  newarc->next = 0;
  return newarc;
}

dag_node *dag_full_p_copy(dag_node *dag)
{
  dag_node *copy;
  copy = dag_get_copy(dag);
  
  if(copy == 0)
    {
      dag_arc *arc;
      copy = new_p_dag(dag->type);
      dag_set_copy(dag, copy);

      arc = dag->arcs;
      while(arc != 0)
        {
          dag_add_arc(copy, new_p_arc(arc->attr, dag_full_p_copy(arc->val)));
          arc = arc->next;
        }
    }

  return copy;
}

int total_cached_constraints = 0;

inline dag_node *fresh_constraint_of(int s)
{
  temporary_generation save(++unify_generation_max);
  return dag_full_copy(typedag[s]);
}

constraint_info **constraint_cache;

inline constraint_info *fresh_p_constraint(int s, constraint_info *next)
{
  constraint_info *c = New constraint_info;
  c -> next = next;
  c -> gen = 0;

  temporary_generation save(++unify_generation_max);
  c -> dag = dag_full_p_copy(typedag[s]);

  return c;
}

struct dag_node *cached_constraint_of(int s)
{
  constraint_info *c = constraint_cache[s];

  while(c && c->gen == unify_generation)
    c = c->next;

  if(c == 0)
    {
      total_cached_constraints++;
      c = constraint_cache[s] = fresh_p_constraint(s, constraint_cache[s]);
    }

  c->gen = unify_generation;

  return c->dag;
}

inline bool
dag_make_wellformed(int new_type, dag_node *dag1, int s1, dag_node *dag2,
                    int s2)
{
    if((s1 == new_type && s2 == new_type) ||
       (!dag_has_arcs(dag1) && !dag_has_arcs(dag2)) ||
       (s1 == new_type && dag_has_arcs(dag1)) ||
       (s2 == new_type && dag_has_arcs(dag2)))
        return true;

    bool res = true;
    if(typedag[new_type]->arcs)
    {
        bool sv = unify_record_failure;
        unify_record_failure = false;
        res = dag_unify1(dag1, cached_constraint_of(new_type)) != FAIL;
        unify_record_failure = sv;
    }

    return res;
}

inline dag_arc *dag_cons_arc(int attr, dag_node *val, dag_arc *next)
{
  dag_arc *arc;

  arc = new_arc(attr, val);
  arc -> next = next;
  
  return arc;
}

inline dag_node *find_attr2(int attr, dag_arc *a, dag_arc *b)
{
  dag_arc *res;
  
  if((res = dag_find_attr(a, attr)) != 0)
    return res -> val;
  if((res = dag_find_attr(b, attr)) != 0)
    return res -> val;
  
  return 0;
}

inline bool unify_arcs1(dag_arc *arcs, dag_arc *arcs1, dag_arc *comp_arcs1, dag_arc **new_arcs1)
{
  dag_node *n;
  
  while(arcs != 0)
    {
      n = find_attr2(arcs->attr, arcs1, comp_arcs1);
      if(n != 0)
	{
#ifdef QC_PATH_COMP
	  if(unify_record_failure) unify_path_rev = cons(arcs->attr, unify_path_rev);
#endif
	  if(dag_unify1(n, arcs->val) == FAIL)
	    {
#ifdef QC_PATH_COMP
	      if(unify_record_failure)
		{
		  unify_path_rev = pop_rest(unify_path_rev);
		  if(!unify_all_failures) return false;
		}
	      else
#endif
		return false;
	    }
#ifdef QC_PATH_COMP
	  if(unify_record_failure) unify_path_rev = pop_rest(unify_path_rev);
#endif
	}
      else
	{
	  *new_arcs1 = dag_cons_arc(arcs->attr, arcs->val, *new_arcs1);
	}

      arcs = arcs->next;
    }
  return true;
}

inline bool dag_unify_arcs(dag_node *dag1, dag_node *dag2)
{
  dag_arc *arcs1, *comp_arcs1, *new_arcs1;

  arcs1 = dag1->arcs;
  new_arcs1 = comp_arcs1 = dag_get_comp_arcs(dag1);

  if(!unify_arcs1(dag2->arcs, arcs1, comp_arcs1, &new_arcs1))
    return false;
  
  if(!unify_arcs1(dag_get_comp_arcs(dag2), arcs1, comp_arcs1, &new_arcs1))
    return false;
  
  if(new_arcs1 != 0)
    {
      dag_set_comp_arcs(dag1, new_arcs1);
    }

  return true;
}

dag_node *dag_unify2(dag_node *dag1, dag_node *dag2)
{
  int s1, s2, new_type;

  new_type = glb((s1 = dag_get_new_type(dag1)), (s2 = dag_get_new_type(dag2)));

  if(new_type == -1)
    {
#ifdef QC_PATH_COMP
      if(unify_record_failure)
        { 
	  save_or_clear_failure();
	  failure = New unification_failure(unification_failure::CLASH, unify_path_rev, unification_cost, s1, s2);

	  if(!unify_all_failures)
	    return FAIL;
	  else
	    new_type = s1;
	}
      else
#endif
	return FAIL;
    }

#if 0

  if(verbosity > 4)
    {
      if(new_type != s1 || new_type != s2)
        {
          if((dag_has_arcs(dag1) && featset[s1] != featset[new_type]) || (dag_has_arcs(dag2) && featset[s2] != featset[new_type]))
            {
              if((dag_has_arcs(dag1) && featset[s1] == featset[new_type]) || (dag_has_arcs(dag2) && featset[s2] == featset[new_type]))
                fprintf(ferr, "glb: one compatible set\n");
              else
                fprintf(ferr, "glb: %s%s(%d) & %s%s(%d) -> %s(%d)\n",
                        typenames[s1], dag_has_arcs(dag1) ? "[]" : "", featset[s1],
                        typenames[s2], dag_has_arcs(dag2) ? "[]" : "", featset[s2],
                        typenames[new_type], featset[new_type]);
            }
          else
            fprintf(ferr, "glb: compatible feature sets\n");
        }
      else
        {
          fprintf(ferr, "glb: type unchanged\n");
        }
    }

#endif

  // _fix_me_ maybe check if actually changed 
  dag_set_new_type(dag1, new_type);

  if(unify_wellformed)
    {
      if(!dag_make_wellformed(new_type, dag1, s1, dag2, s2))
	{
#ifdef QC_PATH_COMP
#ifdef COMPLETE_FAILURE_REPORTING
	  if(unify_record_failure)
            { 
	      save_or_clear_failure();
	      failure = New unification_failure(unification_failure::CONSTRAINT, unify_path_rev, unification_cost, s1, s2);

	      if(!unify_all_failures)
		return FAIL;
            }
	  else
#endif
#endif
	    return FAIL;
	}

      dag1 = dag_deref1(dag1);
    }

  if(!dag_has_arcs(dag2)) // try the cheapest (?) solution first
    {
      dag_set_forward(dag2, dag1);
    }
  else if(!dag_has_arcs(dag1))
    {
      dag_set_new_type(dag2, new_type);
      dag_set_forward(dag1, dag2);
    }
  else
    {
      dag_set_copy(dag1, INSIDE);
      dag_set_forward(dag2, dag1);

      if(!dag_unify_arcs(dag1, dag2))
	{
	  dag_set_copy(dag1, 0);
	  return FAIL;
	}

      dag_set_copy(dag1, 0);
    }

  return dag1;
}

dag_node *dag_full_copy(dag_node *dag)
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
          dag_add_arc(copy, new_arc(arc->attr, dag_full_copy(arc->val)));
          arc = arc->next;
        }
    }

  return copy;
}

struct dag_node *
dag_partial_copy1(dag_node *dag, int attr, list_int *del)
{
    dag_node *copy;
    
    copy = dag_get_copy(dag);
    
    if(copy == 0)
    {
        dag_arc *arc;
        
        copy = new_dag(dag->type);
        
        dag_set_copy(dag, copy);
        
        if(!contains(del, attr))
        {
            arc = dag->arcs;
            while(arc != 0)
	    {
                dag_add_arc(copy,
                            new_arc(arc->attr,dag_partial_copy1(arc->val,
                                                                arc->attr,
                                                                del)));
                arc = arc->next;
	    }
	}
        else if(attr != -1)
        {
            copy->type = maxapp[attr];
        }
    }
    
    return copy;
}

struct dag_node *dag_partial_copy(dag_node *dag, list_int *del)
{
  return dag_partial_copy1(dag, -1, del);
}

void
dag_subsumes(dag_node *dag1, dag_node *dag2, bool &forward, bool &backward)
{
    unification_cost = 0;
    dag_subsumes1(dag1, dag2, forward, backward);
    dag_invalidate_changes();
}

bool
dag_subsumes1(dag_node *dag1, dag_node *dag2, bool &forward, bool &backward)
{
    unification_cost++;

    dag_node *c1 = dag_get_forward(dag1),
             *c2 = dag_get_copy(dag2);
    
    if(forward)
    {
        if(c1 == 0)
        {
            dag_set_forward(dag1, dag2);
        }
        else if(c1 != dag2)
        {
            forward = false;
#ifdef QC_PATH_COMP
#ifdef COMPLETE_FAILURE_REPORTING
            if(unify_record_failure)
            {
                save_or_clear_failure();
                failure =new unification_failure(unification_failure::COREF,
                                                 unify_path_rev,
                                                 1);
            }
#endif
#endif
        }
    }
    
    if(backward)
    {
        if(c2 == 0)
        {
            dag_set_copy(dag2, dag1);
        }
        else if(c2 != dag1)
        {
            backward = false;
#ifdef QC_PATH_COMP
#ifdef COMPLETE_FAILURE_REPORTING
            if(unify_record_failure)
            {
                save_or_clear_failure();
                failure =new unification_failure(unification_failure::COREF,
                                                 unify_path_rev,
                                                 1);
            }
#endif
#endif
        }
    }
    
    if(forward == false && backward == false && !unify_all_failures)
    {
        return false;
    }
    
    if(dag1->type != dag2->type)
    { 
        bool st_12, st_21;
        subtype_bidir(dag1->type, dag2->type, st_12, st_21);
        if(!st_12)
        {
            backward = false;
#ifdef QC_PATH_COMP
            if(unify_record_failure)
            {
                save_or_clear_failure();
                failure = new unification_failure(unification_failure::CLASH,
                                                  unify_path_rev,
                                                  1,
                                                  dag2->type, dag1->type);
            }
#endif
        }
        if(!st_21)
        {
            forward = false;
#ifdef QC_PATH_COMP
            if(unify_record_failure)
            {
                save_or_clear_failure();
                failure = new unification_failure(unification_failure::CLASH,
                                                  unify_path_rev,
                                                  1,
                                                  dag1->type, dag2->type);
            }
#endif
        }

        if(forward == false && backward == false && !unify_all_failures)
        {
            return false;
        }
    }
    
    if(dag1->arcs || dag2->arcs)
    {
        // Partial expansion makes this slightly more complicated. We
        // need to visit a structure even when the corresponding part
        // has been unfilled so that we find all coreferences.
        dag_node *tmpdag1 = dag1, *tmpdag2 = dag2;
        
        if(!dag1->arcs && typedag[dag1->type]->arcs)
            tmpdag1 = cached_constraint_of(dag1->type);

        if(!dag2->arcs && typedag[dag2->type]->arcs)
            tmpdag2 = cached_constraint_of(dag2->type);

        dag_arc *arc1 = tmpdag1->arcs;
        
        while(arc1)
	{
            dag_arc *arc2 = dag_find_attr(tmpdag2->arcs, arc1->attr);
            
            if(arc2)
	    {
#ifdef QC_PATH_COMP
                if(unify_record_failure)
                    unify_path_rev = cons(arc1->attr, unify_path_rev);
#endif
                if(!dag_subsumes1(arc1->val, arc2->val, forward, backward))
                {
#ifdef QC_PATH_COMP
                    // We will not get here when unify_all_failures is on,
                    // since we never return false. So we can always return
                    // from here, unconditionally.
                    
                    if(unify_record_failure)
                        unify_path_rev = pop_rest(unify_path_rev);
                    return false;
#endif
                }
#ifdef QC_PATH_COMP
                if(unify_record_failure)
                    unify_path_rev = pop_rest(unify_path_rev);
#endif
	    }
            
            arc1 = arc1->next;
	}
    }

    return true;
}

#ifdef QC_PATH_COMP

static list<list_int *> paths_found;

void dag_paths_rec(dag_node *dag, dag_node *search)
{
  dag_node *copy;

  if(dag == search)
    paths_found.push_back(reverse(unify_path_rev));

  copy = dag_get_copy(dag);
  if(copy == 0)
    {
      dag_arc *arc;

      dag_set_copy(dag, INSIDE);

      arc = dag->arcs;
      while(arc != 0)
        {
	  unify_path_rev = cons(arc->attr, unify_path_rev);
	  dag_paths_rec(arc->val, search);
	  unify_path_rev = pop_rest(unify_path_rev);
          arc = arc->next;
        }
    }
}

list<list_int *> dag_paths(dag_node *dag, dag_node *search)
{
  paths_found.clear();

  if(unify_path_rev != 0)
    {
      free_list(unify_path_rev); unify_path_rev = 0;
    }

  dag_paths_rec(dag, search);

  dag_invalidate_changes();
  return paths_found; // caller is responsible for free'ing paths
}

#endif

/* like naive dag_copy, but copies cycles */

dag_node *dag_cyclic_copy(dag_node *src, list_int *del)
{
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
  while(arc)
    {
      if(!contains(del, arc->attr))
	{
	  if((v = dag_cyclic_copy(arc->val, 0)) == FAIL)
	    return FAIL;
	  
	  new_arcs = dag_cons_arc(arc->attr, v, new_arcs);
	}
      arc = arc->next;
    }

  arc = dag_get_comp_arcs(src);
  while(arc)
    {
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

#undef CYCLE_PARANOIA
#ifdef SMART_COPYING

/* (almost) non-redundant copying (Tomabechi/Malouf/Caroll) from the LKB */

#if 0

dag_node *dag_copy(dag_node *src, list_int *del)
{
  dag_node *copy;
  int type;

  unification_cost++;

  src = dag_deref1(src);
  copy = dag_get_copy(src);

  if(copy == INSIDE)
    {
      stats.cycles++;
      return FAIL;
    }
  else if(copy == FAIL)
    {
      copy = 0;
      //      fprintf(ferr, "reset copy @ 0x%x\n", (int) src);
    }
  
  if(copy != 0)
    return copy;

#ifdef CYCLE_PARANOIA
  if(del)
    {
      if(dag_cyclic_rec(src))
	return FAIL;
    }
#endif

  type = dag_get_new_type(src);

  dag_arc *new_arcs = dag_get_comp_arcs(src);

  // `atomic' node
  if(src->arcs == 0 && new_arcs == 0)
    {
      if(dag_permanent(src) || type != src->type)
	copy = new_dag(type);
      else
	copy = src;

      dag_set_copy(src, copy);

      return copy;
    }

  dag_set_copy(src, INSIDE);
  
  bool copy_p = dag_permanent(src) || type != src->type || new_arcs != 0;

  dag_arc *arc;

  // copy comp-arcs - top level arcs can be reused

  arc = new_arcs;
  while(arc)
    {
      if(!contains(del, arc->attr))
	{
	  if((arc->val = dag_copy(arc->val, 0)) == FAIL)
	    return FAIL;
	}
      else
	arc->val = 0; // mark for deletion

      arc = arc->next;
    }
  
  // really remove `deleted' arcs
  
  // handle special case of first element
  while(new_arcs && new_arcs->val == 0)
    new_arcs = new_arcs->next;

  // non-first elements
  arc = new_arcs;
  while(arc)
    {
      if(arc->next && arc->next->val == 0)
	arc->next = arc->next->next;
      else
        arc = arc->next;
    }

#ifndef NAIVE_MEMORY
  dag_alloc_state old_arcs;
  dag_alloc_mark(old_arcs);
#endif  

  dag_node *v;

  // copy arcs - could reuse unchanged tail here - too hairy for now

  arc = src->arcs;
  while(arc)
    {
      if(!contains(del, arc->attr))
	{
	  if((v = dag_copy(arc->val, 0)) == FAIL)
	    return FAIL;
	  
	  if(arc->val != v) 
	    copy_p = true;
	  
	  new_arcs = dag_cons_arc(arc->attr, v, new_arcs);
	}
      arc = arc->next;
    }

  if(copy_p)
    {
      copy = new_dag(type);
      copy->arcs = new_arcs;
    }
  else
    {
      copy = src;
#ifndef NAIVE_MEMORY
      dag_alloc_release(old_arcs);
#endif
    }

  dag_set_copy(src, copy);

  return copy;
}

#endif

inline bool arcs_contain(dag_arc *arc, int attr)
{
  while(arc)
    {
      if(arc->attr == attr) return true;
      arc = arc->next;
    }
  return false;
}

inline dag_arc *clone_arcs_del(dag_arc *src, dag_arc *dst, dag_arc *del)
{
  while(src)
    {
      if(!arcs_contain(del, src->attr))
	dst = dag_cons_arc(src->attr, src->val, dst);
      src = src->next;
    }

  return dst;
}

inline dag_arc *clone_arcs_del_del(dag_arc *src, dag_arc *dst, dag_arc *del_arcs, list_int *del_attrs)
{
  while(src)
    {
      if(!contains(del_attrs,src->attr) && !arcs_contain(del_arcs, src->attr))
	dst = dag_cons_arc(src->attr, src->val, dst);
      src = src->next;
    }

  return dst;
}

dag_node *dag_copy(dag_node *src, list_int *del)
{
  dag_node *copy;
  int type;

  unification_cost++;

  src = dag_deref1(src);
  copy = dag_get_copy(src);

  if(copy == INSIDE)
    {
      stats.cycles++;
      return FAIL;
    }
  else if(copy == FAIL)
    {
      copy = 0;
    }
  
  if(copy != 0)
    return copy;

  type = dag_get_new_type(src);

  dag_arc *new_arcs = dag_get_comp_arcs(src);

  // `atomic' node
  if(src->arcs == 0 && new_arcs == 0)
    {
      if(dag_permanent(src) || type != src->type)
	copy = new_dag(type);
      else
	copy = src;

      dag_set_copy(src, copy);

      return copy;
    }

  dag_set_copy(src, INSIDE);
  
  bool copy_p = dag_permanent(src) || type != src->type || new_arcs != 0;

  dag_arc *arc;

  // copy comp-arcs - top level arcs can be reused

  arc = new_arcs;
  while(arc)
    {
      if(!contains(del, arc->attr))
	{
	  if((arc->val = dag_copy(arc->val, 0)) == FAIL)
	    return FAIL;
	}
      else
	arc->val = 0; // mark for deletion

      arc = arc->next;
    }
  
  // really remove `deleted' arcs
  
  // handle special case of first element
  while(new_arcs && new_arcs->val == 0)
    new_arcs = new_arcs->next;

  // non-first elements
  arc = new_arcs;
  while(arc)
    {
      if(arc->next && arc->next->val == 0)
	arc->next = arc->next->next;
      else
        arc = arc->next;
    }

  dag_node *v;

  // copy arcs - could reuse unchanged tail here - too hairy for now

  arc = src->arcs;
  while(arc)
    {
      if(!contains(del, arc->attr))
	{
	  if((v = dag_copy(arc->val, 0)) == FAIL)
	    return FAIL;
	  
	  if(arc->val != v)
	    {
	      copy_p = true;
	      new_arcs = dag_cons_arc(arc->attr, v, new_arcs);
	    }
	}
      arc = arc->next;
    }

  if(copy_p)
    {
      copy = new_dag(type);
      if(del)
	copy->arcs = clone_arcs_del_del(src->arcs, new_arcs, new_arcs, del);
      else
	copy->arcs = clone_arcs_del(src->arcs, new_arcs, new_arcs);
    }
  else
    {
      copy = src;
    }

  dag_set_copy(src, copy);

  return copy;
}

#else

/* plain vanilla copying (as in the paper) */

dag_node *dag_copy(dag_node *src, list_int *del)
{
  dag_node *copy;

  unification_cost++;

  src = dag_deref1(src);
  copy = dag_get_copy(src);

  if(copy == INSIDE)
    {
      stats.cycles++;
      return FAIL;
    }
  
  if(copy != 0)
    return copy;

  dag_set_copy(src, INSIDE);
  
  dag_arc *new_arcs = 0;
  dag_arc *arc;
  dag_node *v;
  
  arc = src->arcs;
  while(arc)
    {
      if(!contains(del, arc->attr))
	{
	  if((v = dag_copy(arc->val, 0)) == FAIL)
	    return FAIL;
	  
	  new_arcs = dag_cons_arc(arc->attr, v, new_arcs);
	}
      arc = arc->next;
    }

  arc = dag_get_comp_arcs(src);
  while(arc)
    {
      if(!contains(del, arc->attr))
	{
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
// non-permanent dags
//

dag_node *dag_get_attr_value_np(dag_node *dag, int attr)
{
  dag_arc *arc;

  arc = dag_find_attr(dag_get_comp_arcs(dag), attr);
  if(arc) return arc->val;

  arc = dag_find_attr(dag->arcs, attr);
  if(arc) return arc->val;
  
  return FAIL;
}

struct dag_node *dag_nth_arg_np(struct dag_node *dag, int n)
{
  int i;
  struct dag_node *arg;

  if((arg = dag_get_attr_value_np(dag_deref1(dag), BIA_ARGS)) == FAIL)
    return FAIL;

  for(i = 1; i < n && arg && arg != FAIL && !subtype(dag_get_new_type(arg), BI_NIL); i++)
    arg = dag_get_attr_value_np(dag_deref1(arg), BIA_REST);

  if(i != n)
    return FAIL;

  arg = dag_get_attr_value_np(dag_deref1(arg), BIA_FIRST);

  return arg;
}

void dag_get_qc_vector_np(qc_node *path, struct dag_node *dag, type_t *qc_vector)
{
  if(dag == FAIL) return;

  dag = dag_deref1(dag);

  if(path->qc_pos > 0)
    qc_vector[path->qc_pos - 1] = dag_get_new_type(dag);

  // _fix_me_
  //  if(dag->arcs == 0 && dag_get_comp_arcs(dag) == 0)
  //    return;

  for(qc_arc *arc = path->arcs; arc != 0; arc = arc->next)
    dag_get_qc_vector_np(arc->val,
			 dag_get_attr_value_np(dag, arc->attr),
			 qc_vector);
}


/* safe printing of dags - this doesn't modify the dags, and can print
   temporary structures */

map<dag_node *, int> dags_visited;

int dag_get_visit_safe(struct dag_node *dag)
{
  return dags_visited[dag];
}

int dag_set_visit_safe(struct dag_node *dag, int visit)
{
  return dags_visited[dag] = visit;
}

static int coref_nr = 0;

void dag_mark_coreferences_safe(struct dag_node *dag, bool np)
// recursively set `visit' field of dag nodes to number of occurences
// in this dag - any nodes with more than one occurence are `coreferenced'
{
  if(np) dag = dag_deref1(dag);

  if(dag_set_visit_safe(dag, dag_get_visit_safe(dag) + 1) == 1)
    { // not yet visited
      dag_arc *arc = dag->arcs;
      while(arc != 0)
	{
	  dag_mark_coreferences_safe(arc->val, np);
	  arc = arc->next;
	}
      
      if(np)
	{
	  arc = dag_get_comp_arcs(dag);
	  while(arc != 0)
	    {
	      dag_mark_coreferences_safe(arc->val, np);
	      arc = arc->next;
	    }
	}
    }
}

void dag_print_rec_safe(FILE *f, struct dag_node *dag, int indent, bool np);

void dag_print_arcs_safe(FILE *f, dag_arc *arc, int indent, bool np)
{
  if(arc == 0) return;

  struct dag_node **print_attrs = (struct dag_node **)
    malloc(sizeof(struct dag_node *) * nattrs);

  int maxlen = 0, i, maxatt = 0;

  for(i = 0; i < nattrs; i++)
    print_attrs[i] = 0;
  
  while(arc)
    {
      i = attrnamelen[arc->attr];
      maxlen = maxlen > i ? maxlen : i;
      print_attrs[arc->attr]=arc->val;
      maxatt = arc->attr > maxatt ? arc->attr : maxatt;
      arc=arc->next;
    }

  fprintf(f, "\n%*s[ ", indent, "");
  
  bool first = true;
  for(int j = 0; j <= maxatt; j++) if(print_attrs[j])
    {
      i = attrnamelen[j];
      if(!first)
	fprintf(f, ",\n%*s",indent + 2,"");
      else
	first = false;
      
      fprintf(f, "%s%*s", attrname[j], maxlen-i+1, "");
      dag_print_rec_safe(f, print_attrs[j], indent + 2 + maxlen + 1, np);
    }
  
  fprintf(f, " ]");
  free(print_attrs);
}

void dag_print_rec_safe(FILE *f, struct dag_node *dag, int indent, bool np)
// recursively print dag. requires `visit' field to be set up by
// mark_coreferences. negative value in `visit' field means that node
// is coreferenced, and already printed
{
  int coref;

  dag_node *orig = dag;
  if(np)
    {
      dag = dag_deref1(dag);
      if(dag != orig) fprintf(f, "~");
    }

  coref = dag_get_visit_safe(dag) - 1;
  if(coref < 0) // dag is coreferenced, already printed
    {
      fprintf(f, "#%d", -(coref+1));
      return;
    }
  else if(coref > 0) // dag is coreferenced, not printed yet
    {
      coref = -dag_set_visit_safe(dag, -(coref_nr++));
      indent += fprintf(f, "#%d:", coref);
    }

  fprintf(f, "%s(", typenames[dag->type]);
  if(dag != orig)
    fprintf(f, "%x->", (int) orig);
  fprintf(f, "%x)", (int) dag);

  dag_print_arcs_safe(f, dag->arcs, indent, np);
  if(np) dag_print_arcs_safe(f, dag_get_comp_arcs(dag), indent, np);
}

void dag_print_safe(FILE *f, struct dag_node *dag, bool np)
{
  if(dag == 0)
    {
      fprintf(f, "%s", typenames[0]);
      return;
    }
  if(dag == FAIL)
    {
      fprintf(f, "fail");
      return;
    }

  dag_mark_coreferences_safe(dag, np);
  
  coref_nr = 1;
  dag_print_rec_safe(f, dag, 0, np);
  dags_visited.clear();
}

dag_node *dag_cyclic_arcs(dag_arc *arc)
{
  dag_node *v;

  while(arc)
    {
#ifdef QC_PATH_COMP
      if(unify_record_failure)
	unify_path_rev = cons(arc->attr, unify_path_rev);
#endif	  

      if((v = dag_cyclic_rec(arc->val)) != 0)
	return v;

#ifdef QC_PATH_COMP
      if(unify_record_failure)
	unify_path_rev = pop_rest(unify_path_rev);
#endif      

      arc = arc->next;
    }

  return 0;
}

// this shall also work on temporary structures
dag_node *dag_cyclic_rec(dag_node *dag)
{
  dag = dag_deref1(dag);

  dag_node *v = dag_get_copy(dag);

  if(v == 0) // not yet seen
    {
      unification_cost++;
      
      dag_set_copy(dag, INSIDE);

      if((v = dag_cyclic_arcs(dag->arcs)) != 0 || (v = dag_cyclic_arcs(dag_get_comp_arcs(dag))) != 0)
	return v;
      
      dag_set_copy(dag, FAIL);
    }
  else if(v == INSIDE) // cycle found
    {
      return dag;
    }

  return 0;
}

bool dag_cyclic(dag_node *dag)
{
  bool res;

  res = dag_cyclic_rec(dag);
  dag_invalidate_changes();
  return res;
}

//
// de-shrinking: make fs totally wellformed
//

map<int, bool> node_expanded;

dag_node *dag_expand_rec(dag_node *dag);

bool dag_expand_arcs(dag_arc *arcs)
{
  while(arcs)
    {
      if(dag_expand_rec(arcs->val) == FAIL)
	return false;

      arcs = arcs->next;
    }

  return true;
}

dag_node *dag_expand_rec(dag_node *dag)
{
  dag = dag_deref1(dag);

  if(node_expanded[(int) dag])
    return dag;

  node_expanded[(int) dag] = true;

  int new_type = dag_get_new_type(dag);

  if(typedag[new_type]->arcs && typedag[new_type]->type == new_type)
    if(dag_unify1(dag, cached_constraint_of(new_type)) == FAIL)
      {
	fprintf(ferr, "expansion failed @ 0x%x for `%s'\n",
		(int) dag, typenames[new_type]);
	return FAIL;
      }

  dag = dag_deref1(dag);
  
  if(!dag_expand_arcs(dag->arcs))
    return FAIL;
  
  if(!dag_expand_arcs(dag_get_comp_arcs(dag)))
    return FAIL;

  return dag;
}

dag_node *dag_expand(dag_node *dag)
{
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

bool dag_valid_rec(dag_node *dag)
{
  if(dag == 0 || dag == INSIDE || dag == FAIL)
    {
      fprintf(ferr, "(1) dag is 0x%x\n", (int) dag);
      return false;
    }

  dag = dag_deref1(dag);

  if(dag == 0 || dag == INSIDE || dag == FAIL)
    {
      fprintf(ferr, "(2) dag is 0x%x\n", (int) dag);
      return false;
    }

  dag_node *v = dag_get_copy(dag);

  if(v == 0) // not yet seen
    {
      dag_arc *arc = dag->arcs;

      dag_set_copy(dag, INSIDE);
      
      while(arc)
	{
	  if(arc->attr > nattrs)
	    {
	      fprintf(ferr, "(3) invalid attr: %d, val: 0x%x\n",
		      arc->attr, (int) arc->val);
	      return false;
	    }

	  if(dag_valid_rec(arc->val) == false)
	    {
	      fprintf(ferr, "(4) invalid value under %s\n",
		      attrname[arc->attr]);
	      return false;
	    }

	  arc = arc->next;
	}

      dag_set_copy(dag, FAIL);
    }
  else if(v == INSIDE) // cycle found
    {
      fprintf(ferr, "(5) invalid dag: cyclic\n");
      return false;
    }

  return true;
}

bool dag_valid(dag_node *dag)
{
  bool res;
  res = dag_valid_rec(dag);
  dag_invalidate_changes();
  return res;
}

void dag_initialize()
{
  // nothing to do
}

void dag_print_rec_fed_safe(FILE *f, struct dag_node *dag) {
// recursively print dag. requires `visit' field to be set up by
// mark_coreferences. negative value in `visit' field means that node
// is coreferenced, and already printed
  dag_arc *arc = dag->arcs ;
  int coref = dag_get_visit_safe(dag) - 1;

  if(coref < 0) { // dag is coreferenced, already printed
    fprintf(f, " #%d", -(coref+1)) ;
    return;
  }
  else if(coref > 0) { // dag is coreferenced, not printed yet
    coref = -dag_set_visit_safe(dag, -(coref_nr++));
    fprintf(f, " #%d=", coref );
  }

  fprintf(f, " [ (%%TYPE %s #T )", typenames[dag->type]);

  while(arc) {
    fprintf(f, " (%s", attrname[arc->attr]) ;
    dag_print_rec_fed_safe(f, arc->val) ;
    fprintf(f, " )");
    arc=arc->next;
  }
}

void dag_print_fed_safe(FILE *f, struct dag_node *dag) {
  if(dag == 0) {
    fprintf(f, "NIL") ;
    return;
  }
  if(dag == FAIL) {
    fprintf(f, "FAIL") ;
    return;
  }

  dag_mark_coreferences_safe(dag, 0);
  
  coref_nr = 1;
  dag_print_rec_fed_safe(f, dag);
  dags_visited.clear();
}
