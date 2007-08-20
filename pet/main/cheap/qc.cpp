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

/* computing quick check paths */

#include "pet-config.h"
#ifdef QC_PATH_COMP

#include "qc.h"
#include "parse.h"
#include "grammar.h"
#include "fs.h"
#include "tsdb++.h"
#include "settings.h"
#include "cheap.h"
#include "failure.h"

#include <queue>

using std::set;
using std::vector;
using std::list;
using std::map;
using std::priority_queue;

// _fix_me_ this should not be hardwired (need to make sure it exists)
#define DUMMY_ATTR "ARGS"

inline bool
intersect_empty(set<int> &a, set<int> &b)
{
    if(a.size() > b.size()) return intersect_empty(b, a);
    
    for(set<int>::iterator iter = a.begin(); iter != a.end(); ++iter)
    {
        if(b.find(*iter) != b.end())
            return false;
    }
    
    return true;
}

template <class TPrio, class TInf>
class pq_item
{
public:
    TPrio prio;
    TInf inf;
    
    pq_item() : prio(), inf() {};
    pq_item(const TPrio &p, const TInf &i) : prio(p), inf(i) {};
};

template <class TPrio, class TInf> inline bool
operator<(const pq_item<TPrio, TInf> &a, const pq_item<TPrio, TInf> &b)
{
    return a.prio < b.prio;
}

set<int>::size_type min_sol_cost;
set<int> min_sol;
long long searchspace;

bool
choose_paths(FILE *f,
             vector<set<int> > &fail_sets,
             vector<list<int> > &path_covers,
             set<int> &selected,
             set<int> &covered,
             int d, int n, time_t timeout)
/* Find a minimal set of paths covering all failure sets.
   The algorithm is recursive branch-and-bound.
   selected contains the paths selected so far, d the current
   depth (number of sets), n the total number of sets
 */
{
    if(verbosity > 1)
    {
        fprintf(ferr, "%*s> (%d) (%d): ", d/10, "", d, covered.size());
    }

    if(d == n)
    {
        time_t t = time(NULL);
        
        if(selected.size() < min_sol_cost)
        {
            min_sol = selected;
            min_sol_cost = selected.size();

            if(verbosity > 1)
                fprintf(ferr, "new solution (cost %d)\n",
                        min_sol_cost);
            
            fprintf(f, "; found solution with %d paths on %s",
                    min_sol_cost, ctime(&t));
            fprintf(f, "; estimated size of remaining search space %lld\n",
                    searchspace);
            fflush(f);
        }
        else
        {
            if(verbosity > 1)
                fprintf(ferr, "too expensive\n");
        }

        if(t > timeout)
        {
            fprintf(f, "; timeout (estimated remaining search space %lld) "
                    "on %s", searchspace, ctime(&t));
            return false;
        }

        return true;
    }
    
    if(selected.size() > min_sol_cost)
    {
        if(verbosity > 1)
            fprintf(ferr, "too expensive\n");
        return true;
    }
    
    if(covered.size() < (unsigned) n 
       && intersect_empty(fail_sets[d], selected))
    {   
        // Set is not yet covered.

        if(verbosity > 1)
            fprintf(ferr, "not covered, %d candidates\n",
                    fail_sets[d].size());

        // Pursue a greedy strategy: Choose the path that covers the largest
        // number of remaining sets which are not yet covered.

        std::priority_queue<pq_item<int, int> > candidates;

        for(set<int>::iterator iter = fail_sets[d].begin();
            iter != fail_sets[d].end(); ++iter)
        {
            int value = 0;
            for(list<int>::iterator it = path_covers[*iter].begin();
                it != path_covers[*iter].end(); ++it)
            {
                if(covered.find(*it) == covered.end())
                    value++;
            }
            candidates.push(pq_item<int, int>(value,*iter));
        }

        while(!candidates.empty())
        {
            pq_item<int, int> top = candidates.top();
            searchspace *= candidates.size();
            
            if(verbosity > 1)
                fprintf(ferr, "%*s- (%d): trying %d (covers %d more sets)\n",
                        d/10, "", d, top.inf, top.prio);

            selected.insert(top.inf);
            for(list<int>::iterator it = path_covers[top.inf].begin(); 
                it != path_covers[top.inf].end(); ++it)
                covered.insert(*it);

            if(!choose_paths(f, fail_sets, path_covers, selected, covered,
                             d+1, n, timeout))
                return false;

            selected.erase(top.inf);
            for(list<int>::iterator it = path_covers[top.inf].begin(); 
                it != path_covers[top.inf].end(); ++it)
                covered.erase(*it);

            searchspace /= candidates.size();
            candidates.pop();
        }

    }
    else
    {
        if(verbosity > 1)
            fprintf(ferr, "already covered\n");

        if(!choose_paths(f, fail_sets, path_covers, selected, covered,
                         d+1, n, timeout))
            return false;
    }

    return true;
}

extern int next_failure_id;

void
compute_qc_sets(FILE *f, const char *tname,
                map<list_int *, int, list_int_compare> &sets,
                double threshold)
{
    fprintf(f, ";; set based paths (threshold %.1f%%)\n\n",
            threshold);

    fprintf(f, "; %d failing sets", sets.size());

    //
    // Get failure sets sorted by frequency.
    //
    
    priority_queue<pq_item<int, list_int *> > sets_by_frequency;
    
    double total_count = 0;
    for(map<list_int *, int, list_int_compare>::iterator iter =
            sets.begin(); iter != sets.end(); ++iter)
    {
        total_count += iter->second;
        sets_by_frequency.push(pq_item<int, list_int *>(iter->second,
                                                        iter->first));
    }
    
    fprintf(f, ", total count %.0f:\n;\n",
            total_count);
    total_count /= 100.0;

    //
    // Select sets below threshold, print selected sets, and sort by size.
    //

    int n = 0;
    double count_so_far = 0;
    std::priority_queue<pq_item<int, list_int *> > sets_by_size;
    int maxpathid = 0;

    while(!sets_by_frequency.empty())
    {
        pq_item<int, list_int *> top = sets_by_frequency.top();
        sets_by_frequency.pop();
        
        count_so_far += top.prio;

        if(count_so_far / total_count >= threshold)
            break;

        fprintf(f, ";  #%d (%d, %.1f%%) [", n, top.prio,
                count_so_far / total_count);
        
        list_int *l = top.inf;
        while(l)
        {
            if(first(l) > maxpathid)
                maxpathid = first(l);
            
            fprintf(f, "%d%s", first(l), rest(l) == 0 ? "" : " ");
     
            l = rest(l);
        }
        
        fprintf(f, "]\n");

        top.prio = -length(top.inf);
        sets_by_size.push(top);
        
        n++;
    }
    
    //
    // Construct fail_sets and fail_weight arrays sorted by size of set.
    //
    vector<set<int> > fail_sets(sets.size());
    vector<int> fail_weight(sets.size());
    vector<list<int> > path_covers(maxpathid+1);

    n = 0;
    while(!sets_by_size.empty())
    {
        pq_item<int, list_int *> top = sets_by_size.top();
        sets_by_size.pop();
        
        list_int *l = top.inf;
        fail_weight[n] = sets[l];
        
        while(l)
        {
            fail_sets[n].insert(first(l));
            path_covers[first(l)].push_back(n);
            l = rest(l);
        }
        n++;
    }
    
    // Find minimal set of paths covering all failure sets
    time_t t = time(NULL);
    fprintf(f, "; searching for minimal number of paths to cover %d sets %s",
            n, ctime(&t));

    min_sol_cost = failure_id.size() + 1;
    searchspace = 1;
    bool minimal;
    set<int> selected;
    set<int> covered;
    minimal = choose_paths(f, fail_sets, path_covers, selected, covered,
                           0, n, t + 30*60);
    
    t = time(NULL);
    fprintf(f, "; %s solution (%d paths) found on %s;  ",
            minimal ? "minimal" : "maybe-minimal", min_sol_cost, ctime(&t));

    // compute optimal order for selected paths
    
    priority_queue< pq_item<int, int> > top_paths;

    for(set<int>::iterator iter = min_sol.begin();
        iter != min_sol.end(); ++iter)
    {
        // compute sum of value of all sets that contain `*iter'
        int v = 0, i = *iter;
        
        for(map<int,int>::size_type j = 0; j < sets.size(); j++)
        {
            if(fail_sets[j].find(i) != fail_sets[j].end())
                v += fail_weight[j];
        }
        
        fprintf(f, "%d [%d] ", i, v);
        top_paths.push(pq_item<int,int>(v, i));
    }
    
    fprintf(f, "\n\n:begin :instance.\n\n%s := *top* &\n[ ",
            tname);
    
    n = 0;
    while(!top_paths.empty())
    {
        pq_item<int, int> top = top_paths.top();
        top_paths.pop();
        
        unification_failure fail = id_failure[top.inf];
        
        if(n != 0) fprintf(f, ",\n  ");
        if(!fail.empty_path())
        {
            fprintf(f, DUMMY_ATTR ".");
            fail.print_path(f);
        }
        else
            fprintf(f, DUMMY_ATTR);
        
        fprintf(f, " \"%d\" #| %d |#", n, top.prio);
        
        n++;
    }
    
    fprintf(f, " ].\n\n:end :instance.\n\n");
}

void
compute_qc_traditional(FILE *f, const char *tname,
                       map<int, double> &paths,
                       int max)
{
    std::priority_queue< pq_item<double, int> > top_failures;
    
    for(map<int, double>::iterator iter = paths.begin();
        iter != paths.end(); ++iter)
    {
        top_failures.push(pq_item<double, int>((*iter).second, (*iter).first));
    }
    
    fprintf(f, "; traditional paths (max %d)\n\n",
            max);
    
    fprintf(f, ":begin :instance.\n\n%s := *top* &\n[ ", tname);
    
    int n = 0;
    while(!top_failures.empty() && n < max)
    {
        pq_item<double, int> top = top_failures.top(); 
        top_failures.pop();
        
        unification_failure fail = id_failure[top.inf];
        
        if(n != 0) fprintf(f, ",\n  ");
        if(!fail.empty_path())
        {
            fprintf(f, DUMMY_ATTR ".");
            fail.print_path(f);
        }
        else
            fprintf(f, DUMMY_ATTR);
      fprintf(f, " \"%d\" #| %.2f |#", n, top.prio);
      
      n++;
    }
    fprintf(f, " ].\n\n:end :instance.\n\n");
}

void
compute_qc_paths(FILE *f)
{
    time_t t = time(NULL);
    fprintf(f, ";;;\n;;; Quickcheck paths for %s, generated"
            " on %d items on %s;;; %s\n;;;\n\n",
            Grammar->property("version").c_str(), stats.id, ctime(&t),
            CHEAP_VERSION);

    fprintf(f, ";; %d total failing paths:\n;;\n", next_failure_id);
    for(int i = 0; i < next_failure_id; i++)
    {
        fprintf(f, ";; #%d ", i);
        id_failure[i].print_path(f);
        fprintf(f, "\n");
    }
    fprintf(f, "\n");

    fprintf(f, ";;\n;; quickcheck paths (unification)\n;;\n\n");
    compute_qc_traditional(f, opt_packing ? "qc_unif_trad_pack"
                                          : "qc_unif_trad",
                           failing_paths_unif, 10000);
    fflush(f);
    compute_qc_sets(f, opt_packing ? "qc_unif_set_pack" : "qc_unif_set",
                    failing_sets_unif, 99.0);

    if(opt_packing)
    {
        fprintf(f, ";;\n;; quickcheck paths (subsumption)\n;;\n\n");
        compute_qc_traditional(f, "qc_subs_trad_pack", failing_paths_subs,
                               10000);
        fflush(f);
        compute_qc_sets(f, "qc_subs_set_pack", failing_sets_subs, 90.0);
    }
}


#endif
