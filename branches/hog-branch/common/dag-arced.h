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

/** \file dag-arced.h
 * Common operations and data structures for arced dags.
 */

#ifndef _DAG_ARCED_H_
#define _DAG_ARCED_H_

#include "dag-alloc.h"

/** Allocate and initialize a new arc with attribute \a attr and value \a val 
 */
inline struct dag_arc *new_arc(attr_t attr, dag_node *val)
{
  dag_arc *newarc = dag_alloc_arc();
  newarc->attr = attr;
  newarc->val = val;
  newarc->next = 0;
  return newarc;
}

/** Add \a newarc to the arc list of \a dag */
inline void dag_add_arc(dag_node *dag, dag_arc *newarc)
{
  newarc->next = dag->arcs;
  dag->arcs = newarc;
}

/** Return the arc that has attribute \a attr, or \c NULL, if there is none */
inline dag_arc *dag_find_attr(dag_arc *arc, attr_t attr)
{
  while(arc && arc->attr != attr) arc = arc->next;
  return arc;
}

/** Small node structure to represent the quick check paths */
struct qc_node
{
  type_t type;
  int qc_pos;

  struct qc_arc *arcs;
};

/** Small arc structure to represent the quick check paths */
struct qc_arc
{
  attr_t attr;
  struct qc_node *val;

  struct qc_arc *next;
};

#endif
