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

/* appropriateness and expansion */

#include "types.h"
#include "options.h"
#include "flop.h"
#include "hierarchy.h"
#include "partition.h"

#include <set>
#include <boost/graph/topological_sort.hpp>

/** Is type \a i a pseudo type?
 * Pseudo types are used to define feature structures that do not belong to
 * the type hierarchy, like quick check structures or restrictors. They are not
 * checked for appropriateness and the like and are not expanded or unfilled.
 * These types have to be specified in the \c flop.set file by means of the \c
 * pseudo-types setting.
 */
bool pseudo_type(type_t i)
{
  return flop_settings->member("pseudo-types", types.name(i).c_str());
}

/** Should expansion of the type dag of type \a i be avoided ?
 * \return \c true if \i is an instance with a status that is mentioned in the
 * 'dont-expand' setting in 'flop.set'
 */
bool dont_expand(type_t i)
{
  return types[i]->tdl_instance &&
    flop_settings->statusmember("dont-expand", types[i]->status);
}

/** For every feature, compute the maximal type that introduces it.
 *
 * \return \c false if a feature gets introduced at two incompatible types,
 * which is an error, \c true if all appropriate types could be computed. The
 * array \c apptype is filled with the maximally appropriate types, i.e., \c
 * apptype[j] is the first subtype of \c *top* that introduces feature \c j.
 */
bool compute_appropriateness()
{
  int i, attr;

  bool fail = false;

  fprintf(fstatus, "- computing appropriateness\n");

  apptype = new int[attributes.number()];
  
  for(i = 0; i < attributes.number(); i++)
    apptype[i] = BI_TOP;
    
  vector<int> topo;
  boost::topological_sort(hierarchy, std::back_inserter(topo));
  for(vector<int>::reverse_iterator it = topo.rbegin(); it != topo.rend(); ++it)
    {
      struct dag_arc *arc;
      i = *it;
        
      if(!pseudo_type(i))
        {
          arc = dag_deref(types[i]->thedag)->arcs;
          while(arc) // look at all top level features of type
            {
              attr = arc->attr;
              
              if(apptype[attr] != BI_TOP)
                {
                  if(!subtype(i, apptype[attr]))
                    {
                      fprintf(ferr, "error: feature `%s' introduced at"
                              " `%s' and `%s'.\n", attrname[attr],
                              type_name(i),
                              type_name(apptype[attr]));
                      fail = true;
                    }
                }
              else
                {
              apptype[attr] = i;
                }
              arc = arc->next;
            }
        }
    }

  for(i = 0; i < attributes.number(); i++)
    {
      if(apptype[i] == BI_TOP)
        {
          // With the option 'no-semantics', all structures under a feature
          // which is specified as 'sem-attr' in the 'flop.set' are
          // removed from the hierarchy definitions and the feature itself is
          // ignored in all computation concering features, such as appropriate
          // type computation.
          if(opt_no_sem 
             && i == attributes.id(flop_settings->req_value("sem-attr")))
            apptype[i] = types.id(flop_settings->req_value("grammar-info"));
          else
            // This attribute did not appear on the top level of a type
            // definition, so maybe it was not introduced correctly
            fprintf(ferr, "warning: attribute `%s' introduced on top (?)\n",
                    attributes.name(i).c_str());
        }
    }

  return !fail;
}

/** Apply the appropriateness conditions recursively on \a dag.
 * In every subdag of \a dag, unify the type of the subdag with all appropriate
 * types induced by the features of that subdag. If this unification fails, the
 * type definition is inconsistent.
 * \return \c true if the dag is consistent with the appropriateness
 * conditions required by the features, \c false otherwise
 */
bool apply_appropriateness_rec(struct dag_node *dag)
{
  dag = dag_deref(dag);

  if(dag_set_visit(dag, dag_get_visit(dag) + 1) == 1) // not yet visited
    { 
      int new_type, old_type;
      struct dag_arc *arc;

      new_type = dag->type;
      arc = dag->arcs;

      while(arc)
        {
          old_type = new_type;
          new_type = glb(new_type, apptype[arc->attr]);
          
          if(new_type == -1)
            {
              fprintf(ferr, "feature `%s' on type `%s' (refined to `%s' from other features) not appropriate "
                      "(appropriate type is `%s')\n",
                      attrname[arc->attr], type_name(dag->type),
                      type_name(old_type),
                      type_name(apptype[arc->attr]));
              return false;
            }

          if(!apply_appropriateness_rec(arc->val))
            return false;
          
          arc = arc->next;
        }

      dag->type = new_type;
    }

  return true;
}

/** Apply the appropriateness conditions on each dag in the hierarchy.
 * \see apply_appropriateness_rec
 */
bool apply_appropriateness()
{
  int i;

  bool fail = false;

  fprintf(fstatus, "- applying appropriateness constraints for types\n");

  for(i = 0; i < types.number(); i++)
    {
      if(!pseudo_type(i) && !apply_appropriateness_rec(types[i]->thedag))
        {
          fprintf(ferr, "when applying appropriateness constraints on type `%s'\n",
                  types.name(i).c_str());
          fail = true;;
        }

      dag_invalidate_visited();
    }

  return !fail;
}

/** Do delta expansion: unify all supertype constraints into a type skeleton.
 *
 * Excluded from this step are all pseudo types (\see pseudo_type). Instances
 * are only expanded if either the option 'expand-all-instances' is active or
 * the instance does not have a status that is mentioned in the 'dont-expand'
 * setting in 'flop.set'.
 * \return \c true, if there were no inconsistent type definitions found, 
 *         \c false otherwise. 
 */
bool delta_expand_types()
{
  int i, e;
  list<int> l;

  vector<int> topo;
  boost::topological_sort(hierarchy, std::back_inserter(topo));
  for(vector<int>::reverse_iterator it = topo.rbegin(); it != topo.rend(); ++it)
    {
      i = *it;
      
      if(!pseudo_type(i) && (opt_expand_all_instances || !dont_expand(i)))
        {
          l = immediate_supertypes(i);
          forallint(e, l)
            {
              if(dag_unify3(types[e]->thedag, types[i]->thedag) == FAIL)
                {
                  fprintf(ferr, "`%s' incompatible with parent constraints"
                          " (`%s')\n", type_name(i), type_name(e));
                  return false;
                }
            }
        }
    }

  return true;
}

/**
 * Recursively collect the types found in a feature structure into a set
 * \param dag A pointer to the feature structure where the used types are
 *        collected 
 * \param cs Reference to a set that will contain the result
 * \return A set of all types occuring in #dag collected in #cs
 */
void critical_types(struct dag_node *dag, set<int> &cs)
{
  dag = dag_deref(dag);

  if(dag_set_visit(dag, dag_get_visit(dag) + 1) == 1) // not yet visited
    { 
      struct dag_arc *arc;

      if(dag->type < types.number() /* && dag->arcs */ )
        cs.insert(dag->type);

      arc = dag->arcs;
      while(arc)
        {
          critical_types(arc->val, cs);
          arc = arc->next;
        }
    }
}

/** Limit the depth of full expansion to MAX_EXP_DEPTH.
 * There might be cases where we have infinite expansion that we could not
 * detect before. Therefore we use this maximum expansion depth.
 */
#define MAX_EXP_DEPTH 1000

/**
 * Recursively unify the feature constraints of the type of each dag node into
 * the node
 *
 * \param dag Pointer to the current dag
 * \param full If \c true, expand subdags without arcs too
 * \result A path to the failure point, if one occured, NULL otherwise
 */
list_int *fully_expand(struct dag_node *dag, bool full)
{
  static int depth = 0;

  dag = dag_deref(dag);
  
  if(dag_set_visit(dag, dag_get_visit(dag) + 1) == 1) // not yet visited
    { 
      struct dag_arc *arc;
      depth++;

      if(depth > MAX_EXP_DEPTH)
        {
          fprintf(ferr, "expansion [cycle with `%s'] for",
                  type_name(dag->type));
          depth--;
          return cons(T_BOTTOM, NULL);
        }

      if(dag->type < types.number() && (full || dag->arcs))
        {
          if(dag_unify3(types[dag->type]->thedag, dag) == FAIL)
            {
              fprintf(ferr, "full expansion with `%s' for",
                      type_name(dag->type));
              depth--;
              return cons(T_BOTTOM, NULL);
            }
        }

      list_int *res;
      arc = dag->arcs;
      while(arc)
        { 
          if((res = fully_expand(arc->val, full)) != NULL)
            {
              depth--;
              return cons(arc->attr, res);
            }
          arc = arc->next;
        }

      depth --;
    }
  return NULL;
}

/**
 * Expand the feature structure constraints of each type after delta expansion.
 * 
 * The algorithm is as follows: Build a graph of types such that a link
 * from type t to type s exists if the feature structure skeleton of type 
 * s uses type t in some substructure. If this graph is acyclic, expand the
 * feature structure constraints fully in topological order over this graph.
 * Otherwise, there are illegal cyclic type dependencies in the definitions 
 *
 * \return true if the definitions are all OK, false otherwise
 */
bool fully_expand_types(bool full_expansion)
{
  int i;
  list<int> l;
  set<int> cs;

  tHierarchy G;
  
  bool fail = false;
  
  for(i = 0; i < types.number(); ++i)
    {
      int v = boost::add_vertex(G);
      assert(v == i);
    }

  for(i = 0; i < types.number(); ++i)
    {
      cs.clear();
      critical_types(types[i]->thedag, cs);

      dag_invalidate_visited();

      for(set<int>::iterator it = cs.begin(); it != cs.end(); ++it)
        {
          if(i != *it && *it < types.number())
            boost::add_edge(i, *it, G);
        }
    }

  initialize_dags(nstatictypes);

  unify_reset_visited = true;
   
  vector<int> topo;
  boost::topological_sort(G, std::back_inserter(topo));
  for(vector<int>::reverse_iterator it = topo.rbegin(); it != topo.rend(); ++it)
    {
      i = *it;
        
      if(!pseudo_type(i))
        {
          list_int *path = fully_expand(types[i]->thedag, full_expansion);

          if(path != NULL)
            {
              fprintf(ferr, " `%s' failed under path (", type_name(i));
              list_int *start = path;
              while (rest(start) != NULL) { // last element is invalid
                fprintf(ferr, "%s", attrname[first(start)]);
                start = rest(start);
                if (rest(start) != NULL) fprintf(ferr, "|");
              }
              fprintf(ferr, ")\n");
              fail = true;
              free_list(path);
            }
          
          dag_invalidate_visited();

          if(!fail && dag_cyclic(types[i]->thedag))
            {
              fprintf(ferr, " `%s' failed (cyclic structure)\n", type_name(i));
              fail = true;
            }
        }

      register_dag(i, (types[i]->thedag = dag_deref(types[i]->thedag)));
    }

  unify_reset_visited = false;

  return !fail;
}

//
// maximal appropriate type computation
//

extern int *maxapp;
map<int, int> nintro; // no of introduced features 

/** Compute the number of features introduced by each type and the maximal
 * appropriate type per feature.
 *
 * The maximal appropriate type \c t for feature \c f is the type that is
 * supertype of every dag \c f points to. It is defined by the type of the
 * subdag \c f points to where \c f is introduced.
 */
void compute_maxapp()
{
  int i;

  maxapp = new int[attributes.number()];
  
  // initialize nintro array to number of features introduced by that type
  for(i = 0; i < types.number(); i++)
    for(int j = 0; j < attributes.number(); j++)
      if(apptype[j] == i) nintro[i]++;
  
  // initialize maxapp array to max appropriate type per feature 
  for(i = 0; i < attributes.number(); i++)
    {
      maxapp[i] = 0;
      struct dag_node *cval = dag_get_attr_value(types[apptype[i]]->thedag, i);
      if(cval && cval != FAIL)
        maxapp[i] = dag_type(cval);
      
      if(verbosity > 7)
        {
          fprintf(fstatus, "feature `%s': value: %s `%s', introduced by `%s'\n",
                  attributes.name(i).c_str(),
                  maxapp[i] > types.number() ? "symbol" : "type",
                  type_name(maxapp[i]),
                  types.name(apptype[i]).c_str());
        }
    }
}

/** @name Unfilling */
/*@{*/ 

static int total_nodes = 0;

/** Recursively unfill \a dag, which is a subdag of the typedag of \a root
 * \return the number of deleted dag nodes
 *
 * Remove all features that are not introduced by \a root and where the subdag
 * they point to 
 * - has no arcs (which may be due to recursive unfilling)
 * - is not coreferenced
 * - its type is the maximally appropriate type for the feature
 *
 * The unfilling algorithm deletes all edges that point to nodes with maximally
 * underspecified information, i.e. whose type is the maximally appropriate
 * type under the attribute and whose substructures are also maximally
 * underspecified. This results in nodes that contain some, but not all of the
 * edges that are appropriate for the node's type. There are in fact partially
 * unfilled fs nodes.
 */
int unfill_dag_rec(struct dag_node *dag, int root)
{
  int nunfilled = 0;
  int coref = dag_get_visit(dag) - 1;

  if(coref < 0) // dag is coreferenced, already visited
    {
      return 0;
    }
  else if(coref > 0) // dag is coreferenced, not yet visited
    {
      dag_set_visit(dag, -1);
    }

  total_nodes++;

  struct dag_arc *arc, *keep, *tmparc;

  arc = dag->arcs; keep = 0;
  
  while(arc)
    {
      struct dag_node *dst = dag_deref(arc->val);
      
      if(dst->arcs)
        nunfilled += unfill_dag_rec(dst, 0);
      else
        total_nodes++;
      
      coref = dag_get_visit(dst) - 1;

      if(dst->arcs == 0 && coref == 0 && dst->type == maxapp[arc->attr]
         && apptype[arc->attr] != root) // && root != -1 <-- Always true
        {
          nunfilled++;
          arc = arc->next;
        }
      else
        {
          tmparc = arc;
          arc = arc->next;
          
          tmparc->next = keep;
          keep = tmparc;
        }
    }
  
  dag->arcs = keep;

  return nunfilled;
}

/** Unfill ALL type dags in the hierarchy.
 * For which arcs and nodes in the dags are removed, \see unfill_dag_rec.
 */
void unfill_types()
{
  int i, n = 0;

  fprintf(fstatus, "- unfilling ");
  
  for(i = 0; i < types.number(); i++)
    {
      struct dag_node *curr = dag_deref(types[i]->thedag);

      dag_mark_coreferences(curr);
      n += unfill_dag_rec(curr, i);
      dag_invalidate_visited();

    }

  fprintf(fstatus, "(%d total nodes, %d removed)\n",
          total_nodes, n);
}
/*@}*/

//
// compute featconfs and featsets for fixed arity encoding
//

//
// partitioning of hierarchy, for non-minimal encoding
//

/** Total no of features for a certain type */
map<int, int> nfeat;
/** prefix ok if in map and >= 0, gives length of prefix */
map<int, int> prefixl;

void prefix_down(int t, int l)
{
  prefixl[t] = l + nintro[t];
  if(verbosity > 7)
    fprintf(fstatus, "prefix length of `%s' = %d\n",
            types.name(t).c_str(), prefixl[t]);
    
  if(boost::out_degree(t, hierarchy) != 1)
    return;
  
  list<int> children = immediate_subtypes(t);
  for(list<int>::iterator it = children.begin(); it != children.end(); ++it)
    {
      prefix_down(*it, l + nintro[*it]);
    }
}

//
// merge all types without features into one partition, top-down
//
void merge_top_down(int t, int p, tPartition &P)
{
  if(nfeat[t] > 0)
    {
#ifdef PREFIX_PARTITIONS
      prefix_down(t, 0);
#endif
      return;
    }
  prefixl[t] = 0;

    P.union_sets(t, p);
  
    list<int> children = immediate_subtypes(t);
    for(list<int>::iterator it = children.begin(); it != children.end(); ++it)
    {
        if(!P.same_set(*it, p))
            merge_top_down(*it, p, P);
    }
}

void merge_partitions(int t, int p, tPartition &P, int s)
{
  if(nfeat[t] == 0 || prefixl.find(t) != prefixl.end() && prefixl[t] >= 0) return;
  
  P.union_sets(t, p);
  
  if(subtype(P(t), t))
    P.make_rep(t);
  else
    {
      if(verbosity > 7)
        fprintf(fstatus, "merging %s into %s partition (from %s)\n",
                types.name(t).c_str(),
                types.name(P(t)).c_str(),
                types.name(s).c_str());
    }
  
  list<int> parents = immediate_supertypes(t);
  
  for(list<int>::iterator it = parents.begin(); it != parents.end(); ++it)
    {
      if(!P.same_set(*it, p))
        merge_partitions(*it, p, P, t);
    }
}

int *featconf; /* minimal feature configuration id for each type */

map<int, list_int *> theconf;
map<int, list_int *> theset;

void bottom_up_partitions()
{
  assert(types.number() == (int) boost::num_vertices(hierarchy));
  tPartition part(types.number());
  
  fprintf(fstatus, "- partitioning hierarchy ");

  merge_top_down(0, part(0), part);
  
  for(int i = 0; i < types.number(); i++)
    {
      if(boost::out_degree(i, hierarchy) == 0)
        merge_partitions(i, part(i), part, 0);
    }
  
  map<int, bool> reached;
  
  nfeatsets = 0;
  for(int i = 0; i < types.number(); i++)
    if(!reached[i])
      {
        list_int *feats = 0;
        if(verbosity > 4)
          fprintf(fstatus, "partition %d (`%s'):\n",
                  nfeatsets,
                  types.name(part(i)).c_str());

        for(int j = 0; j < types.number(); j++)
          if(!reached[j] && part.same_set(i, j))
            {
              featset[j] = nfeatsets;
              reached[j] = true;
              if(verbosity > 4)
                fprintf(fstatus, "  %s\n", types.name(j).c_str());
                
              list_int *l = theconf[featconf[j]];
              while(l)
                {
                  if(!contains(feats, first(l))) feats = cons(first(l), feats);
                  l = rest(l);
                }
            }
        
        if(verbosity > 4)
          {
            fprintf(fstatus, "features (%d):", length(feats));
        
            list_int *l = feats;
            while(l)
              {
                fprintf(fstatus, " %s", attributes.name(first(l)).c_str()); 
                l = rest(l);
              }
            fprintf(fstatus, "\n");
          }

        theset[nfeatsets] = feats;

        nfeatsets++;
      }

  fprintf(fstatus, "(%d partitions)\n", nfeatsets);

}

// featconf and featset computation

int si_compare(const void *a, const void *b)
{
  return *((short int *) a) - *((short int *) b);
}

void generate_featsetdescs(int nconfs, map<int, list_int*> &conf)
{
  // generate feature table descriptors

  featsetdesc = new featsetdescriptor[nconfs];

  for(int i = 0; i < nconfs; i++)
    {
      list_int *l = conf[i];
      int n = length(l);
      featsetdesc[i].n = n;
      featsetdesc[i].attr = n > 0 ? new short int [n] : 0;

      int j = 0;
      while(l)
        {
          featsetdesc[i].attr[j++] = first(l);
          l = rest(l);
        }
      
      if(n > 0)
        qsort(featsetdesc[i].attr, n, sizeof(short int), si_compare);
    }
}

void compute_feat_sets(bool minimal)
{
  map<list_int *, int> feature_confs;
  int feature_conf_id = 0;
  int i;

  featconf = new int[types.number()];
  featset = new int[types.number()];

  for(i = 0; i < types.number(); i++)
    {
      int nf = 0;
      list_int *feats = 0;

      for(int j = 0; j < attributes.number(); j++)
        {
          if(subtype(i, apptype[j]))
            {
              nf++;
              feats = cons(j, feats);
            }
        }
          
      if(feats)
        {
          if((featconf[i] = feature_confs[feats]) == 0)
            {
              featconf[i] = (feature_confs[feats] = ++feature_conf_id);
              theconf[feature_conf_id] = feats;
            }
        }
      else
        featconf[i] = 0;

      nfeat[i] = nf;

      if(verbosity > 7 && nintro[i] > 0)
        {
#if 0
          int nsub = DFS(hierarchy, i, reached).length();

          fprintf(fstatus, "type `%s': nsub: %d nfeat: %d fconf: %d nintro: %d",
                  types.name(i).c_str(),
                  nsub, 
                  nf,
                  featconf[i],
                  nintro[i]);
          
          int sumintro = 0;
          for(int j = 0; j < types.number(); j++)
            if(reached[j]) sumintro += nintro[j];
          
          fprintf(fstatus, " sumintro: %d", sumintro);

          for(int j = 0; j < attributes.number(); j++)
            if(apptype[j] == i)
              fprintf(fstatus, " %s", attributes.name(j).c_str());
          
          fprintf(fstatus, "\n");
#endif
        }
    }

  for(i = 0; verbosity > 7 && i < feature_conf_id; i++)
    {
      fprintf(fstatus, "feature configuration %d:", i);
      
      list_int *l = theconf[i];
      while(l)
        {
          fprintf(fstatus, " %s", attributes.name(first(l)).c_str()); 
          l = rest(l);
        }
      fprintf(fstatus, "\n");
    }

  if(!minimal)
    {
      bottom_up_partitions();
      generate_featsetdescs(nfeatsets, theset);
    }
  else
    {
      nfeatsets = feature_conf_id + 1;

      for(int i = 0; i < types.number(); i++)
        {
          featset[i] = featconf[i];
        }

      generate_featsetdescs(nfeatsets, theconf);
    }
}
