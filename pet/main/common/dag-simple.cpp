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

/* An implementation of unify1 and unify2 of Wroblewski(1987) with
   changes for welltyped unification.
   Unify2 is buggy in the paper (for certain cases of convergent arcs),
   fixing it requires a third, directed mode of unification where only
   one of the input dags is modified. This resembles `into' unification
   a la Ciortuz(2000). This directed mode has to fall back to
   destructive unification for nodes copied previously within the same
   unification context - otherwise we'd have to dereference the copy slot
   (since we could get chains of copies) */

/* Main functionality provided to the outside is
   dag_unify1 (destructive unification),
   dag_unify2 (non-destructive unification) and
   dag_unify3 (semi-destructive unification) */

#include "pet-system.h"
#include "dag.h"
#include "dag-arced.h"
#include "types.h"

#ifdef FLOP
int visit_generation = 0;
#endif
int copy_generation = 0;
int copy_wf_generation = 0;

void dag_init(dag_node *dag, int s)
{
  dag->type = s;
  dag->forward = dag;
  dag->arcs = 0;
#ifdef FLOP
  dag->visit_generation = -1;
#endif
  dag->copy_generation = -1;
}

struct dag_node *current_base = 0;

inline bool dag_current(dag_node *dag)
{
  // fixme: this doesn't work unless mmap is available
  return dag >= current_base && dag <= p_alloc.current_base();
}

int dag_type(dag_node *dag)
{
  dag = dag_deref(dag);
  return dag->type;
}

void dag_set_type(dag_node *dag, int s)
{
  dag = dag_deref(dag);
  dag -> type = s;
}

dag_node *dag_get_attr_value(dag_node *dag, int attr)
{
  dag_arc *arc;

  if(dag == FAIL)
	  return FAIL;
  dag = dag_deref(dag);
  
  arc = dag_find_attr(dag->arcs, attr);
  if(arc == 0) return FAIL;
  
  return arc->val;
}

struct dag_node *make_nth_arg(int n, dag_node *v)
// make v the n th argument of new dag, return this dag
{
  assert(n >= 1);

  dag_node *res, *t1, *t2;
  res = new_dag(BI_TOP);
  t1 = new_dag(BI_TOP);
  dag_add_arc(res, new_arc(BIA_ARGS, t1)); 
  
  for(int i = 1; i < n ; i++)
    {
      t2 = new_dag(BI_TOP);
      dag_add_arc(t1, new_arc(BIA_REST, t2));
      t1 = t2;
    }

  dag_add_arc(t1, new_arc(BIA_FIRST, v));

  return res;
}

dag_node * dag_unify3_rec(dag_node *dag1, dag_node *dag2, int generation);

#ifdef FLOP
bool unify_reset_visited = false;
#endif

bool dag_cyclic_rec(dag_node *dag);

bool dag_make_wellformed3(int new_type, dag_node *dag1, dag_node *dag2)
{
  if((dag1->type == new_type && dag2->type == new_type) ||
     (!dag1->arcs && !dag2->arcs) ||
     (dag1->type == new_type && dag1->arcs) ||
     (dag2->type == new_type && dag2->arcs))
    // well-typedness already enforced
    return true;

  if(type_dag(new_type))
    {
      dag2->type = new_type;
      
      if(dag_unify3_rec(type_dag(new_type), dag2, ++copy_wf_generation) == FAIL)
	return false;
    }
  else
    dag2->type = new_type;

  return true;
}

dag_node *dag_unify1_rec(dag_node *dag1, dag_node *dag2);

dag_node *dag_unify1(dag_node *dag1, dag_node *dag2)
{
  dag_node *res;

  unification_cost = 0;
  current_base = (dag_node *) t_alloc.current();
  res = dag_unify1_rec(dag1, dag2);
  dag_invalidate_copy();
  return res;
}

dag_node *dag_unify1_rec(dag_node *dag1, dag_node *dag2)
{
  int new_type;
  dag_arc *arc1, *arc2;

  unification_cost++;

  dag1 = dag_deref(dag1); dag2 = dag_deref(dag2);

  assert(dag2 != NULL);

  if(dag1 == dag2) return dag2;

  if((new_type = glb(dag1->type,dag2->type)) == -1)
    return FAIL;

#ifdef FLOP
  if(unify_reset_visited) 
    {
      dag_set_visit(dag1, 0);
      dag_set_visit(dag2, 0);
    }
#endif

  assert(dag2 != NULL);

  dag1->forward = dag2;
  
  if(unify_wellformed)
    {
      if(!dag_make_wellformed3(new_type, dag1, dag2))
	return FAIL;
    }

  dag2->type = new_type;

  arc1=dag1->arcs;
  while(arc1 != 0)
    {
      if((arc2=dag_find_attr(dag2->arcs,arc1->attr)))
	{
	  if(dag_unify1_rec(arc1->val, arc2->val) == FAIL)
	    return FAIL;
	}
      else
	dag_add_arc(dag2, new_arc(arc1->attr, arc1->val));


      arc1 = arc1->next;
    }

  return dag2;
}

dag_node *dag_copy_rec(dag_node *src, int generation)
{
  dag_node *copy;
  dag_arc *arc;

  unification_cost++;

  src = dag_deref(src);

  copy = dag_get_copy(src, generation);
  if(copy == 0)
    {
      copy = new_dag(src->type);
      
      dag_set_copy(src, copy, generation);
      
      arc = src->arcs;
      while(arc != 0)
        {
          dag_add_arc(copy, new_arc(arc->attr, dag_copy_rec(arc->val, generation)));
          arc = arc->next;
        }
    }

  return copy;
}

dag_node *dag_copy(dag_node *src)
{
  dag_node *res;

  res = dag_copy_rec(src, copy_generation);
  dag_invalidate_copy();
  return res;
}

dag_node *dag_current_or_copy(dag_node *src, int generation)
{
  // _fix_me_ eigentlich rekursiv
  if(dag_current(src))
    return src;
  else
    return dag_copy_rec(src, generation);
}

dag_node *dag_unify2_rec(dag_node *dag1, dag_node *dag2);

dag_node *dag_make_wellformed2(int new_type, dag_node *dag1, dag_node *dag2)
{
  if((dag1->type == new_type && dag2->type == new_type) ||
     (!dag1->arcs && !dag2->arcs) ||
     (dag1->type == new_type && dag1->arcs) ||
     (dag2->type == new_type && dag2->arcs))
    return dag2;
  
  if(type_dag(new_type))
    {
      dag_node *c = dag_copy_rec(type_dag(new_type), ++copy_wf_generation);
      return dag_unify3_rec(dag2, c, copy_generation);
    }
  return dag2;
}

dag_node *dag_unify2_rec(dag_node *dag1, dag_node *dag2)
{
  int new_type;
  dag_arc *arc1, *arc2;
  dag_node *copy1, *copy2, *result, *wdag2;

  unification_cost++;

  dag1 = dag_deref(dag1);
  dag2 = dag_deref(dag2);

  copy1 = dag_get_copy(dag1, copy_generation);
  copy2 = dag_get_copy(dag2, copy_generation);

  if(copy1 == 0 && copy2 == 0)
    {
      assert(!dag_current(dag1));
      if(dag_current(dag2)) // because of wellformed unification
	return dag_unify3_rec(dag1, dag2, copy_generation);

      if((new_type = glb(dag1->type, dag2->type)) == -1)
	return FAIL;
      
      result = new_dag(new_type);
      
      if(unify_wellformed)
	{
	  wdag2 = dag_make_wellformed2(new_type, dag1, dag2);
	  if(wdag2 == FAIL)
	    return FAIL;
	}
      else
	wdag2 = dag2;

      dag_set_copy(dag1, result, copy_generation);
      dag_set_copy(dag2, result, copy_generation);
      
      arc1 = dag1->arcs;
      while(arc1 != 0)
        {
          if((arc2 = dag_find_attr(wdag2->arcs, arc1->attr)))
            {
              if((copy1 = dag_unify2_rec(arc1->val,arc2->val)) == FAIL )
                return FAIL;
              
              dag_add_arc(result, new_arc(arc1->attr, copy1));
            }
          else
            dag_add_arc(result, new_arc(arc1->attr, dag_current_or_copy(arc1->val, copy_generation)));

          arc1 = arc1->next;
        }

      arc2 = wdag2->arcs;
      while(arc2 != 0)
        {
          if(!dag_find_attr(dag1->arcs, arc2->attr))
            dag_add_arc(result, new_arc(arc2->attr, dag_current_or_copy(arc2->val, copy_generation)));
                    
          arc2 = arc2->next;
        }
    }
  else if(copy1 && copy2)
    {
      result = dag_unify1_rec(copy1, copy2);
    }
  else if(copy1)
    {
      result = dag_unify3_rec(dag2, copy1, copy_generation);
    }
  else
    {
      result = dag_unify3_rec(dag1, copy2, copy_generation);
    }

  return result;
}

dag_node *dag_unify2(dag_node *dag1, dag_node *dag2)
{
  dag_node *res;
  
  unification_cost = 0;
  current_base = (dag_node *) t_alloc.current();
  res = dag_unify2_rec(dag1, dag2);
  dag_invalidate_copy();
  return res;
}

dag_node * dag_unify3_rec(dag_node *dag1, dag_node *dag2, int generation)
{
  int new_type;
  dag_arc *arc1, *arc2;
  dag_node *copy;

  unification_cost++;

  dag1 = dag_deref(dag1);
  dag2 = dag_deref(dag2);

  if(dag1 == dag2) return dag2;

  copy = dag_get_copy(dag1, generation);

  if(copy == 0)
    {
      if(dag_current(dag1))
	return dag_unify1_rec(dag1, dag2);

      if((new_type = glb(dag1->type, dag2->type)) == T_BOTTOM)
	return FAIL;
      
      dag_set_copy(dag1, dag2, generation);

      if(unify_wellformed)
	{
	  if(!dag_make_wellformed3(new_type, dag1, dag2))
	    return FAIL;
	}

      dag2->type = new_type;

#ifdef FLOP
      if(unify_reset_visited) 
	dag_set_visit(dag2, 0);
#endif

      arc1=dag1->arcs;
      while(arc1 != 0)
	{
	  if((arc2=dag_find_attr(dag2->arcs,arc1->attr)))
	    {
	      if(dag_unify3_rec(arc1->val, arc2->val, generation) == FAIL)
		return FAIL;
	    }
	  else
	    dag_add_arc(dag2, new_arc(arc1->attr, dag_current_or_copy(arc1->val, generation)));
	  
	  arc1 = arc1->next;
	}
    }
  else
    return dag_unify1_rec(copy, dag2);

  return dag2;
}

dag_node *dag_unify3(dag_node *dag1, dag_node *dag2)
{
  dag_node *res;

  unification_cost = 0;
  current_base = (dag_node *) t_alloc.current();
  res = dag_unify3_rec(dag1, dag2, copy_generation);
  dag_invalidate_copy();
  return res;
}

bool dags_compatible_rec(dag_node *dag1, dag_node *dag2, int generation)
{
  dag_arc *arc1, *arc2;
  dag_node *frwrd;

  dag1 = dag_deref(dag1);
  dag2 = dag_deref(dag2);

  frwrd = dag_get_copy(dag1, generation);

  if(frwrd != 0) 
    dag1 = frwrd;

  if(dag1 == dag2) return true;

  if(glb(dag1->type, dag2->type) == -1)
    return false;

  dag_set_copy(dag1, dag2, generation);

  arc1=dag1->arcs;
  while(arc1 != 0)
    {
      if((arc2=dag_find_attr(dag2->arcs,arc1->attr)))
	{
	  if(dags_compatible_rec(arc1->val, arc2->val, generation) == false)
	    return false;
	}
      arc1 = arc1->next;
    }

  return true;
}

bool dags_compatible(dag_node *dag1, dag_node *dag2)
{
  bool res;

  res = dags_compatible_rec(dag1, dag2, copy_generation);
  dag_invalidate_copy();
  return res;
}

bool dag_cyclic_rec(dag_node *dag)
{
  int v;
  
  unification_cost++;

  dag = dag_deref(dag);

  v = dag_get_visit(dag);

  if(v == 0) // not yet seen
    {
      dag_arc *arc;

      dag_set_visit(dag, 1);

      arc = dag->arcs;
      while(arc)
	{
	  if(dag_cyclic_rec(arc->val))
	    return true;
	  arc = arc->next;
	}
      
      dag_set_visit(dag, 2);
    }
  else if(v == 1) // cycle found
    {
      return true;
    }

  return false;
}

bool dag_cyclic(dag_node *dag)
{
  bool res;

  res = dag_cyclic_rec(dag);
  dag_invalidate_visited();
  return res;
}

//
// de-shrinking: make fs totally wellformed
//

dag_node *dag_expand(dag_node *dag)
{
    // _fix_me_: this is not yet implemented
    // but refer to fully_expand in flop/expand.cpp which contains
    // the necessary code 
    return FAIL;
}

void dag_initialize()
{
  // nothing to do
}

