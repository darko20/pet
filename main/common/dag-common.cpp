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

/* operations on dags that are shared between different unifiers */

#include "pet-system.h"
#include "grammar-dump.h"
#include "dag.h"
#include "types.h"

/* global variables */

dag_node *FAIL = (dag_node *) -1;
bool unify_wellformed = true;
int unification_cost;

bool dag_nocasts;
int *featset;
int nfeatsets;
featsetdescriptor *featsetdesc;

struct dag_node **typedag = 0; // for [ 0 .. ntypes [

//
// external representation
//

void dump_node(dumper *f, dag_node_dump *x)
{
  f->dump_int(x->type);
  f->dump_short(x->nattrs);
}

void undump_node(dumper *f, dag_node_dump *x)
{
  x->type = f->undump_int();
  x->nattrs = f->undump_short();
}

void dump_arc(dumper *f, dag_arc_dump *x)
{
  f->dump_short(x->attr);
  f->dump_short(x->val);
}

void undump_arc(dumper *f, dag_arc_dump *x)
{
  x->attr = f->undump_short();
  x->val = f->undump_short();
}

// 
// name spaces etc.
//

void initialize_dags(int n)
{
  int i;
  typedag = New dag_node *[n];

  for(i = 0; i < n; i++) typedag[i] = 0;
}

void register_dag(int i, struct dag_node *dag)
{
  typedag[i] = dag;
}

list<struct dag_node *> dag_get_list(struct dag_node* first)
{
  list <struct dag_node *> L;

#ifdef DAG_SIMPLE
  if(first && first != FAIL) first = dag_deref(first);
#endif

  while(first && first != FAIL && !subtype(dag_type(first), BI_NIL))
    {
      dag_node *v = dag_get_attr_value(first, BIA_FIRST);
      if(v != FAIL) L.push_back(v);
      first = dag_get_attr_value(first, BIA_REST);
    }

  return L;
}

struct dag_node *dag_get_attr_value(struct dag_node *dag, const char *attr)
{
  int a = lookup_attr(attr);
  if(a == -1) return FAIL;

  return dag_get_attr_value(dag, a);
}

struct dag_node *dag_nth_arg(struct dag_node *dag, int n)
{
  int i;
  dag_node *arg;

  if((arg = dag_get_attr_value(dag, BIA_ARGS)) == FAIL)
    return FAIL;

  for(i = 1; i < n && arg && arg != FAIL && !subtype(dag_type(arg), BI_NIL); i++)
    arg = dag_get_attr_value(arg, BIA_REST);

  if(i != n)
    return FAIL;

  arg = dag_get_attr_value(arg, BIA_FIRST);

  return arg;
}

struct dag_node *dag_get_path_value_l(struct dag_node *dag, list_int *path)
{
  while(path)
    {
      if(dag == FAIL) return FAIL;
      dag = dag_get_attr_value(dag, first(path));
      path = rest(path);
    }

  return dag;
}

struct dag_node *dag_get_path_value(struct dag_node *dag, const char *path)
{
  if(path == 0 || strlen(path) == 0) return dag;

  const char *dot = strchr(path, '.');
  if(dot != 0)
    {
      char *attr = New char[strlen(path)+1];
      strncpy(attr, path, dot - path);
      attr[dot - path] = '\0';
      dag_node *f = dag_get_attr_value(dag, attr);
      delete[] attr;
      if(f == FAIL) return FAIL;
      return dag_get_path_value(f, dot + 1);
    }
  else
    return dag_get_attr_value(dag, path);
}

#ifndef FLOP

// create dag node with one attribute .attr.
struct dag_node *dag_create_attr_value(int attr, dag_node *val)
{
  dag_node *res;
  if(attr < 0 || attr >= nattrs || apptype[attr] < 0 || apptype[attr] >= ntypes)
    return FAIL;

  res = dag_full_copy(typedag[apptype[attr]]);
  dag_invalidate_changes();
  dag_set_attr_value(res, attr, val);

  return res;
}

struct dag_node *dag_create_attr_value(const char *attr, dag_node *val)
{
  int a = lookup_attr(attr);
  if(a == -1) return FAIL;

  return dag_create_attr_value(a, val);
}

struct dag_node *dag_create_path_value(const char *path, int type)
{
  dag_node *res = 0;

  // base case
  if(path == 0 || strlen(path) == 0)
    {
      if(type >= 0 && type < ntypes && typedag[type] != 0)
	{
	  res = dag_full_copy(typedag[type]);
	  dag_invalidate_changes();
	}
      return res;
    }

  const char *dot = strchr(path, '.');
  if(dot != 0)
    {
      char *firstpart = New char[strlen(path)+1];
      strncpy(firstpart, path, dot - path);
      firstpart[dot - path] = '\0';

      res = dag_create_attr_value(firstpart
                                  , dag_create_path_value(dot + 1, type));
      delete[] firstpart;
      return res;
    }
  else
    return dag_create_attr_value(path, dag_create_path_value(0, type));
}

#endif