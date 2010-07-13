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

#include "dag-common.h"
#include "grammar-dump.h"
#include "dag.h"
#include "types.h"

#include <cstdlib>
#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

using namespace std;
using boost::algorithm::split;
using boost::algorithm::is_any_of;

/* global variables */

dag_node *FAIL = (dag_node *) -1;
bool unify_wellformed = true;
int unification_cost;

bool dag_nocasts;
int *featset;
int nfeatsets;
featsetdescriptor *featsetdesc;

dag_node **typedag = 0; // for [ 0 .. nstatictypes [

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

void register_dag(int i, dag_node* dag)
{
  typedag[i] = dag;
}

list<dag_node*> dag_get_list(dag_node* first)
{
  list <dag_node*> L;

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

dag_node *dag_get_attr_value(dag_node* dag, const char *attr)
{
  int a = lookup_attr(attr);
  if(a == -1) return FAIL;

  return dag_get_attr_value(dag, a);
}

// This is basically the prior dag_nth_arg with the only difference
// being that the attribute can be specified. Made inline so that there will
// be no difference in performance.
inline dag_node *dag_nth_element(dag_node *dag, int attr, int n)
{
  int i;
  dag_node *arg;

  if((arg = dag_get_attr_value(dag, attr)) == FAIL)
    return FAIL;

  for(i = 1; i < n && arg && arg != FAIL && !subtype(dag_type(arg), BI_NIL); i++)
    arg = dag_get_attr_value(arg, BIA_REST);

  if(i != n)
    return FAIL;

  arg = dag_get_attr_value(arg, BIA_FIRST);

  return arg;
}

dag_node *dag_nth_element(dag_node *dag, list_int *path, int n)
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
  return dag_nth_element(dag, attr, n); // inline call
}

dag_node *dag_nth_arg(dag_node *dag, int n)
{
  return dag_nth_element(dag, BIA_ARGS, n); // inline call
}

static void dag_find_paths_recursion(dag_node* dag, type_t maxapp,
    list_int *lpath, std::list<list_int*> &result)
{
  assert(dag != NULL);
  if ((lpath != NULL) && (subtype(dag_type(dag), maxapp))) {
    list_int *copy = copy_list(lpath);
    result.push_back(copy);
  }
  for (dag_arc *arc = dag->arcs; arc != NULL; arc = arc->next) {
    lpath = append(lpath, arc->attr);
    dag_find_paths_recursion(arc->val, maxapp, lpath, result);
    lpath = pop_last(lpath);
  }
}

std::list<list_int*> dag_find_paths(dag_node* dag, type_t maxapp)
{
  std::list<list_int*> result;
  dag_find_paths_recursion(dag, maxapp, NULL, result);
  return result;
}

dag_node* dag_get_path_value_check_dlist(dag_node *dag, list_int *path)
{
  while(path) {
    if(dag == FAIL) return FAIL;
    int feature = first(path);
    dag_node *next = dag_get_attr_value(dag, feature);
    if (feature == BIA_LIST) {
      dag_node *last =  dag_get_attr_value(dag, BIA_LAST);
      if (next == last) return FAIL;
    }
    dag = next;
    path = rest(path);
  }
  return dag;
}

dag_node *dag_get_path_value(dag_node *dag, list_int *path) {
  while(path) {
    if(dag == FAIL) return FAIL;
    dag = dag_get_attr_value(dag, first(path));
    path = rest(path);
  }
  return dag;
}

dag_node *dag_get_path_value(dag_node *dag, const char *path)
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
dag_node* dag_get_path_avail(dag_node *dag, list_int **path)
{
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
dag_node *dag_create_attr_value(attr_t attr, dag_node *val)
{
  assert(is_attr(attr) && is_type(apptype[attr]));

  dag_node* res = dag_full_copy(type_dag(apptype[attr]));
  dag_invalidate_changes();
  dag_set_attr_value(res, attr, val);

  return res;
}

dag_node *dag_create_attr_value(const string& attr, dag_node *val)
{
  if(val == FAIL) return FAIL;
  int a = lookup_attr(attr);
  if(a == -1) return FAIL;

  return dag_create_attr_value(a, val);
}

dag_node *dag_create_path_value(const string& path, type_t type)
{
  if(! is_type(type) || type_dag(type) == 0) return FAIL;

  // base case
  if (path.empty()) {
    dag_node* res = dag_full_copy(type_dag(type));
    dag_invalidate_changes();
    return res;
  }

  size_t dot = path.find('.');
  if(dot != string::npos) {
    string firstpart = path.substr(0, dot);
    string secondpart = path.substr(dot+1);
    return dag_create_attr_value(firstpart, dag_create_path_value(secondpart, type));
  } else
    return dag_create_attr_value(path, dag_create_path_value("", type));
}


dag_node *dag_unify(dag_node *root, dag_node *arg, list_int *path) {
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


dag_node *dag_create_path_value(list_int *path, type_t type)
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
dag_node *dag_create_path_value(list_int *path, dag_node *dag)
{
  if(path == 0) {
      return dag;
  } else {
    return dag_create_attr_value(first(path)
                                 , dag_create_path_value(rest(path), dag));
  }
}

list_int *path_to_lpath(const string& thepath)
{
  if (thepath.empty()) return NULL;
  string pathroot(thepath); // we need pathroot to be able to free the memory
  string path(thepath);

  list_int *head = cons((attr_t) 0, NULL);
  list_int *tail = head;

  vector<string> elements;
  split(elements, thepath, is_any_of("."));

  BOOST_FOREACH(string path, elements) {
    attr_t feat = lookup_attr(path);
    if (feat == -1) {
      free_list(head);
      return NULL;
    }
    tail->next = cons(feat, NULL);
    tail = tail->next;
  }
  tail = head->next;
  head->next = NULL;
  free_list(head);
  return tail;
}


#endif
