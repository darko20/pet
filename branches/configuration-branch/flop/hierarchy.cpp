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

/* operations on type hierarchy */

#include "bitcode.h"
#include "flop.h"
#include "hierarchy.h"
#include "types.h"
#include "settings.h"
#include "list-int.h"
#include "options.h"
#include "utility.h"
#include "logging.h"

#include <boost/graph/topological_sort.hpp>
#include <sstream>

using std::list;
using std::map;
using std::vector;

void acyclicTransitiveReduction(tHierarchy &G);

//
// the main entry point to this module is process_hierarchy
//

// The type hierarchy is represented as a directed graph. The vertex
// descriptor (implicitely) corresponds to the id of the type it represents.
tHierarchy hierarchy;

// The length of the bitcodes (number of bits).
int codesize;

//
// register_type, subtype_constraint and undo_subtype_constraint are
// called by the TDL reader when parsing the input.
//

// Register a new type s in the hierarchy.
void register_type(int s)
{
    int v = boost::add_vertex(hierarchy);
    assert(v == s);
}

// Enter the subtype relationship (t1 is an immediate subtype of t2) into
// the hierarchy.
void subtype_constraint(int sub, int super)
{
    if(sub == super)
    {
        LOG(logSemantic, WARN,
            "`" << types.name(sub) << "' is declared subtype of itself.");
    }
    else
    {
        assert(sub >= 0 && super >= 0); 
        boost::add_edge(super, sub, hierarchy);
    }
}

// remove all the subtype constraints involving t - this is used when a
// type is redefined (as in the patch files)
void undo_subtype_constraints(int t)
{
    boost::clear_out_edges(t, hierarchy);
    boost::clear_in_edges(t, hierarchy);
}

bool is_simple(tHierarchy g) {
    tHierarchy::vertex_iterator i, iend;
    tHierarchy::adjacency_iterator a, aend;
    tHierarchy::out_edge_iterator e, eend;
    for(boost::tie(i, iend) = boost::vertices(g); i != iend; ++i) {
        boost::tie(a, aend) = boost::adjacent_vertices(*i, g);
        boost::tie(e, eend) = boost::out_edges(*i, g);
        if ((aend - a) != (eend - e)) return false;
    }
    return true;
}

/**
 * functions to find parents and children of a given type
 */
/*@{*/
/** helper function object to return target of edge; it seems it's not possible
 * to use ptr_fun(boost::target)
 */
struct EdgeTargetExtractor : std::unary_function<tHierarchyEdge, tHierarchyVertex>
{
    tHierarchyVertex operator()(const tHierarchyEdge& e) const
    {
        return boost::target(e, hierarchy);
    }
};

/** helper function object to return source of edge; it seems it's not possible
 * to use ptr_fun(boost::source)
 */
struct EdgeSourceExtractor : std::unary_function<tHierarchyEdge, tHierarchyVertex>
{
    tHierarchyVertex operator()(const tHierarchyEdge& e) const
    {
        return boost::source(e, hierarchy);
    }
};
/*@}*/

/** return the list of all immediate subtypes of \a t */
list<int> immediate_subtypes(int t)
{
    std::pair<boost::graph_traits<tHierarchy>::out_edge_iterator,
              boost::graph_traits<tHierarchy>::out_edge_iterator>
        e(boost::out_edges(t, hierarchy));
    
    list<int> l;
    std::transform(e.first, e.second, std::front_inserter(l),
                   EdgeTargetExtractor());
    return l;
}

/** return the list of all immediate supertypes of \a t */
list<int> immediate_supertypes(int t)
{
    std::pair<boost::graph_traits<tHierarchy>::in_edge_iterator,
              boost::graph_traits<tHierarchy>::in_edge_iterator>
        e(boost::in_edges(t, hierarchy));

    list<int> l;
    std::transform(e.first, e.second, std::front_inserter(l),
                   EdgeSourceExtractor());
    return l;
}

/** @name Bitcode Computation */
/*@{*/
/** maps from bit position in the bitcode to corresponding type */
map<int,int> idbit_type;

/** compute the transitive closure encoding */
void compute_code_topo() 
{
    // the code to be assigned next
    int codenr = codesize - 1;
    
    // used to iterate over the children
    list<int> l; int c;
    
    // iterate over the nodes in the hierarchy in reverse topological order
    vector<int> topo;
    boost::topological_sort(hierarchy, std::back_inserter(topo));
    for(vector<int>::iterator it = topo.begin(); it != topo.end(); ++it)
    {
        // get the corresponding type id
        int current_type = *it;
      
        if(leaftypeparent[current_type] == -1) // leaf types don't get a code
        {
            // check if this has already been visited - cannot happen
            // if iterator works as expected
            if(types[current_type]->bcode != NULL)
              throw tError("conception error: " + types.name(current_type)
                           + " already visited...");
            
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
                  throw tError("conception error: " + types.name(c) 
                               + " not yet computed...");
                
                // combine with subtypes bitcode using binary or
                *types[current_type]->bcode |= *types[c]->bcode;
            }
        }
    }
}

/** Print all subtypes of bitcode \a b for debugging */
std::string debug_print_subtypes(bitcode *b) {
  list_int *l = b->get_elements();
  std::ostringstream out;
  for(list_int *c = l; c != NULL; c = rest(c)) {
    out << " " << types.name(idbit_type[first(c)]);
  }
           
  free_list(l);
  return out.str();
}

/*@}*/

/** Detect cyclic arcs using dfs: The existence of a back edge indicates a
 *  cycle.
 */
class cycleDetector : public boost::dfs_visitor<>
{
public:
    /** Create DFS visitor with reference to the return value \a has_cycle */
    cycleDetector(bool& has_cycle)
        : _has_cycle(has_cycle)
    { }

    /** If a back edge exists, this graph has a cycle. */
    template <class Edge, class Graph>
    void back_edge(Edge, Graph&)
    {
        _has_cycle = true;
    }
    
protected:
    bool& _has_cycle;
};

template<typename Graph>
bool
isAcyclic(Graph &G)
{
    bool hasCycle = false;
    cycleDetector vis(hasCycle);
    boost::depth_first_search(G, visitor(vis));
    return !hasCycle;
}

/** helper predicate: return fixed false/true */
template<typename T, bool V> struct fixedPred : std::unary_function<T, bool>
{
    bool operator()(const T& t) const
    {
        return V;
    }
};


// recompute hierarchy so it's a semilattice
// theoretical background: (Ait-Kaci et al., 1989)
void make_semilattice()
{
  int i, j, low, high;

  bool changed;

  LOG(logSemantic, DEBUG, "make_semilattice(1) at " << clock());

  // nr of synthesized glb types so far, old (total) number of types
  int glbtypes = 0, oldntypes = types.number();

  // scratch bitcode
  bitcode *temp = new bitcode(codesize);

  bool glbdebug;
  get_opt("opt_glbdebug", glbdebug);
  LOG(logApplC, INFO, "glbs ");

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
          // if result is all zeroes
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

           if(glbdebug)
           {
             LOG(logSemantic, DEBUG,
                 "Introducing " << name << " for " << types.name(i)
                 << " and " << types.name(j) << ":" << std::endl
                 << "[" << types.name(i) << "]:"
                 << debug_print_subtypes(types[i]->bcode) << std::endl
                 << "[" << types.name(j) << "]:"
                 << debug_print_subtypes(types[j]->bcode) << std::endl
                 << "[" << name << "]:" <<debug_print_subtypes(temp));
           }

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

  LOG(logApplC, INFO, "[" << glbtypes << "], ");

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

  LOG(logApplC, DEBUG, " (" << clock() << "), ");

  LOG(logApplC, INFO, "recomputing");

#ifdef NAIVE_RECOMPUTATION

  // this is the naive approach: look at all ordered pairs
  // and check if they're in subtype relation

  boost::remove_edge_if(fixedPred<tHierarchyEdge, true>(), hierarchy);
  
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
  if(!isAcyclic<tHierarchy>(hierarchy))
    {
      throw tError("conception error: new type hierarchy is cyclic...");
    }

  // now we have to remove the redundant links - this is just
  // computing the transitive reduction of the hierarchy

  acyclicTransitiveReduction(hierarchy);

  LOG(logSemantic, DEBUG, " (" << clock() << ")");

  // do a few sanity checks:
  
  if(!is_simple(hierarchy)) {
    throw tError("conception error - making hierarchy simple");
  }

#if 0
  if(!Is_Loopfree(hierarchy))
    {
      LOG(logSemantic, ERROR, "conception error - making hierarchy loopfree");
      Delete_Loops(hierarchy);
    }

  LOG(logSemantic, DEBUG, " (%ld)", clock());
#endif
}

inline bool simple_leaftype(int i)
// is `i' a simple leaftype (it has no children and exactly one parent)
{
    return boost::out_degree(i, hierarchy) == 0 &&
        boost::in_degree(i, hierarchy) == 1;
}

inline void mark_leaftype(int i)
// mark `i' as a leaftype
{
  leaftypeparent[i] = immediate_supertypes(i).front();
  // LOG(logSemantic, DEBUG, "LT: " << i << " [" << leaftypeparent[i] << "]");

  nstaticleaftypes++;
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

    vector<int> topo;
    boost::topological_sort(hierarchy, std::back_inserter(topo));
    for(vector<int>::iterator it = topo.begin(); it != topo.end(); ++it)
    {
        // get the corresponding type id
        i = *it;

        if(simple_leaftype(i))
        {
          //fprintf(stderr, "SLT %d %s\n", i, types.name(i).c_str());
            mark_leaftype(i);
        }
        else if (boost::in_degree(i, hierarchy) == 1) // one parent
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
              {
                //fprintf(stderr, "CLT %d\n", i);
                mark_leaftype(i);
              }
        }
    }

#endif
}


/** Recursively print all subtypes of a given type t */
void
print_subtypes(FILE *f, int t, HASH_SPACE::hash_set<int> &visited)
{
    if(visited.find(t) != visited.end())
        return;
    
    visited.insert(t);
    fprintf(f, " %s", types.name(t).c_str());

    list<int> children = immediate_subtypes(t);
    for(list<int>::iterator child = children.begin();
        child != children.end(); ++child)
    {
        print_subtypes(f, *child, visited);
    }
}


void
print_hierarchy(FILE *f)
{
    HASH_SPACE::hash_set<int> visited;
    for(int i = 1; i < types.number() ; i++)
    {
        visited.clear();
        fprintf(f, "%s:", types.name(i).c_str());
        print_subtypes(f, i, visited);
        fprintf(f, "\n");
    }
}


void propagate_status()
{
    struct type *t, *chld;
    
    LOG(logSemantic, INFO, "- status values");
    
    vector<int> topo;
    boost::topological_sort(hierarchy, std::back_inserter(topo));
    
    for(vector<int>::iterator it = topo.begin(); it != topo.end(); ++it)
    {
      t = types[*it];
        
        if(t->status != NO_STATUS)
        {
            boost::graph_traits<tHierarchy>::out_edge_iterator ei, ei_end;
            for(tie(ei, ei_end) = boost::out_edges(*it, hierarchy); ei != ei_end; ++ei)
            {
                chld = types[boost::target(*ei, hierarchy)];
                
                if(chld->defines_status == 0)
                    if(chld->status == NO_STATUS || !flop_settings->member("weak-status-values", statustable.name(t->status).c_str()))
                    {
                        if(chld->status != NO_STATUS &&
                           chld->status != t->status &&
                           !flop_settings->member("weak-status-values", statustable.name(chld->status).c_str()))
                        {
                            LOG(logSemantic, INFO,
                                "`" << types.name(chld->id)
                                << "': status `" << statustable.name(t->status)
                                << "' from `" << types.name(t->id)
                                << "' overwrites old status `"
                                << statustable.name(chld->status)
                                << "' from `" << types.name(chld->status_giver)
                                << "'...");
                        }
                        
                        chld->status_giver = t->id;
                        chld->status = t->status;
                    }
            }
        }
    }
}


bool process_hierarchy(bool propagate_status_p)
{
  int i;

  LOG(logApplC, INFO, "- type hierarchy (");

  // sanity check: is the hierarchy simple (contains no parallel edges)
  if(!is_simple(hierarchy))
    {
      LOG(logSemantic, WARN,
          "type hierarchy is not simple (should not happen)");
      assert(false);
      //Make_Simple(hierarchy);
    }

  // check for cyclicity:
  if(!isAcyclic(hierarchy))
  {
      LOG(logSemantic, WARN, "type hierarchy is cyclic.");
      return false;
  }
  
  // make all maximal types subtypes of TOP
  for(i = 1; i < types.number() ; i++)
  {
      if(boost::in_degree(i, hierarchy) == 0)
      {
          subtype_constraint(i, BI_TOP);
      }
  }

  LOG(logApplC, INFO, "leaf types ");

  // for each type t, leaftypeparent[t] is -1 if t is not a leaftype,
  // and the id of the parent type otherwise
  
  find_leaftypes();
  
  LOG(logApplC, INFO, "[" << nstaticleaftypes << "], "); 
  
  LOG(logApplC, INFO, "bitcodes, ");

  // codesize is number of non-leaf types
  codesize = types.number() - nstaticleaftypes;

  initialize_codes(codesize);
  compute_code_topo();
  
  for(i = 0; i < types.number(); i++) if(leaftypeparent[i] == -1)
    register_codetype(*types[i]->bcode, i);

  make_semilattice();

  LOG(logAppl, INFO, ")");

  if(propagate_status_p) propagate_status();

  return true;
}
