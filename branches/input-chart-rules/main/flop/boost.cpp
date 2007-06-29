/* test of boost graph library */

#include <boost/config.hpp>

#include <algorithm>
#include <vector>
#include <utility>
#include <iostream>
#include <fstream>

#include <stdlib.h>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/reverse_graph.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/graph/graphviz.hpp>

#include "hierarchy.h"
void acyclicTransitiveReduction(tHierarchy &G);

int main()
{
    const int SIZE = 25;
    const int DENSITY = 3;

    srandom(time(NULL));
    
    std::cout << "Boost test" << std::endl;
    
    tHierarchy G;
    
    std::cout << " - Generating hierarchy with " << SIZE << " vertices" << std::endl;

    for(int i = 0; i < SIZE; ++i)
    {
        tHierarchyVertex v = boost::add_vertex(G);
        if(v != i)
            std::cout << " - Vertex " << i << " != " << v << std::endl;
    }

    std::cout << " - Adding " << SIZE * DENSITY << " edges" << std::endl;

  for(int i = 0; i < SIZE * DENSITY; ++i)
  {
      int v1, v2;
      do {
          v1 = random() % SIZE;
          v2 = random() % SIZE;
      } while(v1 >= v2);
      
      boost::add_edge(v1, v2, G);
  }

  std::cout << " - topological sort" << std::endl;

  std::vector<tHierarchyVertex> c;
  boost::topological_sort(G, std::back_inserter(c));

  for(int i = 0; i < c.size(); i++)
      std::cout << c[i] << std::endl; 
  
  std::ofstream g1("g1.dot");
  //  boost::print_graph(G, boost::get(boost::vertex_index, G));
  boost::write_graphviz(g1, G);

  acyclicTransitiveReduction(G);

  std::ofstream g2("g2.dot");
  //  boost::print_graph(G, boost::get(boost::vertex_index, G));
  boost::write_graphviz(g2, G);

  return 0;
  

  std::cout << std::endl << "reversed graph:" << std::endl;
  boost::print_graph(boost::make_reverse_graph(G), 
                     boost::get(boost::vertex_index, G));

  return 0;
}

