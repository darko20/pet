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

/* Implementation of class to maintain sets of permissible paths in word
   hypothesis graphs */

#include "paths.h"
#include <algorithm>

tPaths::tPaths()
    : _all(true), _paths()
{
}

tPaths::tPaths(const list<int> &p)
    : _all(false), _paths(p.begin(), p.end())
{
}

tPaths
tPaths::common(const tPaths &that) const
{
    if(all())
        return tPaths(*this);
    else if(that.all())
        return tPaths(that);
    
    set<int> result;
    set_intersection(_paths.begin(), _paths.end(),
                     that._paths.begin(), that._paths.end(),
                     inserter(result, result.begin()));

    return tPaths(result);
}

template < typename Ctain > 
void set_intersection_1(Ctain c1, typename Ctain::iterator start1
                        , typename Ctain::iterator end1
                        , typename Ctain::iterator start2
                        , typename Ctain::iterator end2) {
  typename Ctain::iterator it2 = start2;
  for(typename Ctain::iterator it1 = start1; it1 != end1; it1++) {
    while ((*it2 < *it1) && (it2 != end2)) it2++;
    if (it2 == end2) return;
    if (*it1 != *it2)
      c1.erase(it1);
    else 
      it1++;
  }
}

void
tPaths::intersect(const tPaths &that)
{
    if(all()) { 
      _all = that._all ; 
      _paths = that._paths ;
    }
    else if(! that.all()) {
      set_intersection_1(_paths, _paths.begin(), _paths.end()
                         , that._paths.begin(), that._paths.end());
    }
}

bool
tPaths::compatible(const tPaths &that) const
{
    if(inconsistent() || that.inconsistent())
        return false;
    if(all() || that.all())
        return true;
    
    tPaths tmp(common(that));
    
    return !tmp._paths.empty();
}

list<int>
tPaths::get() const
{
    if(inconsistent())
    {
        list<int> result;
        result.push_back(-1);
        return result;
    }
    else if(all())
    {
        return list<int>();
    }

    list<int> result(_paths.begin(), _paths.end());
    
    return result;
}

