/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* class representing feature structures, abstracting from underlying dag
   module  */

#include <assert.h>
#include <string.h>
#include "cheap.h"
#include "fs.h"
#include "types.h"
#include "tsdb++.h"

// global variables for quick check

int qc_len;
qc_node *qc_paths;

fs::fs(int type)
{
  if(type < 0 || type >= ntypes)
    throw error("construction of non-existent dag requested");

  _dag = typedag[type];

  _temp = 0;
}

fs::fs(char *path, int type)
{
  if(type < 0 || type >= ntypes)
    throw error("construction of non-existent dag requested");

  _dag = 0; // dag_create_path_value(path, type);

  _temp = 0;
}


fs fs::get_attr_value(int attr)
{
  return fs(dag_get_attr_value(_dag, attr));
}

fs fs::get_attr_value(char *attr)
{
  int a = lookup_attr(attr);
  if(a == -1) return fs();

  struct dag_node *v = dag_get_attr_value(_dag, a);
  return fs(v);
}

fs fs::get_path_value(char *path)
{
  return fs(dag_get_path_value(_dag, path));
}

char *fs::name()
{
  return typenames[dag_type(_dag)];
}

void fs::print(FILE *f)
{
  if(temp())
    {
      dag_print_safe(f, _dag, true);
    }
  else
    dag_print_safe(f, _dag, false);
}

void fs::restrict(list_int *del)
{
  dag_remove_arcs(_dag, del);
}

// statistics

static long int total_cost_fail = 0;
static long int total_cost_succ = 0;

void get_unifier_stats()
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
map<unification_failure, int> failure_id;
map<int, unification_failure> id_failure;

map<int, double> failing_paths;
map<list_int *, int, list_int_compare> failing_sets;


void record_failures(dag_node *root, dag_node *a, dag_node *b)
{
  list<unification_failure *> fails = dag_unify_get_failures(a, b, true);
  
  unification_failure *f;
  list_int *sf = 0;

  if(opt_compute_qc)
    {
      int total = fails.size(), good = 0;
      int *value = new int[total], price = 0;
      int i = 0;
      int id;
      
      for(list<unification_failure *>::iterator iter = fails.begin(); iter != fails.end(); ++iter)
        {
	  f = *iter;
          value[i] = 0;
          if(f->type() == unification_failure::CLASH)
            {
              // let's see if the quickcheck could have filtered this
              if(opt_hyper)
                throw error("quickcheck computation doesn't work in hyperactive mode");

              dag_node *d1, *d2;
              
              d1 = dag_get_path_value_l(a, f->path());
              d2 = dag_get_path_value_l(b, f->path());
              
              int s1 = BI_TOP, s2 = BI_TOP;
              
              if(d1 != FAIL) s1 = dag_type(d1);
              if(d2 != FAIL) s2 = dag_type(d2);
              
              if(glb(s1, s2) == -1)
                
                {
                  value[i] = f->cost();
                  price += f->cost();
                  good++;

                  if(failure_id.find(*f) == failure_id.end())
                    {
                      id = failure_id[*f] = next_failure_id++;
                      id_failure[id] = *f;
                    }
                  else
                    id = failure_id[*f];

                  list_int *p = sf, *q = 0;

                  while(p && first(p) < id)
                    q = p, p = rest(p);
                  
                  if(q == 0)
                    sf = cons(id, sf);
                  else
                    q -> next = cons(id, p);
                }
            }
          i++;
        }

      if(sf)
        if(failing_sets[sf]++ > 0)
          free_list(sf);
      
      i = 0;
      for(list<unification_failure *>::iterator iter = fails.begin(); iter != fails.end(); ++iter)
        {
	  f = *iter;
          if(value[i] > 0)
            failing_paths[failure_id[*f]] += value[i] / price;
          
          i++;
          delete f;
        }

      delete[] value;
    }

  if(opt_print_failure)
    {
      fprintf(stderr, "failure at\n");
      for(list<unification_failure *>::iterator iter = fails.begin(); iter != fails.end(); ++iter)
        {
          fprintf(stderr, "  ");
          (*iter)->print(stderr);
          fprintf(stderr, "\n");
       }
    }
}

fs unify_restrict(fs &root, const fs &a, fs &b, list_int *del, bool stat)
{
  struct dag_node *res;
  struct dag_alloc_state s;

  dag_alloc_mark(s);

  res = dag_unify(root._dag, a._dag, b._dag, del);


  if(res == FAIL)
    {
      if(stat)
	{
	  total_cost_fail += unification_cost;
	  stats.unifications_fail++;
	}

      if(opt_compute_qc || opt_print_failure)
	record_failures(root._dag, a._dag, b._dag);

      dag_alloc_release(s);
    }
  else
    {
      if(stat)
	{
	  total_cost_succ += unification_cost;
	  stats.unifications_succ++;
	}
    }

  return fs(res);
}

fs copy(const fs &a)
{
  fs res(dag_full_copy(a._dag));
  stats.copies++;
  dag_invalidate_changes();
  return res;
}

fs unify_np(fs &root, const fs &a, fs &b)
{
  struct dag_node *res;

  res = dag_unify_np(root._dag, a._dag, b._dag);

  if(res == FAIL)
    {
      total_cost_fail += unification_cost;
      stats.unifications_fail++;

      if(opt_compute_qc || opt_print_failure)
	record_failures(root._dag, a._dag, b._dag);
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

void subsumes(const fs &a, const fs &b, bool &forward, bool &backward)
{
  dag_subsumes(a._dag, b._dag, forward, backward);
}

#ifdef PACKING
fs packing_partial_copy(const fs &a)
{
  fs res(dag_partial_copy(a._dag, lookup_attr("CONT")));
  dag_invalidate_changes();
  return res;
}
#endif

bool compatible(const fs &a, const fs &b)
{
  struct dag_alloc_state s;
  dag_alloc_mark(s);

  bool res = dags_compatible(a._dag, b._dag);

  dag_alloc_release(s);
  
  return res;
}

int compare(const fs &a, const fs &b)
{
  return a._dag - b._dag;
}

type_t *get_qc_vector(const fs &f)
{
  type_t *vector;

  vector = new type_t [qc_len];
  for(int i = 0; i < qc_len; i++) vector[i] = 0;

  if(opt_hyper && f.temp())
    { 
      dag_get_qc_vector_np(qc_paths, f._dag, vector);
    }
  else
    dag_get_qc_vector(qc_paths, f._dag, vector);
  
  return vector;
}

bool qc_compatible(type_t *a, type_t *b)
{
  for(int i = 0; i < qc_len; i++)
    {
      if(glb(a[i], b[i]) < 0)
        {
#ifdef DEBUG
          fprintf(stderr, "quickcheck fails for path %d with `%s' vs. `%s'\n",
                  i, typenames[a[i]], typenames[b[i]]);
#endif
          return false;
        }
    }
  
  return true;
}

fs_factory::fs_factory(dumper *f)
{
  undump_symbol_tables(f);
}

// strangely enough this is needed for Borland C++ Builder
typedef constraint_info *constraint_info_p;

void fs_factory::initialize(dumper *f, int qc_inst)
{
  struct dag_node *dag;

  initialize_dags(ntypes);

#ifdef CONSTRAINT_CACHE
  constraint_cache = new constraint_info_p [ntypes];
#endif

  for(int i = 0; i < ntypes; i++)
    {
      if(i == qc_inst)
	{
	  if(verbosity > 4) fprintf(fstatus, "qc structure `%s' ",
				    typenames[qc_inst]);
	  qc_paths = dag_read_qc_paths(f, opt_nqc, qc_len);
	  dag = 0;
	}
      else
	dag = dag_undump(f);

      register_dag(i, dag);
#ifdef CONSTRAINT_CACHE
      constraint_cache[i] = 0;
#endif
    }

}
