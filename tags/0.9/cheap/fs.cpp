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

/* class representing feature structures, abstracting from underlying dag
   module  */

#include "cheap.h"
#include "fs.h"
#include "types.h"
#include "tsdb++.h"
#include "restrictor.h"
#include "dagprinter.h"
#include "configs.h"
#include "logging.h"

#include <cstring>
#include <iostream>

using namespace std;


/** @name Quick check
 * see Oepen & Carroll 2000a,b
 */
//@{
// global variables for quick check
qc_node *fs::_qc_paths_unif = NULL, *fs::_qc_paths_subs = NULL;
int fs::_qc_len_unif = 0, fs::_qc_len_subs = 0;

/** compute quickcheck paths (unification) */
bool opt_compute_qc_unif;
/** compute quickcheck paths (subsumption) */
bool opt_compute_qc_subs;
//@}

static bool fs_init();
/** print unification failures */
bool opt_print_failure = fs_init();

static bool fs_init() {
  managed_opt("opt_compute_qc",
    "Activate code that collects unification/subsumption failures "
    "for quick check computation, contains filename to write results to",
    (const char *) NULL);

  managed_opt("opt_nqc_unif",
              "use only top n quickcheck paths (unification)", (int) -1);
  managed_opt("opt_nqc_subs",
              "use only top n quickcheck paths (subsumption)", (int) -1);

  opt_compute_qc_unif = false;
  reference_opt("opt_compute_qc_unif",
                "Activate failure registration for unification",
                opt_compute_qc_unif);

  opt_compute_qc_subs = false;
  reference_opt ("opt_compute_qc_subs",
                 "Activate failure registration for subsumption",
                 opt_compute_qc_subs);

  opt_print_failure = false;
  reference_opt
    ("opt_print_failure",
     "Log unification/subsumption failures "
     "(should be replaced by logging or new/different API functionality)",
     opt_print_failure);
  return opt_print_failure;
}


/** The type that indicates pruning in a dag restrictor */
//type_t dag_restrictor::dag_rest_state::DEL_TYPE;

fs::fs(type_t type)
{
    if(! is_type(type))
        throw tError("construction of non-existent dag requested");

    _dag = type_dag(type);

    _temp = 0;
}

fs::fs(char *path, type_t type)
{
    if(! is_type(type))
        throw tError("construction of non-existent dag requested");

    // TODO: as of rev 339, there are no checks whether the resulting dag
    // is a valid type!
    _dag = dag_create_path_value(path, type);

    _temp = 0;
}

fs::fs(const list_int *path, type_t type)
{
    if(! is_type(type))
        throw tError("construction of non-existent dag requested");

    // TODO: as of rev 339, there are no checks whether the resulting dag
    // is a valid type!
    _dag = dag_create_path_value(const_cast<list_int*>(path), type);

    _temp = 0;
}

fs
fs::get_attr_value(int attr) const
{
    return fs(dag_get_attr_value(_dag, attr));
}

fs
fs::get_attr_value(char *attr) const
{
    int a = lookup_attr(attr);
    if(a == -1) return fs();

    struct dag_node *v = dag_get_attr_value(_dag, a);
    return fs(v);
}

fs
fs::get_path_value(const list_int *path) const
{
    return fs(dag_get_path_value(_dag, const_cast<list_int*>(path)));
}

fs
fs::get_path_value(const char *path) const
{
    return fs(dag_get_path_value(_dag, path));
}

std::list<fs>
fs::get_list() const
{
    list<fs> fs_list;
    fs current = *this;
    while (current.valid() && !subtype(current.type(), BI_NIL)) {
        fs first = current.get_attr_value(BIA_FIRST);
        if (first.valid())
            fs_list.push_back(first);
        current = current.get_attr_value(BIA_REST);
    }
    return fs_list;
}

const char *
fs::name() const
{
    return type_name(dag_type(_dag));
}

const char *
fs::printname() const
{
  return print_name(dag_type(_dag));
}

void fs::print(std::ostream &out, AbstractDagPrinter &dp) const
{
  dp.print(out, _dag, temp());
}

void
fs::restrict(list_int *del)
{
    dag_remove_arcs(_dag, del);
}

bool
fs::modify(modlist &mods)
{
    for(modlist::iterator mod = mods.begin(); mod != mods.end(); ++mod)
    {
        dag_node *p = dag_create_path_value((mod->first).c_str(), mod->second);
        _dag = dag_unify(_dag, p, _dag, 0);
        if(_dag == FAIL)
            return false;
    }
    return true;
}

/** \brief Try to apply as many modifications in \a mods as possible to this
 *  fs. If some of them succeed, the fs is destructively modified,
 *
 * \return \c true if all modifications succeed, \c false otherwise.
 */
bool
fs::modify_eagerly(const modlist &mods) {
  bool full_success = true;
  dag_node *curr = _dag;
  dag_node *newdag;
  for(modlist::const_iterator mod = mods.begin(); mod != mods.end(); ++mod) {
    dag_node *p = dag_create_path_value((mod->first).c_str(), mod->second);
    if (p == FAIL) {
      cerr << "; WARNING: failed to create dag for new path-value ("
           << (mod->first).c_str() << " = " << print_name(mod->second) << ")"
           << endl;
    } else {
      newdag = dag_unify(curr, p, curr, 0);
      if(newdag != FAIL) {
        curr = newdag;
      } else {
        cout << "; WARNING: failed to unify new path-value ("
             << (mod->first).c_str() << " = " << print_name(mod->second)
             << ") into fs (type: " << printname() << ")" << endl;
        full_success = false;
      }
    }
  }
  _dag = curr;
  return full_success;
}

/** \brief Try to apply the modifications in \a mod to this fs. If this
 *  succeeds, modify the fs destructively, otherwise, leave it as it was.
 *
 * \return \c true if the modification succeed, \c false otherwise.
 */
bool
fs::modify_eagerly(fs &mod) {
  if (mod._dag == FAIL) return true ; // nothing to apply
  dag_node *newdag = dag_unify(_dag, mod._dag, _dag, 0);
  if(newdag != FAIL) {
    _dag = newdag;
    return true;
  } else {
    return false;
  }
}

/** Special modification function used in `characterization', which stamps
 *  the input positions of relations into the feature structures. This
 *  destructively changes the fs by assigning it the new dag, if successful.
 *
 *  Make sure that \a path exists (if possible), go to the end of that path,
 *  which must contain a f.s. list and try to find the element of the list
 *  where \a attr : \a value can be unified into.
 *  \return \c true if the operation succeeded, \c false otherwise
 */
bool
fs::characterize(list_int *path, attr_t feature, type_t value) {
  bool succeeded = false;
  dag_node *curr = _dag;
  // try to retrieve the characterization path
  dag_node *p = dag_get_path_value(curr, path);
  if( p == FAIL ) {
    // if it does not exist, maybe due to unfilling, try to put it into the
    // current feature structure using unification
    p = dag_create_path_value(path, BI_TOP);
    if (p == FAIL) return false;
    dag_node *newdag = dag_unify(curr, p, curr, 0);
    if (newdag == FAIL) return false;
    curr = newdag;
    p = dag_get_path_value(curr, path);
  }
  // Now p points to the subdag where the list search should begin

  dag_node *charz_dag
    = dag_create_attr_value(feature, dag_full_copy(type_dag(value)));
  do {
    dag_node *first = dag_get_attr_value(p, BIA_FIRST);
    if (first != FAIL) {
      dag_node *charz = dag_get_attr_value(first, feature);
      if (charz == FAIL || dag_type(charz) == BI_TOP) {
        dag_node *newdag = dag_unify(curr, charz_dag, first, 0);
        if (newdag != FAIL) {
          curr = newdag;
          charz_dag
            = dag_create_attr_value(feature, dag_full_copy(type_dag(value)));
          succeeded = true;
        }
      }
    }
    p = dag_get_attr_value(p, BIA_REST);
  } while (p != FAIL) ;
  _dag = curr;
  return succeeded;
}


// statistics

static long int total_cost_fail = 0;
static long int total_cost_succ = 0;

void
get_unifier_stats()
{
    if(stats.unifications_succ != 0)
    {
        stats.unify_cost_succ = total_cost_succ / stats.unifications_succ;

    }
    else
        stats.unify_cost_succ = 0;

    if(stats.unifications_fail != 0)
        stats.unify_cost_fail = total_cost_fail / stats.unifications_fail;
    else
        stats.unify_cost_fail = 0;

    total_cost_succ = 0;
    total_cost_fail = 0;
}

// recording of failures for qc path computation

int next_failure_id = 0;
map<failure, int> failure_id;
map<int, failure> id_failure;

map<int, double> failing_paths_unif;
map<list_int *, int, list_int_compare> failing_sets_unif;

map<int, double> failing_paths_subs;
map<list_int *, int, list_int_compare> failing_sets_subs;

void
print_failures(std::ostream &out, const list<failure *> &fails,
               bool unification, dag_node *a = 0, dag_node *b = 0) {
  out << "failure (" << (unification ? "unif" : "subs") << ") at"
      << std::endl ;
  for(list<failure *>::const_iterator iter = fails.begin();
      iter != fails.end(); ++iter) {
    out << "  " << **iter << std::endl;
  }
}


void
record_failures(list<failure *> fails, bool unification,
                dag_node *a = 0, dag_node *b = 0) {
  failure *f;
  list_int *sf = 0;

  int total = fails.size();
  int *value = new int[total], price = 0;
  int i = 0;
  int id;

  for(list<failure *>::iterator iter = fails.begin();
      iter != fails.end(); ++iter)
    {
      f = *iter;
      value[i] = 0;
      if(f->type() == failure::CLASH)
        {
          bool good = true;
          // let's see if the quickcheck could have filtered this

          dag_node *d1, *d2;

          d1 = dag_get_path_value(a, f->path());
          d2 = dag_get_path_value(b, f->path());

          int s1 = BI_TOP, s2 = BI_TOP;

          if(d1 != FAIL) s1 = dag_type(d1);
          if(d2 != FAIL) s2 = dag_type(d2);

          if(unification)
            {
              if(glb(s1, s2) != -1)
                good = false;
            }
          else
            {
              bool st_1_2, st_2_1;
              subtype_bidir(s1, s2, st_1_2, st_2_1);
              if(st_1_2 == false && st_2_1 == false)
                good = false;
            }

          if(good)
            {
              value[i] = f->cost();
              price += f->cost();

              if(failure_id.find(*f) == failure_id.end())
                {
                  // This is a new failure. Assign an id.
                  id = failure_id[*f] = next_failure_id++;
                  id_failure[id] = *f;
                }
              else
                id = failure_id[*f];

              // Insert id into sorted list of failure ids for this
              // configuration.
              list_int *p = sf, *q = 0;

              while(p && first(p) < id)
                q = p, p = rest(p);

              if((!p) || (first(p) != id))
                {
                  // This is not a duplicate. Insert into list.
                  // Duplicates can occur when failure paths are also
                  // recorded inside constraint unification. This is
                  // now disabled. Duplicates also occur for subsumption.
                  if(q == 0)
                    sf = cons(id, sf);
                  else
                    q -> next = cons(id, p);
                }
              else if(unification)
                {
                  // _fix_me_ i needed to comment this out because it
                  // didn't work with the more general restrictors
                  // but i don't know the exact reason (bk)
                  //throw tError("Duplicate failure path");
                }
            }
        }
      i++;
    }

  // If this is not a new failure set, free it.
  if(sf)
    {
      if(unification)
        {
          if(failing_sets_unif[sf]++ > 0)
            free_list(sf);
        }
      else
        {
          if(failing_sets_subs[sf]++ > 0)
            free_list(sf);
        }

    }

  i = 0;
  for(list<failure *>::iterator iter = fails.begin();
      iter != fails.end(); ++iter)
    {
      f = *iter;
      if(value[i] > 0)
        {
          if(unification)
            failing_paths_unif[failure_id[*f]] +=
              value[i] / double(price);
          else
            failing_paths_subs[failure_id[*f]] +=
              value[i] / double(price);
        }
      i++;
      delete f;
    }

  delete[] value;
}


fs
unify_restrict(fs &root, const fs &a, fs &b, list_int *del, bool stat) {
  struct dag_alloc_state s;

  dag_alloc_mark(s);

  struct dag_node *res = dag_unify(root._dag, a._dag, b._dag, del);

  if(res == FAIL) {
    if(stat) {
      total_cost_fail += unification_cost;
      stats.unifications_fail++;
    }

    if(opt_compute_qc_unif || opt_print_failure) {
      list<failure *> fails = dag_unify_get_failures(a._dag, b._dag, true);
      if (opt_compute_qc_unif)
        record_failures(fails, true, a._dag, b._dag);
      // \todo replace cerr by a stream that is dedicated to the printing of
      // unification failures
      if (opt_print_failure)
        print_failures(cerr, fails, true, a._dag, b._dag);
    }

    dag_alloc_release(s);
  }
  else {
    if(stat) {
      total_cost_succ += unification_cost;
      stats.unifications_succ++;
    }
  }

  return fs(res);
}

fs
copy(const fs &a)
{
    fs res(dag_full_copy(a._dag));
    stats.copies++;
    dag_invalidate_changes();
    return res;
}

/** Do a unification without partial copy, results in temporary dag. np stands
 *  for "non permanent"
 */
fs
unify_np(fs &root, const fs &a, fs &b)
{
    struct dag_node *res;

    res = dag_unify_temp(root._dag, a._dag, b._dag);

    if(res == FAIL)
    {
        // We really don't expect failures, except in unpacking, or in
        // error conditions. No failure recording, thus.
        total_cost_fail += unification_cost;
        stats.unifications_fail++;

        if(opt_print_failure) {
          LOG(logAppl, ERROR, "unification failure: unexpected failure in non"
              " permanent unification");
        }
    }
    else
    {
        total_cost_succ += unification_cost;
        stats.unifications_succ++;
    }

    fs f(res, unify_generation);
    dag_invalidate_changes();

    return f;
}

void
subsumes(const fs &a, const fs &b, bool &forward, bool &backward)
{
    if(opt_compute_qc_subs || opt_print_failure)
    {
        list<failure *> failures =
            dag_subsumes_get_failures(a._dag, b._dag, forward, backward,
                                      true);

        // Filter out failures that have a representative with a shorter
        // (prefix) path. Assumes path with shortest failure path comes
        // before longer ones with shared prefix.

        list<failure *> filtered;
        for(list<failure *>::iterator f = failures.begin();
            f != failures.end(); ++f)
        {
            bool good = true;
            for(list<failure *>::iterator g = filtered.begin();
                g != filtered.end(); ++g)
            {
                if(prefix(**g, **f))
                {
                    good = false;
                    break;
                }
            }
            if(good)
                filtered.push_back(*f);
            else
                delete *f;

        }

        if (opt_compute_qc_subs)
          record_failures(filtered, false, a._dag, b._dag);
        // \todo replace cerr by a stream that is dedicated to the printing of
        // subsumption failures
        if (opt_print_failure)
          print_failures(cerr, filtered, false, a._dag, b._dag);
    }
    else
      dag_subsumes(a._dag, b._dag, forward, backward);

    if(forward || backward)
        stats.subsumptions_succ++;
    else
        stats.subsumptions_fail++;
}

/* \todo why isn't this a method of fs?
 */
fs
packing_partial_copy(const fs &a, const restrictor &del, bool perm) {
  struct dag_node *res = del.dag_partial_copy(a._dag);
  dag_invalidate_changes();
  if(perm) {
    res = dag_full_p_copy(res);

    // \todo generalize this. This is heavily connected with getting a good
    // context-free approximation out of an HPSG grammar. So maybe more general
    // (dynamic) restrictors a la Kiefer&Krieger would be a good idea.
#if 0
    //
    // one contrastive test run on the 700-item PARC (WSJ) dependency bank
    // seems to suggest that this is not worth it: we get a small increase
    // in pro- and retro-active packings, at the cost of fewer equivalence
    // packings, a hand-full reduction in edges, and a two percent increase
    // in parsing time.  may need more research             (7-jun-03; oe)
    //
    if(subtype(res->type, lookup_type("rule")))
      res->type = lookup_type("rule");
    else if(subtype(res->type, lookup_type("lexrule_supermost")))
      res->type = lookup_type("lexrule_supermost");
#endif

    dag_invalidate_changes();
  }
  // return res; // implicit type conversion calling fs(dag_node *,int). Yuck!
  return fs(res);
}

/* \todo why isn't this a method of fs?
 */
bool
compatible(const fs &a, const fs &b) {
  struct dag_alloc_state s;
  dag_alloc_mark(s);

  bool res = dags_compatible(a._dag, b._dag);

  dag_alloc_release(s);

  return res;
}

/* \todo why isn't this a method of fs?
 */
int
compare(const fs &a, const fs &b)
{
    return a._dag - b._dag;
}

qc_vec fs::get_qc_vector(qc_node *qc_paths, int qc_len) const {
  if (qc_len == 0) return NULL;
  qc_vec vector = new type_t [qc_len];
  memset(vector, 0, qc_len * sizeof(type_t));

  if(temp()) // && opt_hyper temporary dags only during hyperactive parsing
    dag_get_qc_vector_temp(qc_paths, _dag, vector);
  else
    dag_get_qc_vector(qc_paths, _dag, vector);

  return vector;
}

/** Initialize the static variables for quick check appropriately */
void
fs::init_qc_unif(dumper *f, bool subs_too) {
  int nqc_unif = get_opt_int("opt_nqc_unif");
  _qc_paths_unif = dag_read_qc_paths(f, nqc_unif, _qc_len_unif);
  if (subs_too) {
    _qc_paths_subs = _qc_paths_unif;
    _qc_len_subs = _qc_len_unif;
    int nqc_subs = get_opt_int("opt_nqc_subs");
    if(nqc_subs > 0 && nqc_subs < _qc_len_subs)
      _qc_len_subs = nqc_subs;
  }
  if(nqc_unif > 0 && nqc_unif < _qc_len_unif)
    _qc_len_unif = nqc_unif;
}

void
fs::init_qc_subs(dumper *f) {
  int nqc_subs = get_opt_int("opt_nqc_subs");
  _qc_paths_subs = dag_read_qc_paths(f, nqc_subs, _qc_len_subs);
  if(nqc_subs > 0 && nqc_subs < _qc_len_subs)
    _qc_len_subs = nqc_subs;
}

