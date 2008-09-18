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

/* conversion from TDL representation to dags */

#include "options.h"
#include "flop.h"
#include "dag-arced.h"
#include "settings.h"
#include "utility.h"
#include "logging.h"

using std::string;

int ncorefs = 0;
struct dag_node **dagify_corefs;

void dagify_symtabs()
{
  int i;

  nstatus = statustable.number();
  statusnames = (char **) salloc(sizeof(char *) * nstatus);

  for(i = 0; i < nstatus; i++)
    statusnames[i] = (char *) statustable.name(i).c_str();

  first_leaftype = types.number() - nstaticleaftypes;
  nstatictypes = types.number();
  
  typenames = std::vector<std::string>(nstatictypes);
  typestatus = (int *) salloc(sizeof(int) * nstatictypes);
  printnames = std::vector<std::string>(nstatictypes);

  for(i = 0; i < types.number(); i ++)
    {
      typenames[i] = types.name(i);
      typestatus[i] = types[i]->status;
      printnames[i] = types[i]->printname;
      if(printnames[i].empty())
        printnames[i] = typenames[i];
    }

  nattrs = attributes.number();
  attrname = (char **) salloc(sizeof(char *) * nattrs);
  attrnamelen = (int *) salloc(sizeof(int) * nattrs);

  for(i = 0; i < nattrs; i++)
    {
      attrname[i] = (char *) attributes.name(i).c_str();
      attrnamelen[i] = strlen(attrname[i]);
    }
}

struct dag_node *dagify_conjunction(struct conjunction *C, int type);

// Global variable to enable meaningful error messages
int current_toplevel_type = 0;

/** Convert the internal representation of a TDL specification into a dag.
 * \param C the surface oriented representation of the definition
 * \param type the type whose definition is converted
 * \param crefs the number of coreferences in this definition
 */
struct dag_node *dagify_tdl_term(struct conjunction *C, int type, int ncr)
{
  int i;
  struct dag_node *result;
  
  current_toplevel_type = type;

  ncorefs = ncr;
  dagify_corefs = new struct dag_node * [ncorefs];
  for(i = 0; i < ncorefs; i++) dagify_corefs[i] = 0;
  
  result = dagify_conjunction(C, type);

  current_toplevel_type = 0;

  delete[] dagify_corefs;
  return dag_deref(result);
}

struct dag_node *dagify_avm(struct avm *A)
{
  int i;
  struct dag_node *result;

  result = new_dag(BI_TOP);

  for(i = 0; i < A->n; i++)
    {
      struct dag_arc *arc;
      struct dag_node *val;
      int attr;

      attr = attributes.id(A->av[i]->attr);

      
      // With the option 'no-semantics', all structures under a feature
      // which is specified as 'sem-attr' in the 'flop.set' are
      // removed from the hierarchy definitions and the feature itself is
      // ignored in all computation concering features, such as appropriate
      // type computation.
      if(opt_no_sem 
         && attr == attributes.id(flop_settings->req_value("sem-attr")))
        continue;

      if((val = dagify_conjunction(A->av[i]->val, BI_TOP)) == FAIL)
        return FAIL;
 
      if((arc = dag_find_attr(result->arcs, attr)))
        {
          if(dag_unify1(arc->val, val) == FAIL)
            {
              LOG(root, WARN, "type `" << type_name(current_toplevel_type) 
                  << "': unification under `" << attrname[attr] << "' failed");
              return FAIL;
            }
        }
      else
        dag_add_arc(result, new_arc(attr, val));
    }

  return result;
}

struct dag_node *dagify_list_body(struct tdl_list *L, int i, struct dag_node *last)
{
  struct dag_node *result, *tmp;

  if( i >= L->n )
    {
      if(L->dottedpair)
        return dagify_conjunction( L->rest, BI_TOP);
      else
        {
          result = new_dag( (L->openlist || L->difflist) ? BI_TOP : BI_NIL );
          // crucially not nil for diff_lists - leads to expansion failure
          if(last)
            {
              if(dag_unify1(last, result) == FAIL)
                {
                  LOG(root, WARN, "type `" << type_name(current_toplevel_type)
                      << "': unification of `LAST' failed");
                  return FAIL;
                }
            }
          return result;
        }
    }
  
  result = new_dag(BI_LIST);
  if((tmp = dagify_conjunction(L->list[i], BI_TOP)) == FAIL) return FAIL;
  dag_add_arc(result, new_arc(BIA_FIRST, tmp));
  if((tmp = dagify_list_body(L, i+1, last)) == FAIL) return FAIL;
  dag_add_arc(result, new_arc(BIA_REST, tmp));

  return result;
}

struct dag_node *dagify_list(struct tdl_list *L)
{
  struct dag_node *result;

  if(L->difflist)
    {
      struct dag_node *last, *lst;

      result = new_dag(BI_DIFF_LIST);
      
      last = new_dag(BI_TOP);
      dag_add_arc(result, new_arc(BIA_LAST, last));

      if((lst = dagify_list_body(L, 0, last)) == FAIL) return FAIL;
      dag_add_arc(result, new_arc(BIA_LIST, lst));
    }
  else
    {
      result = dagify_list_body(L, 0, NULL);
    }

  return result;
}

struct dag_node *dagify_conjunction(struct conjunction *C, int type)
{
  int i;
  int cref = -1;

  for(i = 0; C && i < C->n; i++)
    if( C->term[i]->tag == TYPE || C->term[i]->tag == ATOM ||
        C->term[i]->tag == STRING )
      {
        int newtype;

        if( C->term[i]->tag == ATOM)
          {
            C->term[i]->type = types.id(string("'") + C->term[i]->value);
          }
        else if( C->term[i]->tag == STRING )
          {
            C->term[i]->type = types.id(string("\"") + C->term[i]->value + string("\""));
          }

        newtype = glb(type, C->term[i]->type);
        if(newtype == -1)
          {
            LOG(root, WARN,
                "type `" << type_name(current_toplevel_type)
                << "': inconsistent term detected: `" << type_name(type)
                << "' & `" << type_name(C->term[i]->type)
                << "' have no glb...");
            return FAIL;
          }
        type = newtype;
      }
    else if(C->term[i]->tag == COREF)
      {
        if(cref != -1 && cref != C->term[i]->coidx)
          {
            LOG(root, WARN,
                "type `" << type_name(current_toplevel_type)
                << "': term specifies two coreference indices: " << cref
                << " & " << C->term[i]->coidx << "..." << std::endl);
            return FAIL;
          }
        else
          cref = C->term[i]->coidx;
      }

  struct dag_node *result;

  result = new_dag(type);

  for(i = 0; C && i < C->n; i++)
    {
      struct dag_node *tmp;

      if(C->term[i]->tag == FEAT_TERM)
        {
          if((tmp = dagify_avm(C->term[i]->A)) == FAIL)
            return FAIL;
          if(dag_unify1(result, tmp) == FAIL)
            {
              LOG(root, WARN,
                  "type `" << type_name(current_toplevel_type) 
                  << "': feature term unification failed");
              return FAIL;
            }
        }
      else if(C->term[i]->tag == LIST || C->term[i]->tag == DIFF_LIST)
        {
          if((tmp = dagify_list(C->term[i]->L)) == FAIL)
            return FAIL;
          if(dag_unify1(result, tmp) == FAIL)
            {
              LOG(root, WARN, "type `" << type_name(current_toplevel_type)
                  << "': list term unification failed" << std::endl);
              return FAIL;
            }
        }
    }

  if(cref != -1)
    {
      assert(cref < ncorefs);
      if(dagify_corefs[cref] != 0)
        {
          if(dag_unify1(dagify_corefs[cref], result) == FAIL)
            {
              LOG(root, WARN, "type `" << type_name(current_toplevel_type)
                  << "': coreference unification failed" << std::endl);
              return FAIL;
            }
        }
      else
        dagify_corefs[cref] = result;
    }

  return result;
}

void dagify_types()
{
  for(int i = 0; i < types.number(); i++)
    {
      types[i]->thedag = dagify_tdl_term(types[i]->constraint, i,
                                         types[i]->coref ?
                                         types[i]->coref->n : 0);
    }
}
