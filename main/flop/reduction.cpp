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

#include "hierarchy.h"
#include <vector>

#include <iostream>
#include <fstream>
#if 1
#include "flop.h"
#endif

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
  /** The %container has to be a random access container to use this
      output_iterator, and it has to have the right size, which is not
      nice at all.
  */
  template<typename _Container>
  class index_insert_iterator 
    : public std::iterator<std::output_iterator_tag, void, void, void, void>
    {
    protected:
      _Container* container;
      int pos;

    public:
      /// A nested typedef for the type of whatever container you used.
      typedef _Container          container_type;
      
      /// The only way to create this %iterator is with a container.
      explicit 
      index_insert_iterator(_Container& __x) : container(&__x), pos(0) { }

      /**
       *  @param  value  An instance of whatever type container_type::size_type
       *                 is, because it has to be an index into the container
       *  @return  This %iterator, for chained operations.
       *
       *  This iterator is a bit strange in that it does not store the value
       *  into the container at its position, but rather uses value as an index
       *  into the container and stores the current position there.
       */
      index_insert_iterator&
      operator=(typename _Container::const_reference __value) 
      { 
        (*container)[(int) __value] = pos;
        return *this;
      }

      /// Simply returns *this.
      index_insert_iterator& 
      operator*() { return *this; }

      /// Increment position and returns *this.
      index_insert_iterator& 
      operator++() { pos++ ; return *this; }

      /// return a copy of the current state after incrementing pos.
      index_insert_iterator
      operator++(int) { 
        index_insert_iterator __tmp = *this; 
        pos++;
        return *__tmp; 
      }
    };


public:
  /** Build a data structure to compare the vertices of \a G according to their
   *  topological order.
   */
  topologicalCompare(tHierarchy &G)
    : _G(G), _topos(boost::num_vertices(G), 0)
  {
    // This was a fatal error! We do not need the edges in topological order,
    // but the position in the order for every node
    // boost::topological_sort(_G, std::back_inserter(_ord));

    // _topos[type1] > _topos[type2] means type1 is potentially closer to
    // *top* than type1
    boost::topological_sort(_G, index_insert_iterator< vector<int> >(_topos));
  }

  /** 
      Compares two edges according to a topological order of the underlying
      graph. Returns true if the first edge precedes the second edge in the
      order. Edges are first sorted with source closer to the leaves, then 
      with target being closer to *top*.
  */
  bool
  operator()(const tHierarchyEdge &e1, const tHierarchyEdge &e2)
  {
    int s = _topos[boost::source(e2, _G)] - _topos[boost::source(e1, _G)];
    if (s != 0)
      {
        // edges have different source, put in decreasing order,
        // i.e. leaves come first: pos(source(e1)) < pos(source(e2))
        return s > 0;
      }
    else
      {
        // edges have identical source, put in increasing order, i.e.
        // targets up in the hierarchy (near to *top*) come first:
        // pos(target(e1)) > pos(target(e2))
        return _topos[boost::target(e1, _G)] > _topos[boost::target(e2, _G)];
      }
  }

private:
  tHierarchy &_G;
  std::vector<int> _topos;
};

/** Compute transitive reduction of an acyclic graph. This implements the
    algorithm from Mehlhorn: Data Structures and Algorithms, Vol II,
    pages 7 -- 9.
 */
void
acyclicTransitiveReduction(tHierarchy &G)
{
    
    // Obtain a list of the edges in G in topological order
    std::vector<tHierarchyEdge> allEdges;
    copy(boost::edges(G).first, boost::edges(G).second
         , std::back_inserter(allEdges));
    sort(allEdges.begin(), allEdges.end(), topologicalCompare(G));

    boost::graph_traits<tHierarchy>::vertices_size_type num_vertices
      = boost::num_vertices(G);

    std::vector<std::list<tHierarchyVertex> > closure(num_vertices);
    bool *reached = new bool[num_vertices];
    
    for(tHierarchyVertex v = 0; v < num_vertices; ++v)
    {
        closure[v].push_back(v);
    }

    tHierarchyVertex previous_v = num_vertices;
    for(std::vector<tHierarchyEdge>::iterator it = allEdges.begin()
          ; it != allEdges.end(); ++it)
    {

        tHierarchyVertex v = boost::source(*it, G);
        
        if(v != previous_v)
        {
            for(tHierarchyVertex z = 0; z < num_vertices; ++z)
                reached[z] = false;
            
            previous_v = v;
            reached[v] = true;   // for security's sake in case of loops
        }
        
        tHierarchyVertex w = boost::target(*it, G);
        if(!reached[w])
        {  // e is in the transitive reduction
 
            std::list<tHierarchyVertex> &Rw = closure[w];
            for(std::list<tHierarchyVertex>::iterator itZ = Rw.begin()
                  ; itZ != Rw.end(); ++itZ)
            {
                if(!reached[*itZ])
                {
                    reached[*itZ] = true;
                    closure[v].push_back(*itZ);
                }
            }
        }
        else 
        {
            G.remove_edge(*it);
        }
    }
    delete[] reached;
}

