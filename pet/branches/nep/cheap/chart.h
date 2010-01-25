/* -*- Mode: C++ -*-
 * PET
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

/** \file chart.h 
 * Chart data structure for parsing in dynamic programming style.
 */

#ifndef _CHART_H_
#define _CHART_H_

#include "item.h"
#include "hashing.h"

#include <string>
#include <list>
#include <memory>
#include <queue>

/** Chart data structure for parsing, aka dynamic programming */
class chart {
public:
  /** Create a chart for \a len words, and item_owner \a owner to handle the
   *  proper destruction of items on chart destruction.
   *  \attention The function length() will return \a len + 1.
   */
  chart(int len, std::auto_ptr<item_owner> owner);
  ~chart();

  /**
   * Resets the chart, removing all items, but leaving _trees and _readings
   * untouched.
   * This method is only needed for mapping the new chart (tChart) to this
   * old chart implementation.
   * TODO remove this method if tChart is not used anymore
   */
  void reset(int len);

  /** Add item to the appropriate internal data structures, depending on its
   *  activity.
   */
  void add(tItem *);

  /** Remove the item in the set from the chart */
  void remove(HASH_SPACE::hash_set<tItem *> &to_delete);

  /** Print chart items using \a aip.
   *  Enable/disable printing of passive, active and blocked items.
   */
  void print(tAbstractItemPrinter *aip, item_predicate toprint = alltrue) const;

  /** Print chart items to stream \a out using \a aip.
   *  Enable/disable printing of passive, active and blocked items.
   */
  void print(std::ostream &out, tAbstractItemPrinter *aip = NULL,
             item_predicate toprint = alltrue) const;

  /** Get statistics from the chart, like nr. of active/passive edges, average
   *  feature structure size, items contributing to a reading etc.
   */
  void get_statistics();

  /** Return the number of passive edges */
  inline int& pedges() { return _pedges; }

  /** Return the length of the chart */
  unsigned int length() { return (unsigned int) _Cp_start.size() ; }
  /** The number of the rightmost chart node */
  unsigned int rightmost() { return length() - 1; }

  /** Return the trees stored in the chart after unpacking */
  std::vector<tItem *> &trees() { return _trees; }
  /** Return the readings found during parsing */
  std::vector<tItem *> &readings() { return _readings; }

  /** Compute a shortest path through the chart based on the \a weight_fn
   *  function that returns a weight for every chart item.
   *
   * This function can for example be used to get the best partial results or
   * to extract an input item sequence. \c weight_t is the numeric type
   * returned by \a weight_fn, i.e., weight_fn has to be of type 
   * \code unary_function< tItem *, weight_t >
   * \param result a list of items constituting the minimum overall weight path
   * \param weight_fn a \code unary_function< tItem *, weight_t > \endcode
   *        determining the weight for a passive chart edge (smaller is better)
   * \param all if there is more than one optimal path, passing \c true will
   *        return all items on the optimal paths, otherwise, only the items on
   *        one of the optimal paths (the default)
   */
  template < typename weight_t, typename weight_fn_t >
  void shortest_path(std::list <tItem *> &result, weight_fn_t weight_fn
                     , bool all = false);

  /** Extract a surface string from the input items in this chart. This is
   *  in particular relevant where the input is given as a word lattice.
   */
  std::string get_surface_string();

  /** Return \c true if the chart is connected using only edges considered \a
   *  valid, i.e., there is a path from the first to the last node.
   */
  bool connected(item_predicate valid);

private:
  static int _next_stamp;

  std::vector<tItem *> _Chart;
  std::vector<tItem *> _trees;
  std::vector<tItem *> _readings;

  int _pedges;

  std::vector< std::list<tItem *> > _Cp_start, _Cp_end;
  std::vector< std::list<tItem *> > _Ca_start, _Ca_end;
  std::vector< std::vector < std::list<tItem*> > > _Cp_span;

  std::auto_ptr<item_owner> _item_owner;

  friend class chart_iter;
  friend class chart_iter_span_passive;
  friend class chart_iter_topo;
  friend class chart_iter_adj_active;
  friend class chart_iter_adj_passive;
  friend class chart_iter_filtered;
};

std::ostream &operator<<(std::ostream &out, const chart &ch) ;

/** Return all items from the chart.
 * \attention iterators must return items in order of `stamp', so the
 * `excursion' works.
 */
class chart_iter {
public:
  /** Create a new iterator for \a C */
  inline chart_iter(const chart *C) : _LI(C->_Chart), _curr(_LI.begin()) { }

  /** Create a new iterator for \a C */
  inline chart_iter(const chart &C) : _LI(C._Chart), _curr(_LI.begin()) { }

  /** Increase iterator */
  inline chart_iter &operator++(int) {
    ++_curr;
    return *this;
  }

  /** Is the iterator still valid? */
  inline bool valid() {
    return _curr != _LI.end();
  }

  /** If valid(), return the current item, \c NULL otherwise. */
  inline tItem *current() {
    if(valid())
      return *_curr;
    else
      return 0;
  }

protected:
  friend class chart;

  const std::vector<class tItem *> &_LI;
  std::vector<class tItem *>::const_iterator _curr;
};


// WAS: class chart_iter_filtered : protected chart_iter {

class chart_iter_filtered : public chart_iter {
private:
  item_predicate _to_include;

  /** Move to the next possible item that is not filtered */
  inline void proceed() {
    while (valid() && ! _to_include(*_curr)) ++_curr;
  }

public:
  /** create a new iterator for \a C, returning only those for which \a incl
   *  return \c true
   */
  inline chart_iter_filtered(const chart *C, item_predicate incl)
    : chart_iter(C), _to_include(incl) { proceed(); }

  /** see above */
  inline chart_iter_filtered(const chart &C, item_predicate incl)
    : chart_iter(C), _to_include(incl) { proceed(); }
  
  /** Increase iterator */
  inline chart_iter &operator++(int) {
    ++_curr;
    proceed();
    return *this;
  }
};

/** Return all passive items having a specified span.
 * \attention iterators must return items in order of `stamp', so the
 * `excursion' works.
 */
class chart_iter_span_passive {
public:
  /** Create an iterator for all passive items in \a C starting at \a i1 and
   *  ending at \a i2.
   */
  inline chart_iter_span_passive(chart *C, int i1, int i2) :
    _LI(C->_Cp_span[i1][i2-i1]) {
    _curr = _LI.begin();
  }

  /** Create an iterator for all passive items in \a C starting at \a i1 and
   *  ending at \a i2.
   */
  inline chart_iter_span_passive(chart &C, int i1, int i2) :
    _LI(C._Cp_span[i1][i2-i1]) {
    _curr = _LI.begin();
  }

  /** Increase iterator */
  inline chart_iter_span_passive &operator++(int) {
    ++_curr;
    return *this;
  }

  /** Is the iterator still valid? */
  inline bool valid() {
    return _curr != _LI.end();
  }

  /** If valid(), return the current item, \c NULL otherwise. */
  inline tItem *current() {
    if(valid())
      return *_curr;
    else
      return 0;
  }

private:
  item_list &_LI;
  item_iter _curr;
};


/** Return all passive in topological order, i.e., those with smaller start
 * position first.
 */
class chart_iter_topo {
private:
  inline void next() {
    while ((_curr == _LI[_currindex].end()) && ++_currindex && valid()) {
      _curr = _LI[_currindex].begin();
    }
  }

public:
  /** Create a new iterator for \a C */
  inline chart_iter_topo(chart *C) : _max(C->rightmost()), _LI(C->_Cp_start) {
    _currindex = 0;
    _curr = _LI[_currindex].begin();
    next();
  }

  /** Create a new iterator for \a C */
  inline chart_iter_topo(chart &C) : _max(C.rightmost()), _LI(C._Cp_start) {
    _currindex = 0;
    _curr = _LI[_currindex].begin();
    next();
  }

  /** Increase iterator */
  inline chart_iter_topo &operator++(int) {
    _curr++;
    next();
    return *this;
  }

  /** Is the iterator still valid? */
  inline bool valid() {
    return (_currindex <= _max);
  }

  /** If valid(), return the current item, \c NULL otherwise. */
  inline tItem *current() {
    return (valid() ? *_curr : 0);
  }

private:
  friend class chart;

  int _max, _currindex;
  std::vector< std::list< class tItem * > > &_LI;
  item_iter _curr;
};

/** Return all passive items adjacent to a given active item
 * \attention iterators must return items in order of `stamp', so the
 * `excursion' works.
 */
class chart_iter_adj_passive {
public:
  /** Create new iterator for chart \a C that returns the passive items
   *  adjacent to \a active.
   */
  inline
  chart_iter_adj_passive(chart *C, tItem *active)
    : _LI(active->left_extending() ?
          C->_Cp_end[active->start()] : C->_Cp_start[active->end()]) {
    _curr = _LI.begin();
  }

  /** Increase iterator */
  inline chart_iter_adj_passive &operator++(int) {
    ++_curr;
    return *this;
  }

  /** Is the iterator still valid? */
  inline bool valid() {
    return _curr != _LI.end();
  }

  /** If valid(), return the current item, \c NULL otherwise. */
  inline tItem *current() {
    return (valid() ? *_curr : 0);
  }

private:
  item_list &_LI;
  item_iter _curr;
};

/** Return all active items adjacent to a given passive item
 * \attention iterators must return items in order of `stamp', so the
 * `excursion' works.
 */
class chart_iter_adj_active {
  inline void overflow() {
    _at_start = false;
    _curr = _LI_end.begin();
  }

public:
  /** Create new iterator for chart \a C that returns the active items
   *  adjacent to \a passive.
   */
  inline chart_iter_adj_active(chart *C, tItem *passive)
    : _LI_start(C->_Ca_start[passive->end()]),
      _LI_end(C->_Ca_end[passive->start()]),
      _at_start(true) {
    _curr = _LI_start.begin();
    if(_curr == _LI_start.end()) overflow();
  }

  /** Increase iterator */
  inline chart_iter_adj_active &operator++(int) {
    ++_curr;
    if(_at_start && _curr == _LI_start.end())
      overflow();

    return *this;
  }

  /** Is the iterator still valid? */
  inline bool valid() {
    return _curr != _LI_end.end();
  }

  /** If valid(), return the current item, \c NULL otherwise. */
  inline tItem *current() {
    return (valid() ? *_curr : 0);
  }

private:
  item_list &_LI_start, &_LI_end;
    
  bool _at_start;
    
  item_iter _curr;
};


//
// shortest path algorithm for chart 
// Bernd Kiefer (kiefer@dfki.de)
//

/** Implemenation of shortest path function template */
template< typename weight_t, typename weight_fn_t > 
void chart::shortest_path(std::list <tItem *> &result, weight_fn_t weight_fn
                          , bool all) {
  // unary_function< tItem *, weight_t >
  std::vector<tItem *>::size_type size = _Cp_start.size() ;
  std::vector<tItem *>::size_type u, v ;

  std::vector < std::list < weight_t > > pred(size) ;

  weight_t *distance = new weight_t[size + 1] ;
  weight_t new_dist ;

  item_iter curr ;
  tItem *passive ;

  // compute the minimal distance and minimal distance predecessor nodes for
  // each node
  for (u = 1 ; u <= size ; u++) { distance[u] = UINT_MAX ; }
  distance[0] = 0 ;

  for (u = 0 ; u < size ; u++) {
    /* this is topologically sorted order */
    for (curr = _Cp_start[u].begin() ; curr != _Cp_start[u].end() ; curr++) {
      passive = *curr ;
      v = passive->end() ; new_dist = distance[u] + weight_fn(passive) ;
      if (distance[v] >= new_dist) {
        if (distance[v] > new_dist) {
          distance[v] = new_dist ;
          pred[v].clear() ;
        }
        pred[v].push_front(u) ;
      } 
    }
  }

  /** Extract all best paths */
  std::queue < weight_t > current ;
  bool *unseen = new bool[size + 1] ;
  for (u = 0 ; u <= size ; u++) unseen[u] = true ;

  current.push(size - 1) ;
  while (! current.empty()) {
    u = current.front() ; current.pop() ;
    for (curr = _Cp_end[u].begin() ; curr != _Cp_end[u].end() ; curr++) {
      passive = *curr ;
      v = passive->start() ;
      if ((find (pred[u].begin(), pred[u].end(), v) != pred[u].end())
          && (distance[u] == weight_fn(passive) + distance[v])) {
        result.push_front(passive) ;
        if (unseen[v]) { current.push(v) ; unseen[v] = false ; }
        if (! all) break; // only extract one path  
      }
    }
  }

  delete[] distance;
  delete[] unseen;
}

#endif
