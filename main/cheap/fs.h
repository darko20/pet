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

/* feature structures */

#ifndef _FS_H_
#define _FS_H_

#include "types.h"
#include "dag.h"

// modification list: a list of pairs (path, type)
typedef list<pair<string, type_t> > modlist;

class fs
{
 public:

  fs(int type);
  fs(char *path, int type);

  inline fs()
    { _dag = 0; _temp = 0; }

  inline fs(struct dag_node *dag, int temp = 0)
    { _dag = dag; _temp = temp; }

  inline fs(const fs &f)
    { _dag = f._dag; _temp = f._temp; }

  inline ~fs() {};
  
  inline fs &operator=(const fs &f)
  { _dag = f._dag; _temp = f._temp; return *this; }

  inline bool valid() { return _dag != 0 && _dag != FAIL; }

  inline bool operator==(const fs &f)
  { return _dag == f._dag; }

  inline bool operator!=(const fs &f)
  { return _dag != f._dag; }

  fs get_attr_value(int attr);
  fs get_attr_value(char *attr);

  fs get_path_value(const char *path);

  inline fs nth_arg(int n) {
    if(_temp)
      return fs(dag_nth_arg_np(_dag, n), _temp);
    else
      return fs(dag_nth_arg(_dag, n)); }

  char *name();
  char *printname();

  int type() { return dag_type(_dag); }
  void set_type(int s) { dag_set_type(_dag, s); }

  inline int size() { return 1; /* dag_size(_dag); */ }
  
  inline dag_node *dag() { return _dag; }
  inline int temp() const { return _temp; }

  inline void set_temp(int gen) { _temp = gen; }

  void restrict(list_int *del);

  void expand() { _dag = dag_expand(_dag); }

  bool modify(modlist &mods);

  void print(FILE *f);

 private:
  
  struct dag_node *_dag;
  int _temp;

  friend fs unify_restrict(fs &root, const fs &, fs &, list_int *del = 0, bool stat = true);
  friend fs copy(const fs &);
  friend bool compatible(const fs &, const fs &);
  friend type_t *get_qc_vector(qc_node *qc_paths, int qc_len, const fs &);

  friend fs unify_np(fs &root, const fs &, fs &);
  friend void subsumes(const fs &a, const fs &b, bool &forward, bool &backward);

  friend fs packing_partial_copy(const fs &a, list_int *del, bool perm);

  friend int compare(const fs &, const fs &);
  friend bool operator<(const fs &, const fs &);
  friend bool operator>(const fs &, const fs &);
};

inline bool operator<(const fs &a, const fs &b)
{ return a._dag < b._dag; }

inline bool operator>(const fs &a, const fs &b)
{ return a._dag > b._dag; }

extern fs unify_restrict(fs &root, const fs &, fs &, list_int *del, bool stat);
inline fs unify(fs &root, const fs &a, fs &b) { return unify_restrict(root, a, b, 0, false); }

extern fs unify_np(fs &root, const fs &, fs &);

// Caution: this function does not reset forward and backward on entry.
// If forward or backward are set to false on entry, subsumption in that
// direction is not checked.
extern void subsumes(const fs &a, const fs &b, bool &forward, bool &backward);

fs packing_partial_copy(const fs &a, list_int *del, bool perm);

extern fs copy(const fs &);
extern bool compatible(const fs &, const fs &);

extern void get_unifier_stats();

extern type_t *get_qc_vector(qc_node *qc_paths, int qc_len, const fs &);

extern bool
qc_compatible_unif(int qc_len, type_t *, type_t *);

extern void
qc_compatible_subs(int qc_len, type_t *a, type_t *b,
                   bool &forward, bool &backward);

//
// feature structure memory allocation
//

// allocation state

class fs_alloc_state
{
 public:
    inline fs_alloc_state(bool auto_release = true)
        : _auto(auto_release)
    {
        dag_alloc_mark(_state);
    }
    
    inline ~fs_alloc_state()
    {
        if(_auto)
        {
            dag_alloc_release(_state);
        }
    }

    inline void release()
    {
        dag_alloc_release(_state); _auto = false;
    }

    inline void clear_stats()
    {
        dag_alloc_clear_stats();
    }
    
    long dynamic_usage()
    {
        return dag_alloc_dynamic_mem();
    }
    
    long static_usage()
    {
        return dag_alloc_static_mem();
    }
    
    void reset()
    {
        dag_alloc_reset();
    }
    
    void may_shrink()
    {
        dag_alloc_may_shrink();
    }
    
 private:
    struct dag_alloc_state _state;
    bool _auto;
};

#ifdef QC_PATH_COMP

//
// computing of failure paths for the quickcheck
// only supported with tomabechi unifier
//

#include "failure.h"

extern map<unification_failure, int> failure_id;
extern map<int, unification_failure> id_failure;
extern map<int, double> failing_paths_unif;
extern map<list_int *, int, list_int_compare> failing_sets_unif;
extern map<int, double> failing_paths_subs;
extern map<list_int *, int, list_int_compare> failing_sets_subs;

#endif

#endif
