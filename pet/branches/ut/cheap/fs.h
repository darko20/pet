/* -*- Mode: C++ -*- */
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

/** \file fs.h
 * Feature Structures: An adapter/wrapper for dag implementations.
 */

#ifndef _FS_H_
#define _FS_H_

#include "pet-config.h"

#include "types.h"
#include "dag.h"

/** Features structure modification list: a list of pairs (path, type).
 * In these modlists, the path is represented by a string where the feature
 * names are separated by dots ('.')
 */
typedef std::list< std::pair<std::string, type_t> > modlist;
typedef type_t *qc_vec;

/** Adapter for different dag implementations */
class fs
{
  /** @name global variables for quick check */
  /*@{*/
  /** Compact representation of unification/subsumption quick check paths as
      tree of qc_node nodes */
  static qc_node *_qc_paths_unif, *_qc_paths_subs;
  /** Number of the unification/subsumption quick check paths to consider */
  static int _qc_len_unif, _qc_len_subs;
  /*@}*/

 public:

  /** Construct fs from typedag of \a type */
  fs(type_t type);
  /** Construct minimal fs containing \a path ending in typedag for \a type.
   *  \attention there are no checks whether the resulting fs is valid!
   */
  fs(const char *path, type_t type);
  /** Construct minimal fs containing \a path ending in typedag for \a type.
   *  \attention there are no checks whether the resulting fs is valid!
   */
  fs(const list_int *path, type_t type);

  /** Default constructor */
  inline fs(struct dag_node *dag = NULL, int temp = 0)
    { _dag = dag; _temp = temp; }

  /** Copy constructor: flat copy. */
  inline fs(const fs &f)
    { _dag = f._dag; _temp = f._temp; }

  /** Destructor */
  inline ~fs() {};

  /** Assignment operator: flat copy */
  inline fs &operator=(const fs &f)
  { _dag = f._dag; _temp = f._temp; return *this; }

  /** return \c true if the underlying dag is valid (not empty or \c FAIL) */
  inline bool valid() const { return _dag != 0 && _dag != FAIL; }

  /** return \c true if dags are eq */
  inline bool operator==(const fs &f) const { return _dag == f._dag; }

  /** return \c true if dags are not eq */
  inline bool operator!=(const fs &f) const { return _dag != f._dag; }

  /** Return a new fs representing the subdag under \a attr, if this Attribute
      is in the root fs. */
  fs get_attr_value(int attr) const;
  /** Return a new fs representing the subdag under \a attr, if this Attribute
      is in the root fs. */
  fs get_attr_value(const char *attr) const;

  /** Return a new fs representing the subdag under \a path, if this Path
      exists in the fs. */
  fs get_path_value(const list_int *path) const;
  /** Return a new fs representing the subdag under \a path, if this Path
      exists in the fs. */
  fs get_path_value(const char *path) const;

  /** Return the \a n th subdag in the \c ARGS list */
  inline fs nth_arg(int n) const {
    if(_temp)
      return fs(dag_nth_arg_temp(_dag, n), _temp);
    else
      return fs(dag_nth_arg(_dag, n)); }

  /** Return the \a n th subdag in the list under attribute \a attr. */
  inline fs nth_value(int attr, int n) const {
    if(_temp)
      return fs(dag_nth_element_temp(_dag, attr, n));
    else
      return fs(dag_nth_element(_dag, attr, n));
  }

  /** Return the \a n th subdag in the list under path \a path. */
  inline fs nth_value(const list_int *path, int n) const {
    if(_temp)
      return fs(dag_nth_element_temp(_dag, const_cast<list_int*>(path), n));
    else
      return fs(dag_nth_element(_dag, const_cast<list_int*>(path), n));
  }

  /**
   * Find all paths that end in a type that is a subtype of \a maxapp.
   * The paths must be freed by the caller.
   */
  inline std::list<list_int*> find_paths(type_t maxapp = BI_TOP) {
    return dag_find_paths(dag(), maxapp);
  }

  /**
   * Converts the list that is represented by this feature structure
   * into an STL list of feature structures. If this fs does not
   * represent a list, an empty STL list will be returned.
   */
  std::list<fs> get_list() const;

  /** Return internal type name of the root node */
  const char *name() const;
  /** Return external type name of the root node */
  const char *printname() const;

  /** Return type of the root node */
  int type() const { return dag_type(_dag); }
  /** Set type of the root node to \a s */
  void set_type(type_t s) { dag_set_type(_dag, s); }

  /** Return size of this fs. _fix_me_ not implemented */
  inline int size() const { return 1; /* dag_size(_dag); */ }

  /** Return internal representation of fs */
  inline dag_node *dag() const { return _dag; }
  /** If the underlying dag is a non-permanent dag, return the generation it
   *  belongs to.
   */
  inline int temp() const { return _temp; }

  /** The underlying dag is a non-permanent dag, set the generation it belongs
      to. */
  inline void set_temp(int gen) { _temp = gen; }

  /** Recursively remove all edges with attributes contained in \a del */
  void restrict(list_int *del);

  /** Re-fill the dag. */
  void expand() { _dag = dag_expand(_dag); }

  /** \brief Try to apply the modifications in \a mods to this fs. If this
   *  succeeds, modify the fs destructively, otherwise, the fs will be invalid.
   *
   * \return \c true if the modifications succeed, \c false otherwise.
   */
  bool modify(modlist &mods);

  /** \brief Try to apply as many modifications in \a mods as possible to this
   *  fs. If this succeeds, the fs is destructively modified,
   *
   * \return \c true if all modifications succeed, \c false otherwise.
   */
  bool modify_eagerly(const modlist &mods);

  /** \brief Try to apply the modifications in \a mod to this fs. If this
   *  succeeds, modify the fs destructively, otherwise, leave it as it was.
   *
   * \return \c true if the modification succeed, \c false otherwise.
   */
  bool modify_eagerly(fs &mod);

  /** Special modification function used in `characterization', which stamps
   *  the input positions of relations into the feature structures.
   *  Make sure that \a path exists (if possible), go to the end of that path,
   *  which must contain a f.s. list and try to find the element of the list
   *  where \a attr is not already filled and \a attr : \a value can be unified
   *  into.
   *  \return \c true if the operation succeeded, \c false otherwise
   */
  bool characterize(list_int *path, attr_t attr, type_t value);

  /** Print readably for debugging purposes */
  void print(std::ostream &out, class AbstractDagPrinter &dp) const;


  /** Initialize the static variables for quick check appropriately */
  //static void
  //init_qc(qc_node *unif, int unif_len, qc_node *subs, int subs_len){
  //  _qc_paths_unif = unif; _qc_len_unif = unif_len;
  //  _qc_paths_subs = subs; _qc_len_subs = subs_len;
  //}
  /* initialize the qc structure for unification from the dumper stream \a f.
     if \a subs_too is true, the same structure is used for subsumption quick
     check */
  static void init_qc_unif(class dumper *f, bool subs_too);
  /* initialize the qc structure for subsumption from the dumper stream \a f. */
  static void init_qc_subs(class dumper *f);

  static int get_unif_qc_length() { return _qc_len_unif; }
  static int get_subs_qc_length() { return _qc_len_subs; }

  /** Return the unification quick check vector of a feature structure. */
  inline qc_vec get_unif_qc_vector() const {
    return get_qc_vector(_qc_paths_unif, _qc_len_unif);
  }

  /** Return the unification quick check vector of the n'th arg of a
      feature structure. */
  inline qc_vec get_unif_qc_vector(int nextarg) const {
    return nth_arg(nextarg).get_qc_vector(_qc_paths_unif, _qc_len_unif);
  }

  /** Return the subsumption quick check vector of a feature structure. */
  inline qc_vec get_subs_qc_vector() const {
    return get_qc_vector(_qc_paths_subs, _qc_len_subs);
  }

  /** \brief Check to quick check vectors \a a and \a b for compatibility with
   *  respect to unification. \a qc_len is the length of the vectors.
   */
  inline static bool qc_compatible_unif(const qc_vec &a, const qc_vec &b) {
    for(int i = 0; i < _qc_len_unif; ++i) {
      if(glb(a[i], b[i]) == T_BOTTOM) {
#ifdef PETDEBUG
        LOG(logAppl, DEBUG, "quickcheck fails for path " << i
            << " with `" << print_name(a[i])
            << "' vs. `" << print_name(b[i]) << "'");
#endif
        return false;
      }
    }
    return true;
  }

  /** Check to quick check vectors \a a and \a b for compatibility with respect
   *  to subsumption in both directions. \a qc_len is the length of the
   *  vectors. If \a a subsumes \a b, \a forward is \c true on return,
   *  analogously for \a backward.
   *  \attention \a forward and \a backward must be \c true when calling this
   *  function.
   */
  inline static void
  qc_compatible_subs(const qc_vec &a, const qc_vec &b,
                     bool &forward, bool &backward){
    bool st_a_b, st_b_a;
    for(int i = 0; i < _qc_len_subs; ++i) {
      if(a[i] != b[i]) {
        subtype_bidir(a[i], b[i], st_a_b, st_b_a);
        if(st_a_b == false) { backward = false; if (forward == false) return; }
        if(st_b_a == false) { forward = false; if (backward == false) return; }
      }
    }
  }

 private:

  dag_node *_dag;
  int _temp;

  friend fs unify_restrict(fs &root, const fs &fs1, fs &fs2, list_int *del = 0, bool stat = true);
  friend fs copy(const fs &);
  friend bool compatible(const fs &, const fs &);

  /** Return the quick check vector of a feature structure.
   *  \param qc_paths A path tree representing the previously determined quick
   *         check paths in a compact way.
   *  \param qc_len The maximal number of quick check paths to extract (the
   *         length of the returned array).
   */
  qc_vec get_qc_vector(qc_node *qc_paths, int qc_len) const;

  friend fs unify_np(fs &root, const fs &, fs &);
  friend void subsumes(const fs &a, const fs &b, bool &forward, bool &backward);

  friend fs
    packing_partial_copy(const fs &a, const class restrictor &r, bool perm);


  friend int compare(const fs &, const fs &);
  friend bool operator<(const fs &, const fs &);
  friend bool operator>(const fs &, const fs &);
};

inline bool operator<(const fs &a, const fs &b)
{ return a._dag < b._dag; }

inline bool operator>(const fs &a, const fs &b)
{ return a._dag > b._dag; }

/** Unify two feature structures and return a partially restricted result.
 *  \param root the feature structure to unify into
 *  \param fs1 the substructure under root to unify into
 *  \param fs2 the feature structure to unify into \a a
 *  \param del the list of attributes for restriction of the root node (only).
 *  \param stat if \c true, will count unification statistics.
 *  If the unification failed, the resulting fs will not be valid.
 */
extern fs
unify_restrict(fs &root, const fs & fs1, fs & fs2, list_int *del, bool stat);

/** Unify two feature structures and return the result.
 *  \param root the feature structure to unify into
 *  \param a the substructure under root to unify into
 *  \param b the feature structure to unify into \a a
 *  If the unification failed, the resulting fs will not be valid.
 */
inline fs unify(fs &root, const fs &a, fs &b) {
  return unify_restrict(root, a, b, 0, false);
}

/** Do a unification without partial copy, results in temporary dag. np stands
 *  for "non permanent".
 *  \a root is the dag that contains \a a as subnode.
 *  \a a is unified with \a b and the result is returned.
 *  If the unification failed, the resulting fs will not be valid.
 */
extern fs unify_np(fs &root, const fs &a, fs &b);

/** Do a bidirectional check for subsumption
 * \a forward is  \c true if \a a subsumes \a b, \a backward is \c true if
 * \a b subsumes \a a.
 * \attention this function does not reset forward and backward on entry.
 * If forward or backward are set to false on entry, subsumption in that
 * direction is not checked.
 */
extern void subsumes(const fs &a, const fs &b, bool &forward, bool &backward);

/** Return a restricted (maybe \a permanent) copy of \a a according to
 * restrictor \a del.
 */
fs
packing_partial_copy(const fs &a, const class restrictor &del, bool permanent);

/** clone \a a (make a deep copy). */
extern fs copy(const fs &a);
/** Return \c true if \a a and \a b are unifyable */
extern bool compatible(const fs &a, const fs &b);

/** Transfer unification statistics counters in global variables to a
    statistics object. */
extern void get_unifier_stats();



/** Feature Structure Memory Allocation.
 * Allocation state for feature structure allocation.
 */
class fs_alloc_state
{
 public:
    /** Create a new allocation state for feature structures which
     *  releases all dags allocated since its creation on demand, or at its
     *  destruction.
     * \param auto_release When \c true, call release() at destruction of the
     *        state
     */
    inline fs_alloc_state(bool auto_release = true)
        : _auto(auto_release)
    {
        dag_alloc_mark(_state);
    }

    /** Destructor.
     * Eventually call release(), if the state was created with \c auto_release
     * = \c true
     */
    inline ~fs_alloc_state()
    {
        if(_auto)
        {
            dag_alloc_release(_state);
        }
    }

    /** Release all feature structures allocated since the creation of this
     *  state or the last call to release().
     */
    inline void release()
    {
        dag_alloc_release(_state); _auto = false;
    }

    /** Clear the allocation statistics */
    inline void clear_stats()
    {
        dag_alloc_clear_stats();
    }

    /**
     * Reports how much memory (in bytes) is required for storing all permanent
     * or temporary fs's that have been created so far (as if memory was not
     * reused). The value can be reset by clear_stats().
     */
    long long dynamic_usage()
    {
        return dag_alloc_dynamic_mem();
    }

    /**
     * Reports how much memory (in bytes) is currently allocated for storing
     * temporary fs's. The value can be reset by clear_stats(), which
     * has only a visible effect if allocated memory may shrink.
     */
    long long static_usage()
    {
        return dag_alloc_static_mem();
    }

    /** Release all memory allocated for feature structures. */
    void reset()
    {
        dag_alloc_reset();
    }

    /** Tell the allocator to release memory chunks if it is convenient. */
    void may_shrink()
    {
        dag_alloc_may_shrink();
    }

 private:
    struct dag_alloc_state _state;
    bool _auto;
};

//
// computing of failure paths for the quickcheck
// only supported with tomabechi unifier
//

#include "failure.h"
#include <map>

extern std::map<failure, int> failure_id;
extern std::map<int, failure> id_failure;
extern std::map<int, double> failing_paths_unif;
extern std::map<list_int *, int, list_int_compare> failing_sets_unif;
extern std::map<int, double> failing_paths_subs;
extern std::map<list_int *, int, list_int_compare> failing_sets_subs;

#endif
