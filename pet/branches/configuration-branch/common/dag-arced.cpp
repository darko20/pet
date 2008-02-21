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

/* common operations on arced dags */

#include "errors.h"
#include "dag.h"
#include "dag-arced.h"
#include "dumper.h"
#include "types.h"
#include "utility.h"

dag_node *new_dag(type_t s)
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

static struct qc_node *dag_qc_undumped_nodes = NULL;
static struct qc_arc *dag_qc_undumped_arcs = NULL;

struct qc_node *dag_read_qc_paths(dumper *f, int limit, int &qc_len) {
  int dag_dump_total_nodes, dag_dump_total_arcs;

  struct dag_node_dump dump_n;
  struct dag_arc_dump dump_a;

  dag_dump_total_nodes = f->undump_int();
  dag_dump_total_arcs = f->undump_int();

  dag_qc_undumped_nodes = new qc_node[dag_dump_total_nodes];
  
  if(dag_dump_total_arcs > 0)
    dag_qc_undumped_arcs = new qc_arc[dag_dump_total_arcs];
  else
    dag_qc_undumped_arcs = NULL;

  int current_arc = 0;
  qc_len = 0;

  for(int i = 0; i < dag_dump_total_nodes; i++) {
    undump_node(f, &dump_n);

    if(dump_n.type < 0)
      dump_n.type = -dump_n.type; // node is not expanded

    dag_qc_undumped_nodes[i].type = BI_TOP; // we infer the type
    dag_qc_undumped_nodes[i].qc_pos = 0;
    dag_qc_undumped_nodes[i].arcs = 0;

    if(typestatus[dump_n.type] == ATOM_STATUS) {
      int val;
      
      val = strtoint(type_name(dump_n.type), "in qc structure", true);

      if(val < 0 || val > 1024) // _fix_me_ 1024 is arbitrary
        throw tError("invalid node (value too large) in qc structure");

      val += 1;
      if(limit < 0 || val <= limit) {
        dag_qc_undumped_nodes[i].qc_pos = val;
        if(val > qc_len) qc_len = val;
      }
    }

    if(dump_n.nattrs > 0)
      dag_qc_undumped_nodes[i].arcs = dag_qc_undumped_arcs+current_arc;

    for(int j = 0; j < dump_n.nattrs; j++) {
      undump_arc(f, &dump_a);

      dag_qc_undumped_nodes[i].type
        = glb(dag_qc_undumped_nodes[i].type, apptype[dump_a.attr]);

      dag_qc_undumped_arcs[current_arc].attr = dump_a.attr;
      dag_qc_undumped_arcs[current_arc].val 
        = dag_qc_undumped_nodes + dump_a.val;

      dag_qc_undumped_arcs[current_arc].next =
        (j == dump_n.nattrs - 1) ? 0 : dag_qc_undumped_arcs+current_arc+1;
      current_arc++;
    }
  }

  return dag_qc_undumped_nodes + dag_dump_total_nodes - 2; // remove first level of structure
}

void dag_qc_free()
{
  if(dag_qc_undumped_nodes != 0)
  {
    delete[] dag_qc_undumped_nodes;
    dag_qc_undumped_nodes = 0;
  }
  if(dag_qc_undumped_arcs != 0)
  {
    delete[] dag_qc_undumped_arcs;
    dag_qc_undumped_arcs = 0;
  }
}

// _fix_me_ todo: implement this for the new scheme

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
    if((arc = dag_find_attr(dag->arcs, qarc->attr)) != 0)
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

dag_node *dag_listify_ints(list_int *types)
// builds list in reverse order
{
  dag_node *result = new_dag(BI_NIL);

  while(types != 0)
    {
      assert(first(types) >= 0);
      dag_node *cons = new_dag(BI_CONS);
      dag_node *front = new_dag(first(types));
      dag_add_arc(cons, new_arc(BIA_FIRST, front));
      dag_add_arc(cons, new_arc(BIA_REST, result));
      result = cons;
      types = rest(types);
    }

  return result;
}
