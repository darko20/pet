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

/* class representing a unification failure */

#ifndef _FAILURE_H_
#define _FAILURE_H_

#include "list-int.h"
#include "dag.h"

void print_path(FILE *f, list_int *path);

// this class represents one failure point in unification

class unification_failure
{
 public:
  enum failure_type { SUCCESS, CLASH, CONSTRAINT, CYCLE };
  
  unification_failure();
  unification_failure(const unification_failure &f);
  unification_failure(failure_type t, list_int *rev_path, int cost,
		      int s1 = -1, int s2 = -1, dag_node *cycle = 0, dag_node *root = 0);
  ~unification_failure();
  
  unification_failure &operator=(const unification_failure &f);
  
  void print(FILE *f) const;
  inline void print_path(FILE *f) const { ::print_path(f, _path); }

  inline failure_type type() const { return _type; }
  inline int s1() const { return _s1; }
  inline int s2() const { return _s2; }
  inline bool empty_path() const { return _path == 0; }
  inline list_int *path() const { return _path; }
  inline int cost() const { return _cost; }
  inline list<list_int *> cyclic_paths() const { return _cyclic_paths; }

 private:
  list_int *_path;
  failure_type _type;
  int _s1, _s2;
  int _cost;
  list<list_int *> _cyclic_paths;

  friend int compare(const unification_failure &, const unification_failure &);
};

inline bool operator<(const unification_failure &a, const unification_failure &b)
{ return compare(a,b) == -1; }

#endif
