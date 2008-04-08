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

struct dag_node *dag_get_path_value_l(struct dag_node *dag, list_int *path) {
  while(path) {
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
  if(dot != 0) {
    char *attr = new char[strlen(path)+1];
    strncpy(attr, path, dot - path);
    attr[dot - path] = '\0';
    dag_node *f = dag_get_attr_value(dag, attr);
    delete[] attr;
    if(f == FAIL) return FAIL;
    return dag_get_path_value(f, dot + 1);
  } else
    return dag_get_attr_value(dag, path);
}

/** \brief Return the deepest embedded subdag that could be reached starting at
 *  \a dag and following \a *path. The rest of the path that could not be
 *  traversed is stored in \a *path at return.
 *
 *  \param dag the feature structure from where to descend
 *  \param path a pointer to the variable holding the path to descend from
 *              \a dag
 *  \return the subdag at the end of the longest subpath that could be found,
 *          starting at \a dag, and the subpath that could not be found in 
 *          path.
 */
struct dag_node *
dag_get_path_avail(struct dag_node *dag, list_int **path) {
  while(NULL != *path) {
    dag_node *next_dag = dag_get_attr_value(dag, first(*path));
    // if this is how far we get, *path has the correct value
    if (next_dag == FAIL) return dag;
    // else descend
    dag = next_dag;
    *path = rest(*path);
  }
  return dag;
}

#ifndef FLOP

// create dag node with one attribute .attr.
struct dag_node *dag_create_attr_value(attr_t attr, dag_node *val)
{
  dag_node *res;
  assert(is_attr(attr) && is_type(apptype[attr]));

  res = dag_full_copy(type_dag(apptype[attr]));
  dag_invalidate_changes();
  dag_set_attr_value(res, attr, val);

  return res;
}

struct dag_node *dag_create_attr_value(const char *attr, dag_node *val)
{
  if(val == FAIL) return FAIL;
  int a = lookup_attr(attr);
  if(a == -1) return FAIL;

  return dag_create_attr_value(a, val);
}

struct dag_node *dag_create_path_value(const char *path, type_t type) {
  if(! is_type(type) || type_dag(type) == 0) return FAIL;
  dag_node *res = 0;

  // base case
  if(path == 0 || strlen(path) == 0) {
    res = dag_full_copy(type_dag(type));
    dag_invalidate_changes();
    return res;
  }

  const char *dot = strchr(path, '.');
  if(dot != 0) {
    char *firstpart = new char[strlen(path)+1];
    strncpy(firstpart, path, dot - path);
    firstpart[dot - path] = '\0';

    res = dag_create_attr_value(firstpart
                                , dag_create_path_value(dot + 1, type));
    delete[] firstpart;
    return res;
  } else
    return dag_create_attr_value(path, 
                                 dag_create_path_value((const char *)NULL
                                                       , type));
}


struct dag_node *dag_unify(dag_node *root, dag_node *arg, list_int *path) {
  dag_node *subdag = dag_get_path_avail(root, &path);
  if (path != NULL) {
    // We did not manage to get to the end: create a new dag for the rest of
    // the path and the dag of arg at its end
    arg = dag_create_path_value(path, arg);
    if (arg == FAIL) return FAIL;
  } 
  dag_node *newdag = dag_unify(root, arg, subdag, 0);
  if (newdag == FAIL) return FAIL;
  return newdag;
}


struct dag_node *dag_create_path_value(list_int *path, type_t type)
{
  if(! is_type(type) || type_dag(type) == NULL) return FAIL;
  if(path == 0) {
      dag_node *res = dag_full_copy(type_dag(type));
      dag_invalidate_changes();
      return res;
  } else {
    return dag_create_attr_value(first(path)
                                 , dag_create_path_value(rest(path), type));
  }
}

//_fix_me_ the dag should be unified at the end
struct dag_node *dag_create_path_value(list_int *path, dag_node *dag)
{
  if(path == 0) {
      return dag;
  } else {
    return dag_create_attr_value(first(path)
                                 , dag_create_path_value(rest(path), dag));
  }
}

struct list_int *path_to_lpath(const char *thepath) {
  if(thepath == 0 || strlen(thepath) == 0) return NULL;
  char *path, *pathroot; // we need pathroot to be able to free the memory
  path = pathroot = strdup(thepath);
  char *pathend = path + strlen(path); // points to the \0 char

  list_int *head, *tail;
  head = tail = cons((attr_t) 0, NULL);

  attr_t feat;
  do {
    char *dot = strchr(path, '.');
    if (dot == NULL)
      dot = pathend;
    else 
      *dot = '\0';
    feat = lookup_attr(path);
    if (feat == -1) {
      free(pathroot);
      free_list(head);
      return NULL;
    }
    tail->next = cons(feat, NULL);
    tail = tail->next;
    path = dot + 1;
  } while (path < pathend);
  tail = head->next;
  head->next = NULL;

  free(pathroot);
  free_list(head);
  return tail;
}


struct list_int *path_to_lpath0(const char *path)
{
  if(path == 0 || strlen(path) == 0) return NULL;

  const char *dot = strchr(path, '.');
  if(dot != 0)
    { // copy the feature name into attr and get its code
      char *attr = new char[dot - path + 1];
      strncpy(attr, path, dot - path);
      attr[dot - path] = '\0';
      attr_t feat = lookup_attr(attr);
      delete[] attr;
      if (feat == -1) return NULL;
      list_int *sub = path_to_lpath(dot + 1);
      if (sub == NULL) return NULL;
      return cons(feat, sub);
    }
  else
    {
    attr_t feat = lookup_attr(path);
    if (feat == -1) return NULL;
    return cons(feat, NULL);
    }
}


#endif
