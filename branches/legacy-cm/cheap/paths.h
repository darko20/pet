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

/** \file paths.h
 * Class to maintain sets of permissible paths in word hypothesis graphs.
 */

#ifndef _PATHS_H_
#define _PATHS_H_

#include <list>
#include <set>

/** Maintain sets of permissible paths (in a word hypothesis graph)
 *  for items.  Each path is identified by an positive integer
 *  id. This class abstracts from a specific implementation of what
 *  amounts to set<int> functionality.  There is support for a special
 *  'wildcard' set of paths. It is constructed using the default
 *  constructor. The wildcard set is compatible with every set, except
 *  the set containing no paths. Note that this results in an
 *  important difference between tPaths() and tPaths(list<int>()). */
class tPaths
{
 public:
    /** Default constructor. Initialize to all paths. */
    tPaths();

    /** Construct from a list of integers. All integers must be positive
     *  (0 is allowed). */
    tPaths(const std::list<int> &);

    /** Compute new paths common to \a that. */
    tPaths common(const tPaths &that) const;

    /** Restrict this paths to those also in \a that paths. */
    void intersect(const tPaths &that);

    /** Determine if paths are compatible, i.e. if common(that) will
     *  be non empty, but usually in a more efficient way.
     */
    bool compatible(const tPaths &that) const;

    /** Get list of all paths. The wildcard path is represented by the 
     *  empty list, the empty set of paths by a list containing just a -1.
     */
    std::list<int> get() const;

 private:
    tPaths(const std::set<int> &paths)
        : _all(false), _paths(paths)
    { }

    inline bool inconsistent() const
    {
        return !all() && _paths.empty();
    }
    
    inline bool all() const
    {
        return _all;
    }

    bool _all;
    std::set<int> _paths;
};

#endif
