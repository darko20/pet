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

/* Compute transitive reduction of an acyclic graph. */

#include <vector>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/topological_sort.hpp>
#include "hierarchy.h"

#include <iostream>
#include <fstream>

/** Binary predicate defining a strict weak topological ordering on edges.
 *  Two edges x and y are equivalent if both f(x, y) and f(y, x) are false.
 *  Invariants:
 *  - Irreflexivity: f(x, x) must be false.
 *  - Antisymmetry: f(x, y) implies !f(y, x)
 *  - Transitivity: f(x, y) and f(y, z) imply f(x, z).
 *  - Transitivity of equivalence: Equivalence (as defined above) is
 *    transitive: if x is equivalent to y and y is equivalent to z,
 *    then x is equivalent to z.
 */
class topologicalCompare
{
 public:
    topologicalCompare(tHierarchy &G)
        : _G(G)
    {
        boost::topological_sort(_G, std::back_inserter(_ord));
    }

    /** 
        Compares two edges according to a topological order of the underlying
        graph.Returns true if the first edge precedes the second edge in the
        order. Edges are first sorted decreasing by source, then increasing
        by target.
    */
    bool
    operator()(const tHierarchyEdge &e1, const tHierarchyEdge &e2)
    {
        int s = _ord[boost::source(e2, _G)] - _ord[boost::source(e1, _G)];
        if (s != 0)
        {
            // edges have different source, put in decreasing order.
            return s < 0;
        }
        else
        {
            // edges have identical source, put in increasing order.
            s = _ord[boost::target(e2, _G)] - _ord[boost::target(e1, _G)];
            return s > 0;
        }
    }

 protected:
    tHierarchy &_G;
    std::vector<int> _ord;
};

/** Compute transitive reduction of an acyclic graph. This implements the
    algorithm from Mehlhorn: Data Structures and Algorithms, Vol II, pages 7 -- 9. */
void
acyclicTransitiveReduction(tHierarchy &G)
{
    
    // Obtain a list of the edges in G in topological order
    std::vector<tHierarchyEdge> allEdges;
    copy(boost::edges(G).first, boost::edges(G).second, std::back_inserter(allEdges));
    sort(allEdges.begin(), allEdges.end(), topologicalCompare(G));

#ifdef DEBUG
    std::ofstream f0("new.top");
#endif
   
    std::vector<int> ord;
    boost::topological_sort(G, std::back_inserter(ord));
    
#ifdef DEBUG
    for(int i = 0; i < boost::num_vertices(G) ; i++)
    {
        f0 << "T " << i << ": " << ord[i] << "\n";
    }

    for(std::vector<tHierarchyEdge>::iterator it = allEdges.begin(); it != allEdges.end(); ++it)
    {
        f0 << "(" << ord[boost::source(*it, G)] << "," << ord[boost::target(*it, G)] << ") [" << *it << "]" << std::endl;
    }
    
    std::cout << std::endl;
#endif

    std::vector<std::list<tHierarchyVertex> > closure(boost::num_vertices(G));
    std::vector<bool> reached(boost::num_vertices(G));
    
    for(tHierarchyVertex v = 0; v < boost::num_vertices(G); ++v)
    {
        closure[v].push_back(v);
    }

#ifdef DEBUG
    std::ofstream f1("new.red");
#endif

    tHierarchyVertex previous_v = boost::num_vertices(G);
    for(std::vector<tHierarchyEdge>::iterator it = allEdges.begin(); it != allEdges.end(); ++it)
    {
        tHierarchyVertex v = boost::source(*it, G);
        
        if(v != previous_v)
        {
            for(tHierarchyVertex z = 0; z < boost::num_vertices(G); ++z)
                reached[z] = false;
            
            previous_v = v;
        }
        
        tHierarchyVertex w = boost::target(*it, G);
#ifdef DEBUG
        f1 << v << " - " << w;
#endif
        if(!reached[w])
        {  // e is in the transitive reduction
#ifdef DEBUG
            f1 << " R";
#endif
 
            std::list<tHierarchyVertex> &Rw = closure[w];
            for(std::list<tHierarchyVertex>::iterator itZ = Rw.begin(); itZ != Rw.end(); ++itZ)
            {
                if(!reached[*itZ])
                {
                    reached[*itZ] = true;
                    closure[v].push_back(*itZ);
                }
            }
        }
#ifdef DEBUG
        f1 << std::endl;
#endif
    }
}

#if 0

class compare_edges : public leda_cmp_base<leda_edge>
{
  const leda_graph& G;
  const leda_node_array<int> ord;

public:

  compare_edges(const leda_graph& _G, const leda_node_array<int>& _ord) :
    G(_G), ord(_ord) {} 
  
  int operator()(const leda_edge& e, const leda_edge& f) const 
    { 
      int s = compare(ord[G.source(e)], ord[G.source(f)]);
      if ( s != 0 ) return -s; // edges with larger source come first
      // for equal source edges are ordered by target number
      return compare(ord[G.target(e)], ord[G.target(f)]);
    }

  virtual ~compare_edges() {};
};

void ACYCLIC_TRANSITIVE_REDUCTION(const leda_graph& G, leda_edge_array<bool>& in_reduction)
/* marks all edges in the transitive reduction of G
   The  algorithm is as described in Mehlhorn: Data Structures and Algorithms, 
   Vol II, pages 7 -- 9. 
   G must be acyclic. */
{
  leda_node_array<int> ord(G);
  TOPSORT(G,ord);  // numbered starting at one

  compare_edges cmp(G,ord);

  leda_list<leda_edge> E = G.all_edges();
  E.sort(cmp);

  leda_edge e;
  leda_node v;

#ifdef DEBUG
  std::ofstream f0("old.top");

  forall_nodes(v, G)
  {
      f0 << "T " << hierarchy.inf(v) << ": " << ord[v] << std::endl;
  }

  forall(e, E)
  {
      f0 << "(" << ord[G.source(e)] << "," << ord[G.target(e)] << ") [" << hierarchy.inf(G.source(e)) << "," << hierarchy.inf(G.target(e)) << "]" << std::endl;
  }
#endif
  
  leda_node_array<leda_list<leda_node> > closure(G);

  forall_edges(e,G) in_reduction[e] = false;

  leda_node_array<bool> reached(G,false);
  leda_node previous_v = nil;

  forall_nodes(v,G) closure[v].append(v);

#ifdef DEBUG
  std::ofstream f1("old.red");
#endif

  forall(e,E) 
    {
      leda_node v = G.source(e);
      
      if ( v != previous_v)
	{
	  leda_node z;
	  forall_nodes(z,G) reached[z] = false;
	  
	  previous_v = v;
	}
      
      leda_node w = G.target(e);

#ifdef DEBUG
      f1 << hierarchy.inf(v) << " - " << hierarchy.inf(w);
#endif

      if ( ! reached[w] )
	{ // e is an edge of the transitive reduction
   
#ifdef DEBUG 
         f1 << " R";
#endif

	  in_reduction[e] = true;

	  leda_node z;
	  
	  leda_list<leda_node>& Rw = closure[w];

	  forall(z,Rw) 
	    {
	      if ( !reached[z] )
		{
		  reached[z] = true;
		  closure[v].append(z);
		}
	    }
	}
#ifdef DEBUG
      f1 << endl;
#endif 
    }
}

#endif
