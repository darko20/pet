/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* common operations and data structures for arced dags */

#ifndef _DAG_ARCED_H_
#define _DAG_ARCED_H_

struct dag_node *new_dag(int s);

inline struct dag_arc *new_arc(int attr, dag_node *val)
{
  dag_arc *newarc = dag_alloc_arc();
  newarc->attr = attr;
  newarc->val = val;
  newarc->next = 0;
  return newarc;
}

inline void dag_add_arc(dag_node *dag, dag_arc *newarc)
{
  newarc->next = dag->arcs;
  dag->arcs = newarc;
}

struct qc_node
{
  int type;
  int qc_pos;

  struct qc_arc *arcs;
};

struct qc_arc
{
  int attr;
  struct qc_node *val;

  struct qc_arc *next;
};

#endif
