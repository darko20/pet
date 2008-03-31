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

/* class `chart' */

#include "cheap.h"
#include "chart.h"
#include "tsdb++.h"
#include "item-printer.h"
#include "hashing.h"

using namespace std;
using namespace HASH_SPACE;

//#define DEBUG

chart::chart(int len, auto_ptr<item_owner> owner)
    : _Chart(), _trees(), _readings(), _pedges(0),
      _Cp_start(len + 1), _Cp_end(len + 1),
      _Ca_start(len + 1), _Ca_end(len + 1),
      _Cp_span(len + 1),
      _item_owner(owner)
{
    for(int i = 0; i <= len; i++)
    {
        _Cp_span[i].resize(len + 1 - i);
    }
}

chart::~chart()
{
}

void chart::reset(int len)
{
    _pedges = 0;
    _Chart.clear();
    _Cp_start.clear(); _Cp_start.resize(len + 1);
    _Cp_end.clear();   _Cp_end.resize(len + 1);
    _Ca_start.clear(); _Ca_start.resize(len + 1);
    _Ca_end.clear();   _Ca_end.resize(len + 1);
    _Cp_span.clear();  _Cp_span.resize(len + 1);
    for (int i = 0; i <= len; i++) {
        _Cp_span[i].clear();
        _Cp_span[i].resize(len + 1 - i);
    }
}

void chart::add(tItem *it)
{
#ifdef DEBUG
    it->print(ferr);
    fprintf(ferr, "\n");
#endif

    _Chart.push_back(it);

    if(it->passive())
    {
        _Cp_start[it->start()].push_back(it);
        _Cp_end[it->end()].push_back(it);
        _Cp_span[it->start()][it->end()-it->start()].push_back(it);
        _pedges++;
    }
    else
    {
        if(it->left_extending())
            _Ca_start[it->start()].push_back(it);
        else
            _Ca_end[it->end()].push_back(it);
    }
}

/** A predicate testing the existence of some item in a hash_set */
class contained : public unary_function< bool, tItem *> {
public:
  contained(hash_set<tItem *> &the_set) : _set(the_set) { } 
  bool operator()(tItem *arg) { return _set.find(arg) != _set.end(); }
private:
  hash_set<tItem *> _set;
};

/** Remove the items in the set from the chart */
void chart::remove(hash_set<tItem *> &to_delete)
{ 
    _Chart.erase(remove_if(_Chart.begin(), _Chart.end(), contained(to_delete))
                 , _Chart.end());
    for(hash_set<tItem *>::const_iterator hit = to_delete.begin()
          ; hit != to_delete.end(); hit++) {
#ifdef DEBUG
      it->print(ferr);
      fprintf(ferr, "removed \n");
#endif
      
      tItem *it = *hit;
      if(it->passive())
        {
          _Cp_start[it->start()].remove(it);
          _Cp_end[it->end()].remove(it);
          _Cp_span[it->start()][it->end()-it->start()].remove(it);
          _pedges--;
        }
      else
        {
          if(it->left_extending())
            _Ca_start[it->start()].remove(it);
          else
            _Ca_end[it->end()].remove(it);
        }
    }
}

void chart::print(FILE *f)
{
    int i = 0;
    for(chart_iter pos(this); pos.valid(); pos++, i++)
    {
        (pos.current())->print(f);
        fprintf(f, "\n");
    }
}

void chart::print(tItemPrinter *f, bool passives, bool actives)
{
    int i = 0;
    for(chart_iter pos(this); pos.valid(); pos++, i++)
    {
      tItem *curr = pos.current();
      if ((curr->passive() && passives) || (! curr->passive() && actives))
        f->print(curr);
    }
}

void chart::get_statistics()
{
    // calculate aedges, pedges, raedges, rpedges
    chart_iter iter(this);
    
    long int totalsize = 0;
    
    while(iter.valid())
    {
        tItem *it = iter.current();
        
        if(it->trait() == INFL_TRAIT)
        {
            stats.medges++;
        }
        else if(it -> passive())
        {
            stats.pedges++;
            if(it -> result_contrib())
                stats.rpedges++;
            
            fs f = it -> get_fs();
            totalsize += f.size();
        }
        else
        {
            stats.aedges++;
            if(it -> result_contrib())
                stats.raedges++;
        }
        iter++;
    }
    stats.fssize = (stats.pedges > 0) ? totalsize / stats.pedges : 0;
}



/** Return \c true if the chart is connected using only edges considered \a
 *  valid, i.e., there is a path from the first to the last node.
 */
bool
chart::connected(item_predicate &valid) {
  vector<bool> reached(rightmost() + 1, false);
  queue<int> current;
  int pos;

  reached[0] = true;
  current.push(0);
  while(! reached[rightmost()] && ! current.empty()) {
    pos = current.front(); current.pop();
    for(list<tItem *>::iterator it = _Cp_start[pos].begin()
          ; it != _Cp_start[pos].end(); it++) {
      if (! reached[(*it)->end()] && valid(*it)) {
        reached[(*it)->end()] = true;
        current.push((*it)->end());
      }
    }
  }
  return reached[rightmost()];
}
