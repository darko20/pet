/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* common operations on arced dags */

#include <stdlib.h>
#include <assert.h>

#include "errors.h"
#include "dag.h"
#include "dag-arced.h"
#include "dumper.h"
#include "types.h"
#include "utility.h"

dag_node *new_dag(int s)
{
  dag_node *dag = dag_alloc_node();
  dag_init(dag, s);
  return dag;
}

void dag_remove_arcs(struct dag_node *dag, list_int *del)
{
  struct dag_arc *arc1, *arc2;
  
  // we recreate the dag->arcs list, only keeping elements not in del
  arc1 = dag->arcs; dag->arcs = NULL;

  while(arc1 != 0)
    {
      if(!contains(del, arc1->attr)) // keep this one
	{
	  arc2 = arc1;
	  arc1 = arc1->next;

	  arc2->next = dag->arcs;
	  dag->arcs = arc2;
	}
      else // ignore this one
	{
	  arc1 = arc1->next;
	}
    }
}

struct qc_node *dag_read_qc_paths(dumper *f, int limit, int &qc_len)
{
  struct qc_node *undumped_nodes;
  struct qc_arc *undumped_arcs = NULL;
  int dag_dump_total_nodes, dag_dump_total_arcs;

  struct dag_node_dump dump_n;
  struct dag_arc_dump dump_a;

  dag_dump_total_nodes = f->undump_int();
  dag_dump_total_arcs = f->undump_int();

  undumped_nodes = new qc_node[dag_dump_total_nodes];
  
  if(dag_dump_total_arcs > 0)
    undumped_arcs = new qc_arc[dag_dump_total_arcs];

  int current_arc = 0;
  qc_len = 0;
  
  for(int i = 0; i < dag_dump_total_nodes; i++)
    {
      undump_node(f, &dump_n);

      if(dump_n.type < 0)
	dump_n.type = -dump_n.type; // node is not expanded

      undumped_nodes[i].type = BI_TOP; // we infer the type 
      undumped_nodes[i].qc_pos = 0;
      undumped_nodes[i].arcs = 0;

      if(typestatus[dump_n.type] == ATOM_STATUS)
	{
	  int val;
      
	  val = strtoint(typenames[dump_n.type], "in qc structure", true);

	  if(val < 0 || val > 1024) // XXX 1024 is arbitrary
	    throw error("invalid node (value too large) in qc structure");

	  val += 1;
	  if(limit < 0 || val <= limit)
	    {
	      undumped_nodes[i].qc_pos = val;
	      if(val > qc_len) qc_len = val;
	    }
	}

      if(dump_n.nattrs > 0)
	undumped_nodes[i].arcs = undumped_arcs+current_arc;

      for(int j = 0; j < dump_n.nattrs; j++)
	{
	  undump_arc(f, &dump_a);
	  
	  undumped_nodes[i].type = glb(undumped_nodes[i].type, apptype[dump_a.attr]);

	  undumped_arcs[current_arc].attr = dump_a.attr;
	  undumped_arcs[current_arc].val = undumped_nodes + dump_a.val;
	  undumped_arcs[current_arc].next =
	    (j == dump_n.nattrs - 1) ? 0 : undumped_arcs+current_arc+1;
	  current_arc++;
	}
    }

  return undumped_nodes + dag_dump_total_nodes - 2; // remove first level of structure
}

// XXX todo: implement this for the new scheme

bool dag_prune_qc_paths(dag_node *qc_paths)
{
  bool useful = false;
  dag_arc *arc;

  arc = qc_paths->arcs;
  while(arc != 0)
    {
      if(dag_prune_qc_paths(arc->val) == false)
	useful = true;
      else
	arc->val = 0; // mark for deletion
      arc = arc->next;
    }

  // really remove `deleted' arcs

  // handle special case of first element
  while(qc_paths->arcs && qc_paths->arcs->val == 0)
    qc_paths->arcs = qc_paths->arcs->next;

  // non-first elements
  arc = qc_paths->arcs;
  while(arc)
    {
      if(arc->next && arc->next->val == 0)
        arc->next = arc->next->next;
      else
        arc = arc->next;
    }

  if(useful || qc_paths->type != 0) return false;

  return true; // this node can be pruned
}

void dag_get_qc_vector(qc_node *path, dag_node *dag, type_t *qc_vector)
{
#ifndef DAG_TOMABECHI
  dag = dag_deref(dag);
#endif

  if(path->qc_pos > 0)
    qc_vector[path->qc_pos - 1] = dag->type;

  if(dag->arcs == 0)
    return;

  dag_arc *arc;
  for(qc_arc *qarc = path->arcs; qarc != 0; qarc = qarc->next)
    if((arc = dag_find_attr(dag->arcs, qarc->attr)))
      dag_get_qc_vector(qarc->val, arc->val, qc_vector);
}

void dag_size_rec(struct dag_node *dag, int &nodes)
{
  dag = dag_deref(dag);
  nodes++;

  if(dag_set_visit(dag, dag_get_visit(dag) + 1) == 1)
    { // not yet visited
      struct dag_arc *arc;
      arc = dag->arcs;
      while(arc)
	{
          dag_size_rec(arc->val, nodes);
	  arc = arc->next;
	}
    }
}

int dag_size(dag_node *dag)
{
  if(dag == 0 || dag == FAIL) return 0;
  int nodes = 0;
  dag_size_rec(dag, nodes);
  dag_invalidate_visited();

  return nodes;
}

dag_node *dag_listify_ints(int *types, int ntypes)
{
  dag_node *result = new_dag(BI_NIL);

  for(int i = --ntypes; i >= 0; i--)
    {
      assert(types[i] >= 0);
      dag_node *cons = new_dag(BI_CONS);
      dag_node *first = new_dag(types[i]);
      dag_add_arc(cons, new_arc(BIA_FIRST, first));
      dag_add_arc(cons, new_arc(BIA_REST, result));
      result = cons;
    }

  return result;

}
