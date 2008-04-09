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

#include "qc.h"
#include "parse.h"
#include "grammar.h"
#include "fs.h"
#include "tsdb++.h"
#include "settings.h"
#include "cheap.h"
#include "failure.h"

#include <queue>
#include <iomanip>

using namespace std;

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
choose_paths(ostream &out,
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
  LOG(loggerParse, Level::DEBUG, 
      "%*s> (%d) (%d): ", d/10, "", d, covered.size());

    if(d == n)
    {
        time_t t = time(NULL);
        
        if(selected.size() < min_sol_cost)
        {
            min_sol = selected;
            min_sol_cost = selected.size();

            LOG(loggerParse, Level::DEBUG,
                "new solution (cost %d)", min_sol_cost);
            
            out << "; found solution with " << min_sol_cost << " paths on "
                << ctime(&t)
                << "; estimated size of remaining search space "
                << searchspace << endl;
        }
        else
          LOG(loggerParse, Level::DEBUG, "too expensive");

        if(t > timeout)
        {
          out << "; timeout (estimated remaining search space "
              << searchspace << ") " << "on " << ctime(&t) << endl;
          return false;
        }

        return true;
    }
    
    if(selected.size() > min_sol_cost)
    {
        LOG(loggerParse, Level::DEBUG, "too expensive");
        return true;
    }
    
    if(covered.size() < (unsigned) n 
       && intersect_empty(fail_sets[d], selected))
    {   
        // Set is not yet covered.

        LOG(loggerParse, Level::DEBUG,
            "not covered, %d candidates", fail_sets[d].size());

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
            
            LOG(loggerParse, Level::DEBUG,
                "%*s- (%d): trying %d (covers %d more sets)",
                d/10, "", d, top.inf, top.prio);

            selected.insert(top.inf);
            for(list<int>::iterator it = path_covers[top.inf].begin(); 
                it != path_covers[top.inf].end(); ++it)
                covered.insert(*it);

            if(!choose_paths(out, fail_sets, path_covers, selected, covered,
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
        LOG(loggerParse, Level::DEBUG, "already covered");

        if(!choose_paths(out, fail_sets, path_covers, selected, covered,
                         d+1, n, timeout))
            return false;
    }

    return true;
}

extern int next_failure_id;

void
compute_qc_sets(ostream &out, const char *tname,
                map<list_int *, int, list_int_compare> &sets,
                double threshold)
{
  out << ";; set based paths (threshold " << setprecision(1) << threshold
      << "%)" << endl << endl
      << "; " << sets.size() << " failing sets" ;

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
    
    out << ", total count " << setprecision(0) << total_count << ":\n;"
        << endl;
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

        out << ";  #" << n << " (" << top.prio << ", " 
            << setprecision(1) << (float) count_so_far / total_count
            << "%) [";
        
        list_int *l = top.inf;
        while(l)
        {
            if(first(l) > maxpathid)
                maxpathid = first(l);
            
            out << first(l); 
            if (rest(l) == NULL) break;
            out << " ";

            l = rest(l);
        }
        
        out << "]" << endl;

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
    out << "; searching for minimal number of paths to cover " << n 
        << " sets " << ctime(&t);

    min_sol_cost = failure_id.size() + 1;
    searchspace = 1;
    bool minimal;
    set<int> selected;
    set<int> covered;
    minimal = choose_paths(out, fail_sets, path_covers, selected, covered,
                           0, n, t + 30*60);
    
    t = time(NULL);
    out << "; " << (minimal ? "minimal" : "maybe-minimal")
        << " solution (" << min_sol_cost << " paths) found on " << ctime(&t)
        << ";  ";

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
        
        out << i << " [" << v << "] ";
        top_paths.push(pq_item<int,int>(v, i));
    }
    
    out << "\n\n:begin :instance.\n\n" << tname << " := *top* &\n[ ";
    
    n = 0;
    while(!top_paths.empty())
    {
        pq_item<int, int> top = top_paths.top();
        top_paths.pop();
        
        failure fail = id_failure[top.inf];
        
        if(n != 0) out << ",\n  " ;
        if(!fail.empty_path())
        {
            out << DUMMY_ATTR ".";
            fail.print_path(out);
        }
        else
            out << DUMMY_ATTR;
        
        out << " \"" << n << "\" #| " << top.prio << " |#";
        
        n++;
    }
    
    out << " ].\n\n:end :instance." << endl << endl;
}

void
compute_qc_traditional(ostream &out, const char *tname,
                       map<int, double> &paths,
                       int max)
{
    std::priority_queue< pq_item<double, int> > top_failures;
    
    for(map<int, double>::iterator iter = paths.begin();
        iter != paths.end(); ++iter)
    {
        top_failures.push(pq_item<double, int>((*iter).second, (*iter).first));
    }
    
    out << "; traditional paths (max " << max << ")" << endl;
    
    out << ":begin :instance.\n\n" << tname << " := *top* &\n[ ";
    
    int n = 0;
    while(!top_failures.empty() && n < max)
    {
        pq_item<double, int> top = top_failures.top(); 
        top_failures.pop();
        
        failure fail = id_failure[top.inf];
        
        if(n != 0) out << ",\n  ";
        if(!fail.empty_path())
        {
            out << DUMMY_ATTR ".";
            fail.print_path(out);
        }
        else
            out << DUMMY_ATTR;
        out << " \"" << n << "\" #| " << setprecision(2) << top.prio << " |#";
      
      n++;
    }
    out << " ].\n\n:end :instance." << endl << endl;
}

void
compute_qc_paths(ostream &out) {
  int packing_type;
  Config::get("opt_packing", packing_type);
  time_t t = time(NULL);
  out << ";;;\n;;; Quickcheck paths for " << Grammar->property("version")
      << ", generated on " << stats.id << " items on " << ctime(&t)
      << ";;; " << CHEAP_VERSION << "\n;;;" << endl << endl ;

  out << ";; " << next_failure_id << " total failing paths:\n;;" << endl;
  for(int i = 0; i < next_failure_id; i++) {
    out << ";; #" << i << " ";
    id_failure[i].print_path(out);
    out << endl;
  }
  out << endl;
  }
  fprintf(f, "\n");

  out << ";;\n;; quickcheck paths (unification)\n;;" << endl << endl;
  compute_qc_traditional(out, 
                         (packing_type ? "qc_unif_trad_pack" : "qc_unif_trad"),
                         failing_paths_unif, 10000);

  compute_qc_sets(out, packing_type ? "qc_unif_set_pack" : "qc_unif_set",
                  failing_sets_unif, 99.0);
  
  if(packing_type) {
    out << ";;\n;; quickcheck paths (subsumption)\n;;" << endl << endl;
    compute_qc_traditional(out, "qc_subs_trad_pack",
                           failing_paths_subs, 10000);
    compute_qc_sets(out, "qc_subs_set_pack", failing_sets_subs, 90.0);
  }
}
