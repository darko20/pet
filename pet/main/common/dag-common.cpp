/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* operations on dags that are shared between different unifiers */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
  typedag = new dag_node *[n];

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

struct dag_node *dag_get_attr_value(struct dag_node *dag, char *attr)
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

struct dag_node *dag_get_path_value(struct dag_node *dag, char *path)
{
  if(path == 0 || strlen(path) == 0) return dag;

  char *dot = strchr(path, '.');
  if(dot != 0)
    {
      char *attr = new char[strlen(path)+1];
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

#if 0

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

struct dag_node *dag_create_attr_value(char *attr, dag_node *val)
{
  int a = lookup_attr(attr);
  if(a == -1) return FAIL;

  return dag_create_attr_value(a, val);
}

struct dag_node *dag_create_path_value(char *path, int type)
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

  dag_node *resrest;
  char *dot = strchr(path, '.');
  char *firstpart;

  if(dot != 0)
    {
      firstpart = new char[strlen(path)+1];
      strncpy(firstpart, path, dot - path);
      firstpart[dot - path] = '\0';

      resrest = dag_create_path_value(dot + 1, type);
    }
  else
    {
      firstpart = path;
      resrest = dag_create_path_value(0, type);
    }

  res = dag_create_attr_value(firstpart, resrest);

  if(firstpart != path)
    delete[] firstpart;

  return res;
}

#endif
