/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* appropriateness and expansion */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "flop.h"
#include "types.h"
#include "options.h"

#include <LEDA/set.h>
#include <LEDA/d_array.h>
#include <LEDA/basic_graph_alg.h>
#include <LEDA/graph.h>
#include <LEDA/graph_iterator.h>
#include <LEDA/node_partition.h>

bool pseudo_type(int i)
{
  return flop_settings->member("pseudo-types", types.name(i).c_str());
}

bool dont_expand(int i)
{
  return types[i]->tdl_instance &&
    flop_settings->statusmember("dont-expand", types[i]->status);
}

// find maximal types that introduce features

bool compute_appropriateness()
{
  int i, attr;

  bool fail = false;

  fprintf(fstatus, "- computing appropriateness\n");

  apptype = new int[attributes.number()];
  
  for(i = 0; i < attributes.number(); i++)
    apptype[i] = BI_TOP;

  TOPO_It it(hierarchy);
  
  while(it.valid())
    {
      struct dag_arc *arc;
      i = hierarchy.inf(it.get_node());

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
			      " `%s' and `%s'.\n", attrname[attr], typenames[i],
			      typenames[apptype[attr]]);
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

      ++it;
    }

  for(i = 0; i < attributes.number(); i++)
    {
      if(apptype[i] == BI_TOP)
	{
	  if(opt_no_sem && i == attributes.id(flop_settings->req_value("sem-attr")))
	    apptype[i] = types.id(flop_settings->req_value("grammar-info"));
	  else
	    fprintf(ferr, "warning: attribute `%s' introduced on top (?)\n",
		    attributes.name(i).c_str());
	}
    }

  return !fail;
}

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
		      attrname[arc->attr], typenames[dag->type], typenames[old_type],
		      typenames[apptype[arc->attr]]);
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

bool delta_expand_types()
{
  int i, e;
  list<int> l;

  fprintf(fstatus, "- delta expansion for types\n");

  TOPO_It it(hierarchy);
  
  while(it.valid())
    {
      i = hierarchy.inf(it.get_node());

      if(!pseudo_type(i) && (opt_expand_all_instances || !dont_expand(i)))
	{
	  l = immediate_supertypes(i);
	  forallint(e, l)
	    {
	      if(dag_unify3(types[e]->thedag, types[i]->thedag) == FAIL)
		{
		  fprintf(ferr, "`%s' incompatible with parent constraints"
			  " (`%s')\n", typenames[i], typenames[e]);
		  return false;
		}
	    }
	}

      ++it;
    }

  return true;
}

void critical_types(struct dag_node *dag, leda_set<int> &cs)
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

#define MAX_EXP_DEPTH 1000

bool fully_expand(struct dag_node *dag)
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
		  typenames[dag->type]);
	  depth--;
	  return false;
	}

      if(dag->type < types.number() && (opt_full_expansion || dag->arcs))
	{
	  if(dag_unify3(types[dag->type]->thedag, dag) == FAIL)
	    {
	      fprintf(ferr, "full expansion with `%s' for",
		      typenames[dag->type]);
	      depth--;
	      return false;
	    }
	}

      arc = dag->arcs;
      while(arc)
	{
	  if(!fully_expand(arc->val))
	    {
	      depth--;
	      return false;
	    }
	  arc = arc->next;
	}

      depth --;
    }
  return true;
}

bool fully_expand_types()
{
  int i, e;
  list<int> l;
  leda_set<int> cs;

  GRAPH<int,int> G;
  leda_map<int,leda_node> s_node;

  fprintf(fstatus, "- full type expansion\n");

  bool fail = false;

  for(i = 0; i<types.number(); i++)
    {
      leda_node v0 = G.new_node();
      G[v0] = i;
      s_node[i] = v0;
    }

  for(i = 0; i<types.number(); i++)
    {
      cs.clear();
      critical_types(types[i]->thedag, cs);

      dag_invalidate_visited();

      forall(e, cs)
	if(i != e && e < types.number()) G.new_edge(s_node[i], s_node[e]);
    }

  initialize_dags(ntypes);

  unify_reset_visited = true;

  TOPO_It it(G);
  while(it.valid())
    {
      i = G.inf(it.get_node());

      if(!pseudo_type(i))
	{
	  if(!fully_expand(types[i]->thedag))
	    {
	      fprintf(ferr, " `%s' failed\n", typenames[i]);
	      fail = true;
	    }
	  
	  dag_invalidate_visited();

	  if(!fail && dag_cyclic(types[i]->thedag))
	    {
	      fprintf(ferr, " `%s' failed (cyclic structure)\n", typenames[i]);
	      fail = true;
	    }
	}

      register_dag(i, (types[i]->thedag = dag_deref(types[i]->thedag)));

      ++it;
    }

  unify_reset_visited = false;

  return !fail;
}

//
// maximal appropriate type computation
//

int *maxapp;
leda_d_array<int, int> nintro(0); // no of introduced features 

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
                  typenames[maxapp[i]],
                  types.name(apptype[i]).c_str());
        }
    }
}

//
// shrinking
//

static int total_nodes = 0;

int shrink_dag_rec(struct dag_node *dag, int root)
{
  int nshrinked = 0;
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
	nshrinked += shrink_dag_rec(dst, 0);
      else
	total_nodes++;
      
      coref = dag_get_visit(dst) - 1;

      if(dst->arcs == 0 && coref == 0 && dst->type == maxapp[arc->attr] && apptype[arc->attr] != root && root != -1)
	{
	  nshrinked++;
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

  return nshrinked;
}

void shrink_types()
{
  int i, n = 0;

  fprintf(fstatus, "- shrinking ");
  
  for(i = 0; i < types.number(); i++)
    {
      struct dag_node *curr = dag_deref(types[i]->thedag);

      dag_mark_coreferences(curr);
      n += shrink_dag_rec(curr, i);
      dag_invalidate_visited();

    }

  fprintf(fstatus, "(%d total nodes, %d removed)\n",
	  total_nodes, n);
}

//
// compute featconfs and featsets for fixed arity encoding
//

//
// partitioning of hierarchy, for non-minimal encoding
//

leda_d_array<int, int> nfeat(0); // total no of  features 
leda_d_array<int, int> prefixl(-1); // prefix ok if >= 0, gives length of prefix

void prefix_down(int t, int l)
{
  prefixl[t] = l + nintro[t];
  if(verbosity > 7)
    fprintf(fstatus, "prefix length of `%s' = %d\n",
	    types.name(t).c_str(), prefixl[t]);

  leda_list<leda_edge> children = hierarchy.out_edges(type_node[t]);
  leda_edge e;
  
  if(children.length() != 1) return;

  forall(e, children)
    {
      leda_node dest = hierarchy.opposite(type_node[t], e);
      prefix_down(hierarchy[dest], l + nintro[t]);
    }
}

//
// merge all types without features into one partition, top-down
//
void merge_top_down(int t, leda_node p, leda_node_partition &P)
{
  if(nfeat[t] > 0)
    {
#ifdef PREFIX_PARTITIONS
      prefix_down(t, 0);
#endif
      return;
    }
  prefixl[t] = 0;

  P.union_blocks(type_node[t], p);
  
  leda_list<leda_edge> children = hierarchy.out_edges(type_node[t]);
  leda_edge e;
  
  forall(e, children)
    {
      leda_node dest = hierarchy.opposite(type_node[t], e);
      
      if(!P.same_block(dest, p))
	merge_top_down(hierarchy[dest], p, P);
    }
}

void merge_partitions(int t, leda_node p, leda_node_partition &P, int s)
{
  if(nfeat[t] == 0 || prefixl[t] >= 0 ) return;
  
  P.union_blocks(type_node[t], p);

  if(subtype(hierarchy[P(type_node[t])], t))
    P.make_rep(type_node[t]);
  else
    {
      if(verbosity > 7)
	fprintf(fstatus, "merging %s into %s partition (from %s)\n",
		types.name(t).c_str(),
		types.name(hierarchy[P(type_node[t])]).c_str(),
		types.name(s).c_str());
    }
  
  leda_list<leda_edge> parents = hierarchy.in_edges(type_node[t]);
  leda_edge e;
  
  forall(e, parents)
    {
      leda_node dest = hierarchy.opposite(type_node[t], e);
      
      if(!P.same_block(dest, p))
	merge_partitions(hierarchy[dest], p, P, t);
    }
}

int *featconf; /* minimal feature configuration id for each type */

leda_d_array<int, list_int *> theconf(0);
leda_d_array<int, list_int *> theset(0);

void bottom_up_partitions()
{
  leda_node_partition part(hierarchy);

  fprintf(fstatus, "- partitioning hierarchy ");

  merge_top_down(0, part(type_node[0]), part);

  for(int i = 0; i < types.number(); i++)
    {
      if(hierarchy.outdeg(type_node[i]) == 0)
	merge_partitions(i, part(type_node[i]), part, 0);
    }

  leda_node_array<bool> reached(hierarchy);
  nfeatsets = 0;
  for(int i = 0; i < types.number(); i++)
    if(reached[type_node[i]] == false)
      {
	list_int *feats = 0;
	if(verbosity > 4)
	  fprintf(fstatus, "partition %d (`%s'):\n",
		  nfeatsets,
		  types.name(hierarchy[part(type_node[i])]).c_str());
	
	for(int j = 0; j < types.number(); j++)
	  if(reached[type_node[j]] == false && part.same_block(type_node[i], type_node[j]))
	    {
	      featset[j] = nfeatsets;
	      reached[type_node[j]] = true;
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

void generate_featsetdescs(int nconfs, leda_d_array<int, list_int*> &conf)
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
  leda_d_array<list_int *, int> feature_confs(0);
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
          leda_node_array<bool> reached(hierarchy);
          fprintf(fstatus, "type `%s': nsub: %d nfeat: %d fconf: %d nintro: %d",
                  types.name(i).c_str(),
                  DFS(hierarchy, type_node[i], reached).length(),
                  nf,
                  featconf[i],
                  nintro[i]);
          
          int sumintro = 0;
          for(int j = 0; j < types.number(); j++)
            if(reached[type_node[j]]) sumintro+=nintro[j];

          fprintf(fstatus, " sumintro: %d", sumintro);

	  for(int j = 0; j < attributes.number(); j++)
	    if(apptype[j] == i)
	      fprintf(fstatus, " %s", attributes.name(j).c_str());
          
          fprintf(fstatus, "\n");
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
