/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 2002 Ulrich Callmeier uc@coli.uni-sb.de
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

