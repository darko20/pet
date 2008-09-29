/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 2002 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* \file partition.cpp
 * Class to maintain disjoint-set data structure (partition of integers).
 */

#include "partition.h"
#include <cassert>

tPartition::tPartition(int n) 
{
    assert(n >= 0);
    
    for(int i = 0; i < n; i++)
        make_set(i);
}

void
tPartition::make_set(int x)
{
    _p[x] = x;
}

void
tPartition::link(int x, int y)
{
    if(_rank[x] > _rank[y])
    {
        _p[y] = x;
    }
    else
    {
        _p[x] = y;
        if(_rank[x] == _rank[y])
            _rank[y]++;
    }
}

int
tPartition::find_set(int x)
{
    if(x != _p[x])
    {
        _p[x] = find_set(_p[x]);
    }
    return _p[x];
}

//
// public methods
// 


bool
tPartition::same_set(int a, int b)
{
    return find_set(a) == find_set(b);
}

    
void
tPartition::union_sets(int a, int b)
{
    link(find_set(a), find_set(b));
}


void
tPartition::make_rep(int a)
{
    _rep[find_set(a)] = a;
}

    
int
tPartition::operator()(int a)
{
    int rep = find_set(a);
    std::map<int, int>::iterator it = _rep.find(rep);
    if(it != _rep.end())
        return it->second;
    else
        return rep;
}
