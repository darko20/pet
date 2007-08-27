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

/** \file failure.h
 * Class representing a unification/subsumption failure 
 */

#ifndef _FAILURE_H_
#define _FAILURE_H_

#include "list-int.h"
#include "dag.h"

/** Print a list of attributes in \a path to file stream \a f */
void
print_path(FILE *f, list_int *path);

/** this class represents one failure point in unification/subsumption */

class failure
{
 public:
  /** The possible reasons for failure
      \li \c SUCCESS: A failure that doesn't deserve its name
      \li \c CLASH: failing type unification resp. subsumption
      \li \c CONSTRAINT: well-formedness unification failed
      \li \c CYCLE: the unification generated a cyclic feature structure
      \li \c COREF: subsumption failed due to a coreferenced
                    vs. non-coreferenced node.
  */
    enum failure_type { SUCCESS, CLASH, CONSTRAINT, CYCLE,
                        COREF };
  
    /** Default constructor */
    failure();
    /** Copy constructor (deep copy). */
    failure(const failure &f);
    /** Mostly used custom constructor.
     * \param t The type of failure.
     * \param rev_path The reverse feature path were the failure occured
     * \param cost The unification cost up to the failure point, up to now,
     *             this is the number of nodes already visited/unified.
     * \param s1 The first type involved in a \c CLASH or \c CONSTRAINT failure
     * \param s2 The second type involved in a \c CLASH or \c CONSTRAINT
     *           failure
     * \param cycle If set, the node where a \c CYCLE was detected.
     * \param root  If set, the root node of the dag where a \c CYCLE was
     *              detected 
     * 
     * When used to register subsumption failures, \a s1 is the type that
     * failed to subsume (be the supertype of) \a s2 in a \c CLASH.
     */
    failure(failure_type t, list_int *rev_path, int cost,
                        int s1 = -1, int s2 = -1,
                        dag_node *cycle = 0, dag_node *root = 0);
    ~failure();
  
    /** Assignment operator */
    failure &operator=(const failure &f);
  
    /** Print contents of object readably. */
    void print(FILE *f) const;
    /** Print the path where the failure occured */
    inline void print_path(FILE *f) const { ::print_path(f, _path); }

    /** Return failure type */
    inline failure_type type() const { return _type; }
    /** Return the first type involved */
    inline int s1() const { return _s1; }
    /** Return the second type involved */
    inline int s2() const { return _s2; }
    /** Is the failure path empty? */
    inline bool empty_path() const { return _path == 0; }
    /** return the failure path */
    inline list_int *path() const { return _path; }
    /** Return the cost of this failure */
    inline int cost() const { return _cost; }
    /** Return the list of paths pointing to cyclic structures */
    inline std::list<list_int *> cyclic_paths() const { return _cyclic_paths; }

    friend bool 
      operator<(const failure &a, const failure &b);

 private:
    list_int *_path;
    failure_type _type;
    int _s1, _s2;
    int _cost;
    std::list<list_int *> _cyclic_paths;

    int less_than(const failure &) const;
};

/** Return the common prefix of two unification failures paths */
inline bool
prefix(failure &a, failure &b)
{
    return prefix(a.path(), b.path());
}

/** partial order on unification failures */
inline bool
operator<(const failure &a, const failure &b)
{
    return a.less_than(b) == -1;
}

#endif
