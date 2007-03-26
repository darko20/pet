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

#include <iostream>

// global variables for quick check

int qc_len_unif;
qc_node *qc_paths_unif;
int qc_len_subs;
qc_node *qc_paths_subs;

/** The type that indicates pruning in a dag restrictor */
type_t dag_restrictor::dag_rest_state::DEL_TYPE;

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
    
    _dag = 0; // dag_create_path_value(path, type);
    
    _temp = 0;
}


fs
fs::get_attr_value(int attr)
{
    return fs(dag_get_attr_value(_dag, attr));
}

fs
fs::get_attr_value(char *attr)
{
    int a = lookup_attr(attr);
    if(a == -1) return fs();

    struct dag_node *v = dag_get_attr_value(_dag, a);
    return fs(v);
}

fs
fs::get_path_value(const char *path)
{
    return fs(dag_get_path_value(_dag, path));
}

const char *
fs::name()
{
    return type_name(dag_type(_dag));
}

const char *
fs::printname()
{
  return print_name(dag_type(_dag));
}

void
fs::print(FILE *f, int format)
{
    if(temp()) dag_print_safe(f, _dag, true, format);
    else dag_print_safe(f, _dag, false, format);
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
 *  the input positions of relations into the feature structures.
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
  dag_node *p = dag_get_path_value_l(curr, path);
  if( p == FAIL ) {
    // if it does not exist, maybe due to unfilling, try to put it into the
    // current feature structure using unification
    p = dag_create_path_value(path, BI_TOP);
    if (p == FAIL) return false;
    dag_node *newdag = dag_unify(curr, p, curr, 0);
    if (newdag == FAIL) return false;
    curr = newdag;
    p = dag_get_path_value_l(curr, path);
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

#if 0
// _fix_me_ 
// code for treating CARG differently
bool fs::modify_foo(list_int *path, fs &arg) {
  dag_node *newdag = dag_unify(this->_dag, arg._dag, path);
  if (newdag == FAIL) return false;
  _dag = newdag;
  return true;
}
#endif
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

#ifdef QC_PATH_COMP

// recording of failures for qc path computation

int next_failure_id = 0;
map<unification_failure, int> failure_id;
map<int, unification_failure> id_failure;

map<int, double> failing_paths_unif;
map<list_int *, int, list_int_compare> failing_sets_unif;

map<int, double> failing_paths_subs;
map<list_int *, int, list_int_compare> failing_sets_subs;

void
record_failures(list<unification_failure *> fails, bool unification,
                dag_node *a = 0, dag_node *b = 0)
{
    unification_failure *f;
    list_int *sf = 0;
    
    if(opt_compute_qc)
    {
        int total = fails.size();
        int *value = new int[total], price = 0;
        int i = 0;
        int id;
        
        for(list<unification_failure *>::iterator iter = fails.begin();
            iter != fails.end(); ++iter)
        {
            f = *iter;
            value[i] = 0;
            if(f->type() == unification_failure::CLASH)
            {
                bool good = true;
                // let's see if the quickcheck could have filtered this
                
                dag_node *d1, *d2;
                
                d1 = dag_get_path_value_l(a, f->path());
                d2 = dag_get_path_value_l(b, f->path());
                
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
        for(list<unification_failure *>::iterator iter = fails.begin();
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
	    // _fix_me_ may not delete f if opt_print_failure is on
        }
        
        delete[] value;
    }
    
    if(opt_print_failure)
    {
        fprintf(ferr, "failure (%s) at\n", unification ? "unif" : "subs");
        for(list<unification_failure *>::iterator iter = fails.begin();
            iter != fails.end(); ++iter)
        {
            fprintf(ferr, "  ");
            (*iter)->print(ferr);
            fprintf(ferr, "\n");
        }
	// _fix_me_ need to delete f here
    }
}

#endif

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
        
#ifdef QC_PATH_COMP
    if(opt_compute_qc_unif || opt_print_failure) {
      list<unification_failure *> fails =
        dag_unify_get_failures(a._dag, b._dag, true);
            
      if (opt_compute_qc_unif) 
        record_failures(fails, true, a._dag, b._dag);
    }
#endif
        
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
        
#ifdef QC_PATH_COMP
        if(opt_print_failure)
        {   
            fprintf(ferr, "unification failure: unexpected failure in non"
                    " permanent unification\n");
        }
#endif
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
#ifdef QC_PATH_COMP
    if(opt_compute_qc_subs || opt_print_failure)
    {
        list<unification_failure *> failures =
            dag_subsumes_get_failures(a._dag, b._dag, forward, backward,
                                      true); 

        // Filter out failures that have a representative with a shorter
        // (prefix) path. Assumes path with shortest failure path comes
        // before longer ones with shared prefix.

        list<unification_failure *> filtered;
        for(list<unification_failure *>::iterator f = failures.begin();
            f != failures.end(); ++f)
        {
            bool good = true;
            for(list<unification_failure *>::iterator g = filtered.begin(); 
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
    }
    else
#endif
    dag_subsumes(a._dag, b._dag, forward, backward);

    if(forward || backward)
        stats.subsumptions_succ++;
    else
        stats.subsumptions_fail++;
}

fs
packing_partial_copy(const fs &a, const restrictor &del, bool perm)
{
    struct dag_node *res = del.dag_partial_copy(a._dag);
    dag_invalidate_changes();
    if(perm)
    {
        res = dag_full_p_copy(res);
        
        // _fix_me_ generalize this
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
        return res;
    }
    return res;
}

bool
compatible(const fs &a, const fs &b)
{
    struct dag_alloc_state s;
    dag_alloc_mark(s);
    
    bool res = dags_compatible(a._dag, b._dag);
    
    dag_alloc_release(s);
    
    return res;
}

int
compare(const fs &a, const fs &b)
{
    return a._dag - b._dag;
}

type_t *
get_qc_vector(qc_node *qc_paths, int qc_len, const fs &f)
{
    type_t *vector;
    
    vector = new type_t [qc_len];
    for(int i = 0; i < qc_len; i++) vector[i] = 0;
    
    if(opt_hyper && f.temp())
    { 
        dag_get_qc_vector_temp(qc_paths, f._dag, vector);
    }
    else
        dag_get_qc_vector(qc_paths, f._dag, vector);
    
    return vector;
}

bool
qc_compatible_unif(int qc_len, type_t *a, type_t *b)
{
    for(int i = 0; i < qc_len; i++)
    {
        if(glb(a[i], b[i]) < 0)
        {
#ifdef DEBUG
            fprintf(ferr, "quickcheck fails for path %d with `%s' vs. `%s'\n",
                    i, print_name(a[i]), print_name(b[i]));
#endif
            return false;
        }
    }
    
    return true;
}

void
qc_compatible_subs(int qc_len, type_t *a, type_t *b,
                   bool &forward, bool &backward)
{
    bool st_a_b, st_b_a;
    for(int i = 0; i < qc_len; i++)
    {
        if(a[i] == b[i])
            continue;
        subtype_bidir(a[i], b[i], st_a_b, st_b_a);
        if(st_a_b == false) backward = false;
        if(st_b_a == false) forward = false;
        if(forward == false && backward == false)
            break;
    }
}
