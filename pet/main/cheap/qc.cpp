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

#include "pet-system.h"
#include "qc.h"
#include "parse.h"
#include "grammar.h"
#include "fs.h"
#include "tsdb++.h"

#define DUMMY_ATTR "ARGS"
// XXX fix this

vector< set<int> > fail_sets;
vector<int> fail_weight;

set<int>::size_type min_sol_cost;
set<int> min_sol;

bool intersect_empty(set<int> &a, set<int> &b)
{
  if(a.size() > b.size()) return intersect_empty(b, a);

  for(set<int>::iterator iter = a.begin(); iter != a.end(); ++iter)
    {
      if(b.find(*iter) != b.end())
	return false;
    }

  return true;
}

void choose_paths(FILE *f, set<int> &selected, int d, int n)
/* find a minimal set of paths covering all failure sets
   algorithm is recursive branch-and-bound
   selected contains the paths selected so far, d the current
   depth (number of sets), n the total number of sets
 */
{
  if(d == n)
    {
      if(selected.size() < min_sol_cost)
        {
          min_sol = selected;
          min_sol_cost = selected.size();

          time_t t = time(NULL);
          fprintf(f, ";; found solution with %d paths on %s",
                  min_sol_cost, ctime(&t));
        }
      return;
    }

  if(selected.size() > min_sol_cost)
    return;

  if(intersect_empty(fail_sets[d], selected))
    { // set is not yet covered
      for(set<int>::iterator iter = fail_sets[d].begin(); iter != fail_sets[d].end(); ++iter)
	{
          selected.insert(*iter);
          choose_paths(f, selected, d+1, n);
          selected.erase(*iter);
        }
    }
  else
    choose_paths(f, selected, d+1, n);
}

extern int next_failure_id;

template <class TPrio, class TInf> class pq_item
{
public:
  TPrio prio;
  TInf inf;

  pq_item() : prio(), inf() {};
  pq_item(const TPrio &p, const TInf &i) : prio(p), inf(i) {};
};

template <class TPrio, class TInf> inline bool operator<(const pq_item<TPrio, TInf> &a, const pq_item<TPrio, TInf> &b)
{ return a.prio < b.prio; }

void compute_qc_sets(FILE *f)
{
  time_t t = time(NULL);
  fprintf(f, ";; qc paths [set based] -- generated for %s for %d items on %s\n",
	  Grammar->info().version, stats.id, ctime(&t));

  {
    fprintf(f, ";; %d failing paths:\n", next_failure_id);

    for(int i = 0; i < next_failure_id; i++)
      {
        fprintf(f, ";; #%d ", i);
        id_failure[i].print_path(f);
        fprintf(f, "\n");
      }
    fprintf(f, "\n");
  }

  fprintf(f, ";; %d failing sets:\n", failing_sets.size());

  priority_queue<pq_item<int, list_int *> > top_failures;

  for(map<list_int *, int, list_int_compare>::iterator iter = failing_sets.begin(); iter != failing_sets.end(); ++iter)
    {
      top_failures.push(pq_item<int, list_int *>((*iter).second, (*iter).first));
    }

  int n = 0;
  fail_sets.resize(failing_sets.size());
  fail_weight.resize(failing_sets.size());

  while(!top_failures.empty())
    {
      pq_item<int, list_int *> top = top_failures.top();
      top_failures.pop();

      fprintf(f, ";;  #%d (%d) [", n, top.prio);

      list_int *l = top.inf;
      fail_weight[n] = top.prio;

      while(l)
        {
          fail_sets[n].insert(first(l));
          fprintf(f, "%d%s", first(l), rest(l) == 0 ? "" : " ");
          l = rest(l);
        }
      fprintf(f, "]\n");

      n++;
    }

  // find minimal set of paths covering all (up to 1000) failure sets
  {
    min_sol_cost = failure_id.size() + 1;
    set<int> selected;
    choose_paths(f, selected, 0, n > 1000 ? 1000 : n);
  }

  t = time(NULL);
  fprintf(f, ";; minimal set (%d paths) found %s;;  ",
	  min_sol_cost, ctime(&t));

  // compute optimal order for selected paths

  priority_queue< pq_item<int, int> > top_paths;

  for(set<int>::iterator iter = min_sol.begin(); iter != min_sol.end(); ++iter)
    {
      // compute sum of value of all sets that contain `*iter'
      int v = 0, i = *iter;

      for(map<int,int>::size_type j = 0; j < failing_sets.size(); j++)
        {
          if(fail_sets[j].find(i) != fail_sets[j].end())
            v += fail_weight[j];
        }

      fprintf(f, "%d [%d] ", i, v);
      top_paths.push(pq_item<int,int>(v, i));
    }

  fprintf(f, "\n\n:begin :instance.\n\nqc_paths := *top* &\n[ ");

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

void compute_qc_paths(FILE *f, int max)
{
  compute_qc_sets(f);

  priority_queue< pq_item<double, int> > top_failures;

  for(map<int, double>::iterator iter = failing_paths.begin(); iter != failing_paths.end(); ++iter)
    {
      top_failures.push(pq_item<double, int>((*iter).second, (*iter).first));
    }

  time_t t = time(NULL);
  
  fprintf(f, ";; qc paths [traditional] -- generated for %s for %d items on %s\n",
	  Grammar->info().version, stats.id, ctime(&t));

  fprintf(f, ":begin :instance.\n\nqc_paths := *top* &\n[ ");

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
