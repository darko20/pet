/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 2002 Ulrich Callmeier uc@coli.uni-sb.de
 */

/** \file partition.h
 * Class to maintain disjoint-set data structure (partition of integers).
 */

#ifndef _PARTITION_H_
#define _PARTITION_H_

#include <map>

/** Disjoint-set data structure (Cormen, Leiserson, Rivest, Stein, Chapter 21).
 *  Additionally allows chosing the representative of each set.
 */
class tPartition
{
 public:
    /** \brief Construct and initialize to disjoint sets [0 .. n[.
     *  n must be >= 0. */
    tPartition(int n);

    /** Return true if a and b are in the same set. */
    bool
    same_set(int a, int b);
    
    /** Unite sets containing a and b. */
    void
    union_sets(int a, int b);
    
    /** Make a the representative of the block containing a. */
    void
    make_rep(int a);
    
    /** Return the representative of the block containing a. */
    int
    operator()(int a);

 protected:

    /** Create new set whose only member (and thus representative) is a.
     *  We require that a not already be in some other set. */
    void
    make_set(int a);

    /** Helper function for union_sets. */
    void
    link(int a, int b);
    
    /** Return representative of set containing x. No mapping is done. */
    int
    find_set(int a);
 
    std::map<int, int> _p;
    std::map<int, int> _rank;
    std::map<int, int> _rep;
};

#endif
