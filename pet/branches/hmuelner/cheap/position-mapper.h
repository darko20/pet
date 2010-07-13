/* -*- Mode: C++ -*- */
/** \file position_mapper.h
 * Contains a class to map input positions to chart positions and close gaps in
 * the chart as good as possible.
 */

#ifndef _POSITION_MAPPER_H
#define _POSITION_MAPPER_H

#include "item.h"
#include <map>

/** This class provides a mapping of input to chart positions.
 *
 * Besides reducing chart size by mapping, e.g., character counts to chart
 * positions, another task of this class is to avoid loose ends, possibly
 * created by the deletion of white space or punctuation on the side of the
 * tokenizer.
 *
 * To implement different heuristics that could then be selected from the
 * outside by providing the correct object to map_positions, this class might
 * be turned into an abstract class with different implementations for the
 * different heuristics.
 */
class position_mapper {
private:
  class poschartnode {
  private:
  public:
    inp_list in_edges, out_edges;
    int chartpos;
    void add_in_edge(tInputItem *item) { in_edges.push_back(item); }
    void add_out_edge(tInputItem *item) { out_edges.push_back(item); }

    void set_chartpositions() {
      for (inp_list::iterator item_iterator = this->in_edges.begin()
             ; item_iterator != this->in_edges.end()
             ; item_iterator++) {
        (*item_iterator)->set_end(chartpos);
      }
      for (inp_list::iterator item_iterator = this->out_edges.begin()
             ; item_iterator != this->out_edges.end()
             ; item_iterator++) {
        (*item_iterator)->set_start(chartpos);
      }
    }
  };

  typedef std::map<int, poschartnode *> posmap;

  unsigned int _counts_to_positions;

  posmap _chart;

  /** Get the chart node corresponding to input position \a pos, if there is
   *  one, or create one if necessary.
   */
  poschartnode *get_or_add_node(int pos) {
    poschartnode *chartnode;
    posmap::iterator it = _chart.find(pos);
    if (it == _chart.end()) {
      chartnode = new poschartnode();
      _chart.insert(std::pair<int, poschartnode *>(pos, chartnode));
    } else {
      chartnode = it->second;
    }
    return chartnode;
  }

  /** Stamp the chart positions into all input items stored in the chart */
  void set_all_chartpositions() {
    posmap::iterator it;
    for (it = _chart.begin(); it != _chart.end(); it++) {
      it->second->set_chartpositions();
    }
  }

public:
  /** Create a new \c position_mapper object.
   * \param counts if \c true, the input positions are counts rather than
   *        positions 
   */
  position_mapper(bool counts) {
    _counts_to_positions = (counts ? 1 : 0);
    _chart.clear();
  }

  /** Add an input item \c item to this position_mapper */
  void add(tInputItem *item) {
    poschartnode *node = get_or_add_node(item->startposition());
    node->add_out_edge(item);

    node = get_or_add_node(item->endposition() + _counts_to_positions);
    node->add_in_edge(item);
  }

  /** Compute the mapping and close eventual gaps. 
   * Now close the gaps: For every node with indegree greater than zero and
   * outdegree zero (except for the last one), look for the nearest node to
   * the right that has indegree greater than zero
   */
  int map_to_chart_positions() {
    posmap::iterator it;
    std::list<poschartnode *> pending_nodes;
    int currentpos = 0;
    for (it = _chart.begin(); it != _chart.end(); it++) {
      poschartnode *current_node = it->second;
      if (current_node->out_edges.empty()) {
        // This node has incoming edges only
        pending_nodes.push_back(current_node);
      } else {
        current_node->chartpos = currentpos;
        // make all pending nodes (with outdegree zero left to this node) end
        // in this node
        while (! pending_nodes.empty()) {
          poschartnode *pending = pending_nodes.front();
          pending_nodes.pop_front();
          pending->chartpos = currentpos;
        }
        currentpos++;
      }
    }
    // Make all nodes which have outdegree zero at the end of the chart end in
    // the same node
    while (! pending_nodes.empty()) {
      poschartnode *pending = pending_nodes.front();
      pending_nodes.pop_front();
      pending->chartpos = currentpos;
    }

    set_all_chartpositions();
    
    // Return the number of the rightmost chart node (the leftmost is zero)
    return currentpos;
  }  // end map_to_chart_positions
}; // end position_mapper


#endif
