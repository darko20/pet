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

#include "bitcode.h"
#include "hierarchy.h"
#include <vector>
#include <boost/graph/topological_sort.hpp>

using namespace std;

/** Compare two integer indices using a vector of "positions".
 *  Given two integer indices, return \c true if the position of the first is
 *  greater than the position of the second
 */
class inv_position_compare {
  const vector<int> &_position;

public:
  inv_position_compare(const vector<int> &pos) : _position(pos) {}

  bool operator()(int i, int j) { return _position[i] > _position[j] ; }
};


/** Compute transitive reduction of an acyclic graph. This implements the
    algorithm from Mehlhorn: Data Structures and Algorithms, Vol II,
    pages 7 -- 9.
 */
void
acyclicTransitiveReduction(tHierarchy &G) {

  typedef boost::graph_traits<tHierarchy>::vertices_size_type v_size_type;
  v_size_type num_vertices = boost::num_vertices(G);

  std::vector<std::list<tHierarchyVertex> > closure(num_vertices);
  bitcode reached(num_vertices);

  for(tHierarchyVertex v = 0; v < num_vertices; ++v) {
    closure[v].push_back(v);
  }

  // iterate over the nodes in the hierarchy in topological order. Because of
  // the back_inserter, *top* is the last vertex in the vector
  vector<tHierarchyVertex> topo;
  boost::topological_sort(hierarchy, std::back_inserter(topo));

  // store the topo number of each vertex in this array: we need this to
  // compare the edges. vertices closer to *top* get bigger numbers
  vector<int> vertex_topo_position(num_vertices);
  int i = 0;
  for(vector<tHierarchyVertex>::iterator it = topo.begin()
        ; it != topo.end(); ++it) {
    vertex_topo_position[*it] = i++;
  }

  // Now walk through all the edges such that edges with source vertices closer
  // to the leaves come first; if source vertex is equal, those with target
  // closer to *top* come first, i.e., those who skip a smaller distance!
  for(vector<tHierarchyVertex>::iterator it = topo.begin()
        ; it != topo.end(); ++it) {
    tHierarchyVertex v = *it;

    reached.clear();
    reached.insert(v);   // for security's sake in case of loops
    
    std::vector<int> target(out_degree(v, G));
    boost::graph_traits<tHierarchy>::out_edge_iterator edge_it, edge_it_end;
    for (boost::tie(edge_it, edge_it_end) = out_edges(v, G), i = 0
           ; edge_it != edge_it_end ; edge_it++, i++) {
      target[i] = boost::target(*edge_it, G);
    }
    sort(target.begin(), target.end()
         , inv_position_compare(vertex_topo_position));
    
    for(vector<int>::iterator it = target.begin(); it != target.end(); it++) {
      tHierarchyVertex w = *it;
      if(! reached.member(w)) {  // e is in the transitive reduction
        std::list<tHierarchyVertex> &Rw = closure[w];
        for(std::list<tHierarchyVertex>::iterator itZ = Rw.begin()
              ; itZ != Rw.end(); ++itZ) {
          if(! reached.member(*itZ)) {
            reached.insert(*itZ);
            closure[v].push_back(*itZ);
          }            
        }
      } else {
        // This could be a potential bottleneck
        boost::remove_edge(v,w,G);
      }
    }
  }
}
