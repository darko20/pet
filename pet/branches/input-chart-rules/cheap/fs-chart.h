/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 2007 Peter Adolphs
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

/**
 * \file fs-chart.h
 * A chart implementation with abstract chart vertices instead of integers.
 * \attention This chart implementation is more general than the old chart
 * implementation in chart.h . At the moment, it is only used for chart
 * mapping rules (cf. chart-mapping.h ).
 */ 

#ifndef _FS_CHART_H_
#define _FS_CHART_H_

#include "pet-config.h"

#include "errors.h"
#include "fs.h"
#include "hashing.h"

#include <list>
#include <map>
#include <vector>


// forward declarations:
class tItem;
class tChartVertex;
    
/**
 * Chart data structure for parsing, storing 
 * \link tChart::tItem chart items \endlink that are anchored
 * between (abstract) \link tChart::tChartVertex chart vertices \endlink.
 * \attention This chart implementation is currently only used for chart
 *            mapping rules. The older chart implementation for syntactic
 *            rules is defined in chart.h .
 */
class tChart
{
  
  friend class tItem; // TODO necessary??
  friend class tChartVertex; // TODO necessary??
  
private:
  std::list<tChartVertex*> _vertices;
  std::list<tItem*> _items;
  std::map<tChartVertex*, std::list<tItem*> > _vertex_to_starting_items;
  std::map<tChartVertex*, std::list<tItem*> > _vertex_to_ending_items;
  std::map<tItem*, tChartVertex* > _item_to_prec_vertex;
  std::map<tItem*, tChartVertex* > _item_to_succ_vertex;
  
  /**
   * Copy construction is disallowed.
   */
  tChart(const tChart &chart);
  
public:
  
  /**
   * Create a new empty chart.
   */
  tChart();
  
  /**
   * Destructs the chart, after clearing it.
   * \see clear()
   */
  virtual ~tChart();
  
  /**
   * Clear the chart, resetting it to an initial state. This also
   * deallocates memory for all contained vertices.
   * The memory for tItem objects is handled by tItem::default_owner(), if set.
   */
  void clear();
  
  /**
   * Adds an existing unconnected vertex to this chart. Hands over
   * the ownership of this vertex to the chart, i.e. the memory for
   * this vertex is from now on handled by the chart.
   * @return \a vertex (unchanged)
   */
  tChartVertex*
  add_vertex(tChartVertex *vertex);
  
  /**
   * Removes the specified vertex from the chart and deallocates its
   * memory. Dependent items are removed as well (without memory deallocation).
   */
  void remove_vertex(tChartVertex *vertex);
  
  /**
   * Returns a list of all vertices in this chart.
   */
  std::list<tChartVertex*> vertices();
  
  /**
   * Returns a list of all start vertices in this chart. Start vertices
   * are vertices without any preceding items. If \a connected is set to
   * \c true, only vertices with succeeding items are returned.
   */
  std::list<tChartVertex*> start_vertices(bool connected = true);
  
  /**
   * Returns a list of all end vertices in this chart. End vertices
   * are vertices without any succeeding items. If \a connected is set to
   * \c true, only vertices with preceding items are returned.
   */
  std::list<tChartVertex*> end_vertices(bool connected = true);
  
  /**
   * Adds the specified item to the chart, placed between the specified
   * preceding vertex \a prec and the succeeding vertex \a succ.
   * The memory for tItem objects is handled by tItem::default_owner(), if set.
   * @return \a item (unchanged)
   */
  tItem*
  add_item(tItem *item, tChartVertex *prec, tChartVertex *succ);
  
  /**
   * Removes the specified item from the chart.
   * The memory for tItem objects is handled by tItem::default_owner(), if set.
   */
  void remove_item(tItem *item);
  
  /**
   * Removes the specified items from the chart.
   * The memory for tItem objects is handled by tItem::default_owner(), if set.
   */
  void remove_items(std::list<tItem*> items);
  
  /**
   * Returns a list of all items in this chart.
   * @param skip_blocked if \c true, blocked items are not returned
   * @param skip list of items to be skipped
   */
  std::list<tItem*>
  items(bool skip_blocked=false, std::list<tItem*> skip = std::list<tItem*>());
  
  /**
   * Returns a list of all items that are in the same chart cell as
   * the specified item (including the specified item).
   * @param skip_blocked if \c true, blocked items are not returned
   * @param skip list of items to be skipped
   */
  std::list<tItem*>
  same_cell_items(tItem *item, bool skip_blocked=false,
      std::list<tItem*> skip = std::list<tItem*>());
  
  /**
   * Returns a list of all items immediately succeeding the specified item.
   * @param skip_blocked if \c true, blocked items are not returned
   * @param skip list of items to be skipped
   */
  std::list<tItem*>
  succeeding_items(tItem *item, bool skip_blocked=false,
      std::list<tItem*> skip = std::list<tItem*>());
  
  /**
   * Returns a list of all items succeeding the specified item.
   * @param skip_blocked if \c true, blocked items are not returned
   * @param skip list of items to be skipped
   */
  std::list<tItem*>
  all_succeeding_items(tItem *item, bool skip_blocked=false,
      std::list<tItem*> skip = std::list<tItem*>());
  
};



/**
 * Vertex in a \link tChart chart \endlink .  
 */
class tChartVertex
{
  friend class tChart;

private:
  tChart *_chart;
  
  /**
   * Creates a new vertex, belonging to no chart. Every vertex can
   * belong to at most one chart.
   * \see create()
   */
  tChartVertex();
  
  virtual ~tChartVertex();
  
  /**
   * Informs this vertex that it has been added to a chart.
   */
  void notify_added_to_chart(tChart *chart);
  
public:
  
  /**
   * Returns a new chart vertex, owned by no chart.
   * \param position the external character position in the input
   *                 (defaults to \c -1)
   * \see tChart::add_vertex()
   */
  static tChartVertex* create();
  
  /**
   * Gets all items starting at this vertex.
   */
  //@{
  std::list<tItem*> starting_items();
  const std::list<tItem*> starting_items() const;
  //@}
  
  /**
   * Gets all items ending at this vertex.
   */
  //@{
  std::list<tItem*> ending_items();
  const std::list<tItem*> ending_items() const;
  //@}
  
};



// ========================================================================
// INLINE DEFINITIONS
// ========================================================================

// =====================================================
// class tChart
// =====================================================

inline
tChart::tChart()
{
  // nothing to do
}

inline
tChart::~tChart()
{
  clear();
}

inline tChartVertex*
tChart::add_vertex(tChartVertex *vertex)
{
  _vertices.push_back(vertex);
  vertex->notify_added_to_chart(this);
  return vertex;
}

inline std::list<tChartVertex*>
tChart::vertices()
{
  return _vertices;
}



// =====================================================
// class tChartVertex
// =====================================================

inline
tChartVertex::tChartVertex()
:_chart(NULL)
{
  // nothing to do
}

inline
tChartVertex::~tChartVertex()
{
  // nothing to do
}

inline void
tChartVertex::notify_added_to_chart(tChart *chart)
{
  assert(_chart == NULL);
  _chart = chart;
}

inline tChartVertex*
tChartVertex::create()
{
  return new tChartVertex();
}

inline std::list<tItem*>
tChartVertex::starting_items()
{
  assert(_chart != NULL);
  tChartVertex* key = this;
  std::map<tChartVertex*, std::list<tItem*> >::const_iterator entry =
    _chart->_vertex_to_starting_items.find(key);
  if (entry == _chart->_vertex_to_starting_items.end())
    return std::list<tItem*>();
  else 
    return entry->second;
}

inline const std::list<tItem*>
tChartVertex::starting_items() const
{
  assert(_chart != NULL);
  // "this" is const in const member functions, but since the keys in
  // the map are not declared as const, we have to cast "this":
  tChartVertex* key = const_cast<tChartVertex*>(this);
  std::map<tChartVertex*, std::list<tItem*> >::const_iterator entry =
    _chart->_vertex_to_starting_items.find(key);
  if (entry == _chart->_vertex_to_starting_items.end())
    return std::list<tItem*>();
  else 
    return entry->second;
}

inline std::list<tItem*>
tChartVertex::ending_items()
{
  assert(_chart != NULL);
  tChartVertex* key = this;
  std::map<tChartVertex*, std::list<tItem*> >::const_iterator entry =
    _chart->_vertex_to_ending_items.find(key);
  if (entry == _chart->_vertex_to_ending_items.end())
    return std::list<tItem*>();
  else 
    return entry->second;
}

inline const std::list<tItem*>
tChartVertex::ending_items() const
{
  assert(_chart != NULL);
  // "this" is const in const member functions, but since the keys in
  // the map are not declared as const, we have to cast "this":
  tChartVertex* key = const_cast<tChartVertex*>(this);
  std::map<tChartVertex*, std::list<tItem*> >::const_iterator entry =
    _chart->_vertex_to_ending_items.find(key);
  if (entry == _chart->_vertex_to_ending_items.end())
    return std::list<tItem*>();
  else 
    return entry->second;
}

#endif /*_FS_CHART_H_*/
