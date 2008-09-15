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


/** A list of chart items */
typedef std::list< class tItem * > item_list;
/** Iterator for item_list */
typedef std::list< class tItem * >::iterator item_iter;
/** Iterator for const item list */
typedef std::list< class tItem * >::const_iterator item_citer;
/** A list of input items */
typedef std::list< class tInputItem * > inp_list;
/** Iterator for inp_list */
typedef std::list< class tInputItem * >::iterator inp_iterator;


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
  item_list _items;
  std::map<tChartVertex*, item_list > _vertex_to_starting_items;
  std::map<tChartVertex*, item_list > _vertex_to_ending_items;
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
  void remove_items(item_list items);

  /**
   * Returns a list of all items in this chart.
   * @param skip_blocked if \c true, blocked items are not returned
   * @param skip_pending_inflrs if \c true, items with pending inflectional
   *                            rules are not returned
   * @param skip list of items to be skipped
   */
  item_list
  items(bool skip_blocked = false, bool skip_pending_inflrs = false,
      item_list skip = item_list());

  /**
   * Returns a list of all items that are in the same chart cell as
   * the specified item (including the specified item).
   * @param skip_blocked if \c true, blocked items are not returned
   * @param skip_pending_inflrs if \c true, items with pending inflectional
   *                            rules are not returned
   * @param skip list of items to be skipped
   */
  item_list
  same_cell_items(tItem *item, bool skip_blocked = false,
      bool skip_pending_inflrs = false, item_list skip = item_list());

  /**
   * Returns a list of all items immediately succeeding the specified item.
   * @param skip_blocked if \c true, blocked items are not returned
   * @param skip_pending_inflrs if \c true, items with pending inflectional
   *                            rules are not returned
   * @param skip list of items to be skipped
   */
  item_list
  succeeding_items(tItem *item, bool skip_blocked = false,
      bool skip_pending_inflrs = false, item_list skip = item_list());

  /**
   * Returns a list of all items succeeding the specified item.
   * @param skip_blocked if \c true, blocked items are not returned
   * @param skip_pending_inflrs if \c true, items with pending inflectional
   *                            rules are not returned
   * @param skip list of items to be skipped
   */
  item_list
  all_succeeding_items(tItem *item, bool skip_blocked = false,
      bool skip_pending_inflrs = false, item_list skip = item_list());

  /**
   * Print chart items to stream \a out using \a aip, select active and
   * passive items with \a passives and \a actives.
   */
  void print(std::ostream &out, class tAbstractItemPrinter *printer = NULL,
             bool passives = true, bool actives = true, bool blocked = true);

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
  item_list starting_items();
  const item_list starting_items() const;
  //@}

  /**
   * Gets all items ending at this vertex.
   */
  //@{
  item_list ending_items();
  const item_list ending_items() const;
  //@}

};



/**
 * Utility class for tChart related conversions. This class will eventually
 * become obsolete when the old chart implementation is replaced by tChart.
 */
class tChartUtil
{
private:
  /** path within input items holding the form of the item */
  static list_int* _token_form_path;
  /** path within input items holding the id(s) for the item */
  static list_int* _token_id_path;
  /** path within input items holding the external surface start position */
  static list_int* _token_from_path;
  /** path within input items holding the external surface end position */
  static list_int* _token_to_path;
  /** path within input items holding the stem of the item */
  static list_int* _token_stem_path;
  /** path within input items holding the inflectional rules to apply */
  static list_int* _token_lexid_path;
  /** path within input items holding the name of the lexical entry */
  static list_int* _token_inflr_path;
  /** path within input items holding part-of-speech tags information */
  static list_int* _token_postags_path;
  /** path within input items holding part-of-speech probs information */
  static list_int* _token_posprobs_path;
  /** path within lexical items into which the list of input fss is unified */
  static list_int* _lexicon_tokens_path;
  /** last element of the list of input fss is unified */
  static list_int* _lexicon_last_token_path;
  /** CONTEXT path within the rule's feature structure representation. */
  static list_int* _context_path;
  /** INPUT path within the rule's feature structure representation. */
  static list_int* _input_path;
  /** OUTPUT path within the rule's feature structure representation. */
  static list_int* _output_path;
  /** POSCONS path within the rule's feature structure representation. */
  static list_int* _poscons_path;

  /**
   * Assigns int nodes to all items in the specified \a chart, fills the
   * list \a processed with all processed items (all which are connected to
   * the first start node), and returns the largest int node assigned.
   */
  static int assign_int_nodes(tChart &chart, item_list &processed);

public:

  /**
   * Initialize all path settings.
   */
  static void initialize();

  /** Get the path in lexical items into which input items should be unified. */
  static const list_int* lexicon_tokens_path();
  /** Get the CONTEXT path for chart mapping rules. */
  static const list_int* context_path();
  /** Get the INPUT path for chart mapping rules. */
  static const list_int* input_path();
  /** Get the OUTPUT path for chart mapping rules. */
  static const list_int* output_path();
  /** Get the POSCONS path for chart mapping rules. */
  static const list_int* poscons_path();

  /**
   * Create a new input item from a input feature structure.
   */
  static tInputItem* create_input_item(const fs &input_fs);

  /**
   * Create a token feature structure from an input item. Throws a tError
   * if the feature structure is not valid.
   */
  static fs create_input_fs(tInputItem* item);

  /**
   * Convert a list of input items to a tChart by setting the
   * appropriate start and end vertices of each input item.
   * \param[in] input_items the list of input items to be converted
   * \param[out] chart the result of the conversion
   */
  static void map_chart(std::list<tInputItem*> &input_items, tChart &chart);

  /**
   * Convert an input tChart into a list of input items by setting the
   * appropriate start and end vertices of each input item.
   * \param[in] chart the chart to be converted
   * \param[out] input_items the result of the conversion
   * \return the greatest int chart vertex in \a input_items
   */
  static int map_chart(tChart &chart, inp_list &input_items);

  /**
   * Convert a chart to a tChart by setting the
   * appropriate start and end vertices of each item.
   */
  static void map_chart(class chart &in, tChart &out);

  /**
   * Convert a tChart to a chart by setting the
   * appropriate start and end vertices of each item.
   */
  static int map_chart(tChart &in, class chart &out);

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

inline item_list
tChartVertex::starting_items()
{
  assert(_chart != NULL);
  tChartVertex* key = this;
  std::map<tChartVertex*, item_list >::const_iterator entry =
    _chart->_vertex_to_starting_items.find(key);
  if (entry == _chart->_vertex_to_starting_items.end())
    return item_list();
  else
    return entry->second;
}

inline const item_list
tChartVertex::starting_items() const
{
  assert(_chart != NULL);
  // "this" is const in const member functions, but since the keys in
  // the map are not declared as const, we have to cast "this":
  tChartVertex* key = const_cast<tChartVertex*>(this);
  std::map<tChartVertex*, item_list >::const_iterator entry =
    _chart->_vertex_to_starting_items.find(key);
  if (entry == _chart->_vertex_to_starting_items.end())
    return item_list();
  else
    return entry->second;
}

inline item_list
tChartVertex::ending_items()
{
  assert(_chart != NULL);
  tChartVertex* key = this;
  std::map<tChartVertex*, item_list >::const_iterator entry =
    _chart->_vertex_to_ending_items.find(key);
  if (entry == _chart->_vertex_to_ending_items.end())
    return item_list();
  else
    return entry->second;
}

inline const item_list
tChartVertex::ending_items() const
{
  assert(_chart != NULL);
  // "this" is const in const member functions, but since the keys in
  // the map are not declared as const, we have to cast "this":
  tChartVertex* key = const_cast<tChartVertex*>(this);
  std::map<tChartVertex*, item_list >::const_iterator entry =
    _chart->_vertex_to_ending_items.find(key);
  if (entry == _chart->_vertex_to_ending_items.end())
    return item_list();
  else
    return entry->second;
}



// =====================================================
// class tChartUtil
// =====================================================

inline const list_int*
tChartUtil::lexicon_tokens_path()
{
  return _lexicon_tokens_path;
}

inline const list_int*
tChartUtil:: context_path()
{
  return _context_path;
}

inline const list_int*
tChartUtil:: input_path()
{
  return _input_path;
}

inline const list_int*
tChartUtil:: output_path()
{
  return _output_path;
}

inline const list_int*
tChartUtil:: poscons_path()
{
  return _poscons_path;
}

#endif /*_FS_CHART_H_*/
