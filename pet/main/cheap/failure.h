/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* class representing a unification failure */

#ifndef _FAILURE_H_
#define _FAILURE_H_

#include <stdio.h>
#include <list>
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
