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

#include "pet-system.h"
#include "paths.h"

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

