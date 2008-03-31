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

#include "fs-chart.h"

#include "item.h"

#include <algorithm>



// =====================================================
// Helper functions
// =====================================================

/**
 * Add all \a items that are not included in \a skip to \a result. 
 */
inline static void
filter_items(const std::list<tItem*> &items,
    bool skip_blocked,
    std::list<tItem*> &skip,
    std::list<tItem*> &result)
{
  for (std::list<tItem*>::const_iterator it = items.begin();
       it != items.end();
       it++)
  {
    if (!(skip_blocked && (*it)->blocked()) &&
        (find(skip.begin(), skip.end(), *it) == skip.end()))
    {
      result.push_back(*it);
    }
  }
}



// =====================================================
// class tChart
// =====================================================

void
tChart::clear()
{
  for (std::list<tChartVertex*>::iterator it = _vertices.begin();
       it != _vertices.end();
       it++)
    delete *it;
  _vertices.clear();
  _items.clear(); // items are usually released by the tItem::default_owner
  _vertex_to_starting_items.clear();
  _vertex_to_ending_items.clear();
  _item_to_prec_vertex.clear();
  _item_to_succ_vertex.clear();
}

std::list<tChartVertex*>
tChart::start_vertices(bool connected)
{
  std::list<tChartVertex*> vertices;
  for (std::list<tChartVertex*>::iterator it = _vertices.begin();
       it != _vertices.end();
       it++)
  {
    if ((*it)->ending_items().empty() &&
        (!connected || !(*it)->starting_items().empty()))
    {
      vertices.push_back(*it);
    }
  }
  return vertices;
}

std::list<tChartVertex*>
tChart::end_vertices(bool connected)
{
  std::list<tChartVertex*> vertices;
  for (std::list<tChartVertex*>::iterator it = _vertices.begin();
       it != _vertices.end();
       it++)
  {
    if ((*it)->starting_items().empty() &&
        (!connected || !(*it)->ending_items().empty()))
    {
      vertices.push_back(*it);
    }
  }
  return vertices;
}

tItem*
tChart::add_item(tItem *item, tChartVertex *prec, tChartVertex *succ)
{
  assert((item != NULL) && (prec != NULL) && (succ != NULL));
  assert((prec->_chart == this) && (succ->_chart == this));
  
  _items.push_back(item);
  _item_to_prec_vertex[item] = prec;
  _item_to_succ_vertex[item] = succ;
  _vertex_to_starting_items[prec].push_back(item);
  _vertex_to_ending_items[succ].push_back(item);
  
  item->notify_chart_changed(this);
  
  return item;
}

void
tChart::remove_item(tItem *item)
{
  assert(item != NULL);
  assert(item->_chart == this);
  assert(_item_to_prec_vertex.find(item) != _item_to_prec_vertex.end());
  
  _vertex_to_starting_items[item->prec_vertex()].remove(item);
  _vertex_to_ending_items[item->succ_vertex()].remove(item);
  _item_to_prec_vertex.erase(item);
  _item_to_succ_vertex.erase(item);
  _items.remove(item);
  
  item->notify_chart_changed(0);
  
  // items are usually released by the tItem::default_owner
}

void
tChart::remove_items(std::list<tItem*> items)
{
  for (std::list<tItem*>::iterator it = items.begin();
       it != items.end();
       it++)
  {
    remove_item(*it);
  }
}

std::list<tItem*>
tChart::items(bool skip_blocked, std::list<tItem*> skip)
{
  std::list<tItem*> result;
  filter_items(_items, skip_blocked, skip, result);
  return result;
}

std::list<tItem*>
tChart::same_cell_items(tItem *item, bool skip_blocked, std::list<tItem*> skip)
{
  std::list<tItem*> result;
  filter_items(item->prec_vertex()->starting_items(), skip_blocked,skip,result);
  return result;
}

std::list<tItem*>
tChart::succeeding_items(tItem *item, bool skip_blocked, std::list<tItem*> skip)
{
  std::list<tItem*> result;
  filter_items(item->succ_vertex()->starting_items(), skip_blocked,skip,result);
  return result;
}

std::list<tItem*>
tChart::all_succeeding_items(tItem *item, bool skip_blocked,
    std::list<tItem*> skip)
{
  std::list<tItem*> result;
  std::list<tChartVertex*> vertices;
  vertices.push_back(item->succ_vertex());
  for (std::list<tChartVertex*>::iterator vit = vertices.begin();
       vit != vertices.end();
       vit++)
  {
    std::list<tItem*> succ_items = (*vit)->starting_items();
    // schedule processing of all vertices not processed before:
    for (std::list<tItem*>::iterator iit = succ_items.begin();
         iit != succ_items.end();
         iit++)
    {
      tChartVertex *succ = (*iit)->succ_vertex();
      if (find(vertices.begin(), vertices.end(), succ) == vertices.end())
        vertices.push_back(succ);
    }
    // add filtered succeeding items to the result list:
    filter_items(succ_items, skip_blocked, skip, result);
  }
  return result;
}
