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

#include "chart.h"
#include "tsdb++.h"
#include "item-printer.h"
#include "logging.h"

using namespace std;
using namespace HASH_SPACE;

//#define PETDEBUG

chart::chart(int len, auto_ptr<item_owner> owner)
    : _Chart(), _trees(), _readings(), _pedges(0),
      _Cp_start(len + 1), _Cp_end(len + 1),
      _Ca_start(len + 1), _Ca_end(len + 1),
      _Cp_span(len + 1),
      _item_owner(owner)
{
    for(int i = 0; i <= len; ++i)
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
    for (int i = 0; i <= len; ++i) {
        _Cp_span[i].clear();
        _Cp_span[i].resize(len + 1 - i);
    }
}

void chart::add(tItem *it)
{
#ifdef PETDEBUG
  it->print(DEBUGLOGGER); DEBUGLOGGER << endl;
#endif

    _Chart.push_back(it);

    if(it->passive())
    {
        _Cp_start[it->start()].push_back(it);
        _Cp_end[it->end()].push_back(it);
        _Cp_span[it->start()][it->end()-it->start()].push_back(it);
        ++_pedges;
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
          ; hit != to_delete.end(); ++hit) {
#ifdef PETDEBUG
      it->print(DEBUGLOGGER); DEBUGLOGGER << "removed " << endl;
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

void chart::print(tAbstractItemPrinter *pr, item_predicate toprint) const {
  for(chart_iter_filtered pos(this, toprint); pos.valid(); ++pos) {
    pr->print(pos.current());
  }
}

void chart::print(std::ostream &out, tAbstractItemPrinter *pr,
                  item_predicate toprint) const {
  tItemPrinter def_print(out, LOG_ENABLED(logChart, INFO),
      LOG_ENABLED(logChart, DEBUG));
  if (pr == NULL) {
    pr = &def_print;
  }
  for(chart_iter_filtered pos(this, toprint); pos.valid(); ++pos) {
    out << endl;
    pr->print(pos.current());
  }
}

void chart::get_statistics(statistics &stats)
{
    // calculate aedges, pedges, raedges, rpedges
    chart_iter iter(this);

    long int totalsize = 0;

    while(iter.valid())
    {
        tItem *it = iter.current();

        if(!it->inflrs_complete_p())
        {
            ++stats.medges;
        }
        else if(it -> passive())
        {
            ++stats.pedges;
            if(it -> result_contrib())
                ++stats.rpedges;

            fs f = it -> get_fs();
            totalsize += f.size();
        }
        else
        {
            ++stats.aedges;
            if(it -> result_contrib())
                ++stats.raedges;
        }
        ++iter;
    }
    stats.fssize = (stats.pedges > 0) ? totalsize / stats.pedges : 0;
}

struct input_only_weights : public unary_function< tItem *, unsigned int > {
  unsigned int operator()(tItem * i) {
    // return a weight close to infinity if this is not an input item
    // prefer shorter items over longer ones
    if (dynamic_cast<tInputItem *>(i) != NULL)
      return i->span();
    else
      return 1000000;
  }
};

std::string
chart::get_surface_string() {
  list< tItem * > inputs;
  input_only_weights io;

  shortest_path<unsigned int>(inputs, io, false);
  string surface;
  for(item_citer it = inputs.begin(); it != inputs.end(); ++it) {
    const tInputItem *inp = dynamic_cast<const tInputItem *>(*it);
    if (inp != NULL) {
      surface = surface + inp->orth() + " ";
    }
  }

  int len = surface.length();
  if (len > 0) {
    surface.erase(surface.length() - 1);
  }
  return surface;
}

bool
chart::connected(item_predicate valid) {
  vector<bool> reached(rightmost() + 1, false);
  queue<int> current;
  int pos;

  reached[0] = true;
  current.push(0);
  while(! reached[rightmost()] && ! current.empty()) {
    pos = current.front(); current.pop();
    for(item_iter it = _Cp_start[pos].begin()
          ; it != _Cp_start[pos].end(); ++it) {
      if (! reached[(*it)->end()] && valid(*it)) {
        reached[(*it)->end()] = true;
        current.push((*it)->end());
      }
    }
  }
  return reached[rightmost()];
}

std::ostream &operator<<(std::ostream &out, const chart &ch) {
  ch.print(out);
  return out;
}
