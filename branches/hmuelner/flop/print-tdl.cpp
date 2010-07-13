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

/* printing of internal representation in TDL syntax */

#include "pet-config.h"
#include "flop.h"
#include "lex-io.h"
#include "list-int.h"
#include "logging.h"
#include "errors.h"
#include <ctime>

#include <cstdio>
#include <cstring>

int tdl_save_lines = 1;

/* the _print* functions return if what they printed out was complex enough to require a line
   of it's own */

int tdl_print_avm(FILE *f, int level, Avm *A, Coref_table *coref);
int tdl_print_conjunction(FILE *f, int level, Conjunction *C, Coref_table *coref);

int tdl_print_list_body(FILE *f, int level, Tdl_list *L, Coref_table *coref)
{
  int i, complex = 0, last_complex = 0;

  for(i = 0; i < L -> n(); i++)
    {
      if(i != 0)
        {
          fprintf(f, ",");
          if(last_complex)
            {
              fprintf(f, "\n");
              indent(f, level);
            }
          else
            fprintf(f, " ");

          complex = 1;
        }
      
      last_complex = tdl_print_conjunction(f, level, L->list[i], coref);
    }

  if(L->openlist)
    {
      if(i != 0) fprintf(f, ", ");

      fprintf(f, "...");
    }
  else if(L->dottedpair)
    {
      fprintf(f, " .");
      if(last_complex)
        {
          fprintf(f, "\n");
          indent(f, level);
        }
      else
        fprintf(f, " ");

      complex = 1;
      
      tdl_print_conjunction(f, level, L->rest, coref);
    }

  return complex;
}

int tdl_print_list(FILE *f, int level, Tdl_list *L, Coref_table *coref)
{
  int complex = 0, ind = 0;

  if(L->difflist)
    fprintf(f, "<! "), ind = 3;
  else
    fprintf(f, "< "), ind = 2;

  complex = tdl_print_list_body(f, level + ind, L, coref);

  if(complex && !tdl_save_lines)
    {
      fprintf(f, "\n");
      indent(f, level);
    }
  else fprintf(f, " ");
  
  if(L->difflist)
    fprintf(f, "!>");
  else
    fprintf(f, ">");

  return complex;
}

int tdl_print_conjunction(FILE *f, int level, Conjunction *C, Coref_table *coref)
{
  int i, complex = 0;

  if(C == 0 || C->n() == 0)
    {
      fprintf(f, "[ ]");
      return complex;
    }

  for(i = 0; i < C -> n(); i++)
    {
      if(i > 0)
        {
          fprintf(f, " &");
          fprintf(f, "\n");
          indent(f, level);
        }

      switch(C->term[i]->tag)
        {
        case COREF:
          assert(C->term[i]->coidx < coref->n());
          fprintf(f, "#%s", coref->coref[C->term[i]->coidx]);
          break;
        case TYPE:
          {
            fprintf(f, "%s", types[C->term[i]->type]->printname);
            break;
          }
        case ATOM:
          fprintf(f, "'%s", C->term[i]->value);
          break;
        case STRING:
          fprintf(f, "\"%s\"",  C->term[i]->value);
          break;
        case FEAT_TERM:
          complex = tdl_print_avm(f, level, C->term[i]->A, coref) || complex;
          break;
        case LIST:
        case DIFF_LIST:
          complex = tdl_print_list(f, level, C->term[i]->L, coref) || complex;
          break;
        case TEMPL_PAR:
        case TEMPL_CALL:
          throw tError("internal error: unresolved template call/parameter");
          break;
        default:
          LOG(logSyntax, ERROR,
              "unknown term tag `" << C->term[i]->tag << "'"); 
          break;
        }
    }

  return complex;
}

int tdl_print_avm(FILE *f, int level, Avm *A, Coref_table *coref)
{
  int complex = 0;

  if(A != NULL && A -> n() > 0)
    {
      int j, l,  maxl;

      // find widest feature name
      maxl = 0;
      for (j = 0; j < A -> n(); ++j)
        {
          if((l = A->av[j]->attr.size()) > maxl) maxl = l;
        }
      
      fprintf(f, "[ ");
      
      for(j = 0; j < A -> n(); j++)
        {
          if(j != 0)
            {
              indent(f, level + 2); complex = 1;
            }

          fprintf(f, "%s", A->av[j]->attr.c_str());
          l = A->av[j]->attr.size();

          indent(f, maxl - l + 1);

          if(tdl_print_conjunction(f, level + 2 + maxl + 1, A->av[j]->val, coref))
            complex = 1;

          if(j == A -> n() -1)
            {
              if(complex && !tdl_save_lines)
                {
                  fprintf(f, "\n");
                  indent(f, level);
                }
              else
                fprintf(f, " ");
              
              fprintf(f, "]");
            }
          else
            {
              fprintf(f, ",\n");
            }
        }
    }
  else
    {
      fprintf(f, "[ ]");
    }

  return complex;
}

void tdl_print_constraint(FILE *f, Type *t, const char *name)
{
  Conjunction *c;

  fprintf(f, "%s :=", name);
  
  list_int *l = t->parents;
  while(l)
    {
      fprintf(f, " %s &", types[first(l)]->printname.c_str());
      l = rest(l);
    }

  fprintf(f, "\n");

  c = t->constraint;
  
  indent(f, 2);
  tdl_print_conjunction(f, 2, c, t->coref);

  if(t->status != NO_STATUS)
    {
      fprintf(f, ", status: %s", statustable.name(t->status).c_str());
    }
  
  fprintf(f, ".\n");
}

void write_pre_header(FILE *outf, const std::string& outfname, const std::string& fname,
                      const std::string& grammar_version)
{
  time_t t = time(NULL);
  
  fprintf(outf, ";;; `%s' -- generated from `%s' (%s) on %s\n",
          outfname.c_str(), fname.c_str(), grammar_version.c_str(), ctime(&t));
}

void write_pre(FILE *f)
{
  int i;
  Type *t;

  fprintf(f, ":begin :type.\n\n");
  for(i = 0; i < types.number(); i++)
    {
      t = types[i];
      if(!t->tdl_instance && (t->constraint != NULL || t->parents != NULL))
        {
          fprintf(f, ";; type definition from %s:%d\n", t->def.fname, t->def.linenr);
          tdl_print_constraint(f, t, types[i]->printname.c_str());
          fprintf(f, "\n");
        }
    }
  fprintf(f, ":end :type.\n");

  fprintf(f, "\n:begin :instance.\n\n");
  for(i = 0; i < types.number(); i++)
    {
      t = types[i];
      if(t->tdl_instance && (t->constraint != NULL || t->parents != NULL))
        {
          fprintf(f, ";; instance definition from %s:%d\n", t->def.fname, t->def.linenr);
          tdl_print_constraint(f, t, types[i]->printname.c_str());
          fprintf(f, "\n");
        }
    }
  fprintf(f, ":end :instance.\n");
}
