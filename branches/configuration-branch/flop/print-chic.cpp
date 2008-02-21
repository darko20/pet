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

/* output for the CHIC system */

#include "flop.h"

int save_lines = 1;

/* the print_* functions return if what they printed out was complex enough to require a line
   of it's own */

int print_avm(FILE *f, int level, struct avm *A, struct coref_table *coref);
int print_conjunction(FILE *f, int level, struct conjunction *C, struct coref_table *coref);

int print_list_body(FILE *f, int level, struct tdl_list *L, struct coref_table *coref)
{
  int i, complex = 0, last_complex = 0;

  for(i = 0; i < L -> n; i++)
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
      
      if(i == L -> n - 1 && L -> dottedpair)
        {
          fprintf(f, "|( ");
          complex += print_conjunction(f, level + 3, L -> list[i], coref);

          if(complex) 
            {
              fprintf(f, ",\n");
              indent(f, level + 3);
            }
          else fprintf(f, ", ");
      
          complex += print_conjunction(f, level + 3, L -> rest, coref);
          
          if(complex && !save_lines)
            {
              fprintf(f, "\n");
              indent(f, level);
            }
          else 
            fprintf(f, " ");
      
          fprintf(f, ")");

        }
      else
        {
          last_complex = print_conjunction(f, level, L -> list[i], coref);
        }

    }

  if(L -> openlist)
    {
      if(i != 0) fprintf(f, ", ");

      fprintf(f, "...");
    }

  return complex;
}

int print_list(FILE *f, int level, struct tdl_list *L, struct coref_table *coref)
{
  int complex = 0, ind = 0;

  if(L -> difflist)
    fprintf(f, "<! "), ind = 3;
  else if(L -> dottedpair == 0 || L -> n > 1)
    fprintf(f, "< "), ind = 2;

  complex = print_list_body(f, level + ind, L, coref);

  if(complex && !save_lines)
    {
      fprintf(f, "\n");
      indent(f, level);
    }
  else fprintf(f, " ");
  
  if(L -> difflist)
    fprintf(f, "!>");
  else if (L -> dottedpair == 0 || L -> n > 1)
    fprintf(f, ">");

  return complex;
}

static int dl_cor_nr = 42;
static char *dl_cor = NULL;

void munge_in_coref(struct conjunction *C, struct coref_table *co)
{
  int i, ncorefs = 0;
  
  for(i = 0; i < C -> n; i++)
    {
      if(C -> term[i] -> tag == COREF)
        {
          dl_cor = co -> coref[C -> term[i] -> coidx];
          ncorefs ++;
        }
    }

  if(ncorefs == 0)
    {
      struct term *T;
      dl_cor = (char *) salloc(20);
      sprintf(dl_cor, "dl%d", dl_cor_nr++);

      T = new_term();
      T -> tag = COREF; T -> coidx = co -> n;
      add_term(C, T);

      co -> coref[co -> n++] = dl_cor;
    }
}

int print_exp_list_body(FILE *f, int level, struct tdl_list *L, int i, struct coref_table *coref)
{
  int complex = 0;

  fprintf(f, "[ FIRST ");

  complex = print_conjunction(f, level + 8, L -> list[i], coref);

  fprintf(f, ",\n");
  indent(f, level + 2);
  fprintf(f, "REST  "); 

  if(i == L -> n -1)
    {
      if(L -> difflist)
        {
          dl_cor = (char *) salloc(20);
          sprintf(dl_cor, "dl%d", dl_cor_nr++);
          fprintf(f, "#%s: ", dl_cor);
        }
      if(L -> openlist)
        fprintf(f, "top ");
      else if(L -> dottedpair)
        print_conjunction(f, level + 8 + 6, L -> rest, coref);
      else
        fprintf(f, "nil ");
    }
  else
    {
      fprintf(f, "top\n");
      indent(f, level + 2);
      complex = print_exp_list_body(f, level + 8, L, i + 1, coref) || complex;
    }

  fprintf(f, "]");

  /*  if(complex)
    {
      fprintf(f, "\n");
      indent(f, level);
      complex = 0;
    }
  else
    fprintf(f, " ");
    */

  return complex;
}


int print_exp_list(FILE *f, int level, struct tdl_list *L, struct coref_table *coref)
/* prints (diff)list not using sugared syntax */
{
  int complex = 0, ind = 0;

  if(L -> difflist)
    {
      fprintf(f, "[ LIST "), ind = 7, complex = 1;
      if(dl_cor)
        {
          LOG_FATAL(loggerUncategorized,
                    "nested diff_list - enhance dl_cor mechanism...");
          exit(1);
        }
    }

  if(L -> n > 0)
    {
      complex = print_exp_list_body(f, level + ind, L, 0, coref) || complex;
    }
  else
    {
      if(L -> difflist)
        {
          dl_cor = (char *) salloc(20);
          sprintf(dl_cor, "dl%d", dl_cor_nr++);
          fprintf(f, "#%s: ", dl_cor);
        }
      fprintf(f, "nil");
    }
  
 if (L -> difflist == 1)
    {
      fprintf(f, ",\n");
      indent(f, level);
      fprintf(f, "  LAST #%s ]", dl_cor);
      complex = 0;
      dl_cor = NULL;
    }

  /*  if(complex && !save_lines)
    {
      fprintf(f, "\n");
      indent(f, level);
    }
  else fprintf(f, " "); */

  return complex;
}

int print_conjunction(FILE *f, int level, struct conjunction *C, struct coref_table *coref)
{
  int i, ncorefs = 0, ntypes = 0, nrest = 0, complex = 0;
  int not_builtin_list = 0;

  int contains_cons = 0;

  for(i = 0; i < C -> n; i++)
    {
      if(C -> term[i] -> tag == COREF)
        {
          fprintf(f, "#%s", coref->coref[C -> term[i] -> coidx]);
          ncorefs ++;
        }
      if(C -> term[i] -> tag == LIST && C -> term[i] -> L -> difflist == 0 &&
         C -> term[i] -> L -> dottedpair == 1) contains_cons++;
    }

  for(i = 0; i < C -> n; i++)
    {
      if(C -> term[i] -> tag == TYPE)
        {     
          if(contains_cons == 0 || C -> term[i] -> type != BI_CONS)
            {
              if(ncorefs > 0 && ntypes == 0) fprintf(f, ":");
              fprintf(f, "%s%s", ntypes == 0 ? "" : "^",
                      types.name(C -> term[i] -> type).c_str());
              ntypes ++;
            }
          if(C->term[i]->type != BI_CONS &&
             C->term[i]->type != BI_LIST &&
             C->term[i]->type != BI_DIFF_LIST)
            not_builtin_list++;
        }
    }

  for(i = 0; i < C -> n; i++)
    {
      switch(C -> term[i] -> tag)
        {
        case COREF:
        case TYPE:
          break;
        case ATOM:
          nrest ++;

          if(nrest > 1)
            {
              fprintf(f, " &");
              if(complex)
                {
                  fprintf(f, "\n");
                  indent(f, level);
                }
              else
                fprintf(f, " ");
            }

          fprintf(f, "'%s", C -> term[i] -> value);
          break;
        case STRING:
          nrest ++;

          if(nrest > 1)
            {
              fprintf(f, " &");
              if(complex)
                {
                  fprintf(f, "\n");
                  indent(f, level);
                }
              else
                fprintf(f, " ");
            }

          fprintf(f, "%s\"%s\"", (ncorefs > 0) ? ":" : "", C -> term[i] -> value);
          break;
        case FEAT_TERM:
          nrest ++;

          if(nrest > 1)
            {
              fprintf(f, " &\n");
              indent(f, level);
            
              ntypes = ncorefs = 0;
            }
          
          if(ntypes > 0)
            {
              fprintf(f, "\n");
            }
          else
            {
              fprintf(f, "%stop\n", ncorefs > 0 ? ":" : "");
            }

          indent(f, level);
          complex = print_avm(f, level, C -> term[i] -> A, coref) || complex;
          break;
        case LIST:
        case DIFF_LIST:
          nrest ++;

          if(nrest > 1)
            {
              fprintf(f, " &\n");
              indent(f, level);
            }

          if(ntypes > 0)
            {
              fprintf(f, "\n");
              indent(f, level);
            }
          else
            { 
              if(ncorefs > 0) fprintf(f, "=");
              if(not_builtin_list)
                fprintf(f, "top\n");
            }

          if(not_builtin_list)
            complex = print_exp_list(f, level, C -> term[i] -> L, coref) || complex;
          else
            complex = print_list(f, level, C -> term[i] -> L, coref) || complex;
          break;
        case TEMPL_PAR:
        case TEMPL_CALL:
          LOG_FATAL(loggerUncategorized,
                    "internal error: unresolved template call/parameter");
          exit(1);
          break;
        default:
          LOG(loggerUncategorized, Level::INFO,
              "unknown type of term `%d'", C -> term[i] -> tag); 
          break;
        }
    }

  return complex;
}

int print_avm(FILE *f, int level, struct avm *A, struct coref_table *coref)
{
  int complex ;

  complex = 0;
  
  if(A != NULL && A -> n > 0)
    {
      int j, l,  maxl;

      int *featorder;

      featorder = (int *) salloc(A -> n * sizeof(int));
      assert(featorder != NULL);

      // find widest feature name & initialize feature order
      maxl = 0;
      for (j = 0; j < A -> n; ++j)
        {
          featorder[j] = j;
          if((l = strlen(A -> av[j] -> attr)) > maxl) maxl = l;
        }

      if(A -> is_special)
        {
          if(strcasecmp(A -> av[0] -> attr, "REST") == 0 || strcasecmp(A -> av[0] -> attr, "LAST") == 0)
            {
              /* order is not OK */
              featorder[0] = 1; featorder[1] = 0;
            }
        }
      
      fprintf(f, "[ ");
      
      for(j = 0; j < A -> n; j++)
        {
          if(j != 0)
            {
              indent(f, level + 2); complex = 1;
            }

          fprintf(f, "%s", A -> av[featorder[j]] -> attr);
          l = strlen(A -> av[featorder[j]] -> attr);

          indent(f, maxl - l + 1);

          if(print_conjunction(f, level + 2 + maxl + 1, A -> av[featorder[j]] -> val, coref))
            complex = 1;

          if(j == A -> n -1)
            {
              if(complex && !save_lines)
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
      free(featorder);
    }
  else
    {
      fprintf(f, "[ ]");
    }

  return complex;
}

void print_constraint(FILE *f, struct type *t, const string &name)
{
  int j, navm = 0;
  struct conjunction *c = t->constraint;

  fprintf(f, "%s\n", name.c_str());

  for(j = 0; c != NULL && j < c -> n; j++)
    {
      if (c -> term[j] -> tag == FEAT_TERM)
        {
          navm ++;

          if(navm > 1)
            {
              fprintf(f, " & top\n");
            }
          
          print_avm(f, 0, c -> term[j] -> A, t->coref);
        }
    }

  fprintf(f, "\n");
}

