/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
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

/** \file fs-chart-util.h
 * Utility classes for tChart objects.
 */

#ifndef _FS_CHART_UTIL_H_
#define _FS_CHART_UTIL_H_

#include "pet-config.h"

#include "chart.h"
#include "errors.h"
#include "fs.h"
#include "fs-chart.h"
#include "hashing.h"
#include "item.h"

#include <list>


/**
 * Printer utility class for tChart objects.
 */
class tChartPrinter
{
public:
  
  /** Print a chart. */
  static void print(FILE *file, tChart &chart);
   
};


/**
 * Utility class for tChart related conversions. This class will eventually
 * become obsolete when the old chart implementation is replaced by tChart.
 */
class tChartUtil
{
private:
  /** path within input items holding the form of the item */
  static list_int* _inpitem_form_path;
  /** path within input items holding the list of ids for the item */
  static list_int* _inpitem_ids_path;
  /** path within input items holding the external character start position */
  static list_int* _inpitem_cfrom_path;
  /** path within input items holding the external character end position */
  static list_int* _inpitem_cto_path;
  /** path within input items holding the stem of the item */
  static list_int* _inpitem_stem_path;
  /** path within input items holding the inflectional rules to apply */
  static list_int* _inpitem_lexid_path;
  /** path within input items holding the name of the lexical entry */
  static list_int* _inpitem_inflr_path;
  /** path within input items holding part-of-speech tags information */
  static list_int* _inpitem_postags_path;
  /** path within input items holding part-of-speech probs information */
  static list_int* _inpitem_posprobs_path;
  /** path within lexical items into which the input fs is unified */
  static list_int* _lexitem_inpitem_path;
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
  static int assign_int_nodes(tChart &chart, std::list<tItem*> &processed);
  
public:
  
  /**
   * Initialize all path settings.
   */
  static void initialize();
  
  /** Get the path in lexical items into which input items should be unified. */
  static const list_int* lexitem_inpitem_path();
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
  static void map_chart(chart &in, tChart &out);
  
  /**
   * Convert a tChart to a chart by setting the
   * appropriate start and end vertices of each item.
   */
  static int map_chart(tChart &in, chart &out);
   
};



// ========================================================================
// INLINE DEFINITIONS
// ========================================================================

// =====================================================
// class tChartUtil
// =====================================================

inline const list_int*
tChartUtil::lexitem_inpitem_path()
{
  return _lexitem_inpitem_path;
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

#endif /*_FS_CHART_UTIL_H_*/
