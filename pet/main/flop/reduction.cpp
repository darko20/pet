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

/* compute transitive reduction of a graph - implementation by Kurt Mehlhorn*/

#include <LEDA/graph_alg.h>

#include "flop.h"

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