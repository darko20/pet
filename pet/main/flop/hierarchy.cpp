/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* operations on type hierarchy */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>

#include <string>
#include <vector>

#include "types.h"
#include "flop.h"

#include <LEDA/graph.h>
#include <LEDA/graph_misc.h> 
#include <LEDA/graph_iterator.h>
#include <LEDA/map.h>

//
// the main entry point to this module is process_hierarchy
//

// the type hierarchy is represented as a directed graph. the nodes
// are labeled by integers, that give the id of the type they represent
GRAPH<int, int> hierarchy;

// this maps a type given by id to the node in the graph
leda_map<int,leda_node> type_node;

// the length of the bitcodes (number of bits)
int codesize;

//
// register_type, subtype_constraint and undo_subtype_constraint are
// called by the TDL reader when parsing the input
//

// register a new type s in the hierarchy, and
// establish the mapping from id to graph node
void register_type(int s)
{
  leda_node v0 = hierarchy.new_node();
  hierarchy[v0] = s;
  type_node[s] = v0;
}

// enter the subtype relationship (t1 is an immediate subtype of t2) into
// the hierarchy
void subtype_constraint(int t1, int t2)
{
  if(t1 == t2)
    {
      fprintf(ferr, "warning: `%s' is declared subtype of itself.\n",
	      types.name(t1).c_str());
    }
  else
    {
      assert(t1 >= 0 && t2 >= 0); 
      hierarchy.new_edge(type_node[t2],type_node[t1]);
    }
}

// remove all the subtype constraints involving t - this is used when a
// type is redefined (as in the patch files)
void undo_subtype_constraints(int t)
{
  hierarchy.del_edges(hierarchy.out_edges(type_node[t]));
  hierarchy.del_edges(hierarchy.in_edges(type_node[t]));
}

//
// functions to find parents and children of a given type
//

// return the list of all immediate subtypes of t
list<int> immediate_subtypes(int t)
{
  leda_edge e;
  list<int> l;

  forall_out_edges(e, type_node[t])
    l.push_front(hierarchy.inf(hierarchy.target(e)));

  return l;
}

// return the list of all immediate supertypes of t
list<int> immediate_supertypes(int t)
{
  leda_edge e;
  list<int> l;

  forall_in_edges(e, type_node[t])
    l.push_front(hierarchy.inf(hierarchy.source(e)));

  return l;
}

//
// computation of the bitcodes
//

// maps from bit position in the bitcode to corresponding type
leda_map<int,int> idbit_type;

// compute the transitive closure encoding
void compute_code_topo() 
{
  // the code to be assigned next
  int codenr = codesize - 1;

  // used to iterate over the children
  list<int> l; int c;

  // create an iterator `it' that visits the nodes of the
  // hierarchy in reverse topological order
  TOPO_rev_It it(hierarchy);
  
  while(it.valid()) // iterate over all types in reverse topological order
    {
      // get the corresponding type id
      int current_type = hierarchy.inf(it.get_node());
      
      if(leaftypeparent[current_type] == -1) // leaf types don't get a code
	{
	  // check if this has already been visited - cannot happen
	  // if iterator works as expected
	  if(types[current_type]->bcode != NULL)
	    fprintf(ferr, "conception error: %s already visited...\n",
		    types.name(current_type).c_str());
	  
	  // create a new bitcode, it's initialized to all zeroes
	  types[current_type]->bcode = new bitcode(codesize);

	  // set the bit identifying this type
	  types[current_type]->bcode->insert(codenr);

	  idbit_type[codenr] = current_type;
	  codenr--;

	  // iterate over all immediate subtypes, ignoring leaf types
	  l = immediate_subtypes(current_type);
	  forallint(c, l) if(leaftypeparent[c] == -1)
	    {
	      // check that this has already a code assigned - if not,
	      // there's a horrible flaw somewhere
	      if(types[c]->bcode == NULL)
		fprintf(ferr, "conception error: %s not yet computed...\n",
			types.name(c).c_str());
	      
	      // combine with subtypes bitcode using binary or
	      types[current_type]->bcode->join(*types[c]->bcode);
	    }
	}

      ++it;
    }
}

// recompute hierarchy so it's a semilattice
// theoretical background: (Ait-Kaci et al., 1989)
void make_semilattice()
{
  int i, j, low, high;

  bool changed;

  if(verbosity > 4)
    fprintf(fstatus, " (%ld)", clock());

  // nr of synthesized glb types so far, old (total) number of types
  int glbtypes = 0, oldntypes = types.number();

  // scratch bitcode
  bitcode *temp = new bitcode(codesize);

  fprintf(fstatus, "glbs ");

  low = 0; high = types.number();

  // least fixpoint iteration - add glb types until nothing changes
  do {
    changed = false;

    // main loop: consider all ordered pairs of non-leaf types in the
    // range low ... high
    for(i = low; i < high; i++) if(leaftypeparent[i] == -1)
      for(j = i + 1; j < high; j++) if(leaftypeparent[j] == -1)
	{
	  // combine i's and j's bitcodes by binary and, and check
	  // if result if all zeroes
	  bool empty = intersect_empty(*types[i]->bcode,
				       *types[j]->bcode, temp);
	  
	  // if we have a non empty intersection, and it does not
	  // correspond to a type, we have to introduce a glb type
	  if(!empty && lookup_code(*temp) == -1)
	    {
	      struct type *glbtype;
	      char *name;
	      
	      // make up a name
	      name = (char *) salloc(20);
	      sprintf(name, "glbtype%d", glbtypes++);
	      
	      // create new type using this name and the result of the
	      // intersection as its bitcode
	      glbtype = new_type(name, false);
	      glbtype->def = new_location("synthesized", 0, 0);
	      glbtype->bcode = temp;

	      // register the new type's bitcode in the hash table
	      register_codetype(*temp, glbtype->id);

	      // create a new scratch code
	      temp = new bitcode(codesize);

	      changed = true;
	    }
	}
    // we only have to consider the new types in the next iteration
    low = high; high = types.number();

    // mark the new types as non leaftypes
    leaftypeparent = (int *) realloc(leaftypeparent, high * sizeof(int));
    for(i = low; i < high; i++) leaftypeparent[i] = -1;

  } while(changed);

  fprintf(fstatus, "[%d], ", glbtypes);

  // register the codes corresponding to non-leaf types
  resize_codes(types.number());
  for(i = 0; i < types.number(); i++)
    {
      if(leaftypeparent[i] == -1)
	register_typecode(i, types[i]->bcode);
      else
	register_typecode(i, NULL);
    }

  // now we have to recompute the graph representation of the
  // hierarchy - there are two ways of doing this:

  if(verbosity > 4)
    fprintf(fstatus, " (%ld)", clock());

  fprintf(fstatus, "recomputing");

#ifdef NAIVE_RECOMPUTATION

  // this is the naive approach: look at all ordered pairs
  // and check if they're in subtype relation

  hierarchy.del_all_edges();
  for(i = 0; i < types.number(); i++)
    {
      for(j = i + 1; j < types.number(); j++)
	if(core_subtype(i, j))
	  subtype_constraint(i, j);
	else if(core_subtype(j, i))
	  subtype_constraint(j, i);
    }

#else

  // this is a little more clever: keep the original hierarchy, and
  // only look at pairs involving a glb type.

  list_int *subtypes, *l;

  for(i = oldntypes; i < types.number(); i++)
    {
      l = subtypes = types[i]->bcode->get_elements();
      while(l)
	{
	  if(i != idbit_type[first(l)])
	    subtype_constraint(idbit_type[first(l)], i);

	  l = rest(l);
	}
      free_list(subtypes);
      
      for(j = 1; j < types.number(); j++) if(leaftypeparent[j] == -1)
	if(i != j && core_subtype(i, j))
	  subtype_constraint(i, j);
    }

#endif

  // sanity check: is the new hierarchy still acyclic
  if(!Is_Acyclic(hierarchy))
    {
      fprintf(ferr, "conception error: new type hierarchy is cyclic...\n");
      exit(1);
    }

  // now we have to remove the redundant links - this is just
  // computing the transitive reduction of the hierarchy

  leda_edge_array<bool> in_reduction(hierarchy);
  ACYCLIC_TRANSITIVE_REDUCTION(hierarchy, in_reduction);

  // now all edges that are in the reduction are marked - remove the others

  if(verbosity > 4)
    fprintf(fstatus, " (%ld)", clock());

  leda_list<leda_edge> el = hierarchy.all_edges();
  leda_edge e;
  int ndel = 0;
  forall(e, el)
    if(!in_reduction[e]) hierarchy.del_edge(e), ndel++;

  // do a few sanity checks:

  if(!Is_Simple(hierarchy))
    {
      fprintf(ferr, "conception error - making hierarchy simple\n");
      Make_Simple(hierarchy);
    }

  if(!Is_Loopfree(hierarchy))
    {
      fprintf(ferr, "conception error - making hierarchy loopfree\n");
      Delete_Loops(hierarchy);
    }

  if(verbosity > 4)
    fprintf(fstatus, " (%ld)", clock());

}

inline bool simple_leaftype(int i)
// is `i' a simple leaftype (it has no children and exactly one parent)
{
  return hierarchy.outdeg(type_node[i]) == 0 &&
    hierarchy.indeg(type_node[i]) == 1;
}

inline void mark_leaftype(int i)
// mark `i' as a leaftype
{
  leaftypeparent[i] = immediate_supertypes(i).front();
  nleaftypes++;
}

void find_leaftypes()
{
  int i;

  // initialize the leaftype array to all -1
  leaftypeparent = (int *) salloc(types.number() * sizeof(int));
  for(i = 0; i < types.number(); i++) leaftypeparent[i] = -1;

#ifdef ONLY_SIMPLE_LEAFTYPES

  // a type is a leaf type if it has no children and exactly one parent
  for(i = 0; i < types.number() ; i++)
    if(simple_leaftype(i))
      mark_leaftype(i);

#else

  // a type is a leaf type if it has exactly one parent, and a) it has no
  // children (simple leaftype), or b) all its children are leaftypes.

  // create an iterator `it' that visits the nodes of the
  // hierarchy in reverse topological order
  TOPO_rev_It it(hierarchy);
  
  while(it.valid()) // iterate over all types in reverse topological order
    {
      // get the corresponding type id
      i = hierarchy.inf(it.get_node());

      if(simple_leaftype(i))
	{
	  mark_leaftype(i);
	}
      else if (hierarchy.indeg(type_node[i]) == 1) // one parent
	{
	  // check if all children are leaftypes
	  int c; bool good = true;
	  
	  list<int> l = immediate_subtypes(i);
	  forallint(c, l)
	    if(leaftypeparent[c] == -1)
	      {
		good = false;
		break;
	      }
	  
	  if(good) 
	    mark_leaftype(i);
	}

      ++it;
    }

#endif
}

bool process_hierarchy()
{
  int i;

  fprintf(fstatus, "- type hierarchy (");

  // sanity check: is the hierarchy simple (contains no parallel edges)
  if(!Is_Simple(hierarchy))
    {
      fprintf(ferr, "type hierarchy is not simple (should not happen)\n");
      Make_Simple(hierarchy);
    }

  // check for cyclicity:
  if(!Is_Acyclic(hierarchy))
    {
      fprintf(ferr, "type hierarchy is cyclic.\n");
      return false;
    }

  // make all maximal types subtypes of TOP
  for(i = 1; i < types.number() ; i++)
    {
      if(hierarchy.indeg(type_node[i]) == 0)
	{
	  subtype_constraint(i, BI_TOP);
	}
    }

 fprintf(fstatus, "leaf types ");

  // for each type t, leaftypeparent[t] is -1 if t is not a leaftype,
  // and the id of the parent type otherwise

  find_leaftypes();

  fprintf(fstatus, "[%d], ", nleaftypes); 

  fprintf(fstatus, "bitcodes, ");

  // codesize is number of non-leaf types
  codesize = types.number() - nleaftypes;

  initialize_codes(codesize);
  compute_code_topo();
  
  for(i = 0; i < types.number(); i++) if(leaftypeparent[i] == -1)
    register_codetype(*types[i]->bcode, i);

  make_semilattice();

  fprintf(fstatus, ")\n");

  return true;
}
