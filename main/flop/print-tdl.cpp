/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* printing of internal representation in TDL syntax */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "flop.h"

int tdl_save_lines = 1;

/* the print_* functions return if what they printed out was complex enough to require a line
   of it's own */

int tdl_print_avm(FILE *f, int level, struct avm *A, struct coref_table *coref);
int tdl_print_conjunction(FILE *f, int level, struct conjunction *C, struct coref_table *coref);

int tdl_print_list_body(FILE *f, int level, struct tdl_list *L, struct coref_table *coref)
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
      
      last_complex = tdl_print_conjunction(f, level, L -> list[i], coref);
    }

  if(L -> openlist)
    {
      if(i != 0) fprintf(f, ", ");

      fprintf(f, "...");
    }
  else if( L -> dottedpair)
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
      
      tdl_print_conjunction(f, level, L -> rest, coref);
    }

  return complex;
}

int tdl_print_list(FILE *f, int level, struct tdl_list *L, struct coref_table *coref)
{
  int complex = 0, ind = 0;

  if(L -> difflist)
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
  
  if(L -> difflist)
    fprintf(f, "!>");
  else
    fprintf(f, ">");

  return complex;
}

int tdl_print_conjunction(FILE *f, int level, struct conjunction *C, struct coref_table *coref)
{
  int i, complex = 0;

  if(C == 0 || C->n == 0)
    {
      fprintf(f, "[ ]");
      return complex;
    }

  for(i = 0; i < C -> n; i++)
    {
      if(i > 0)
        {
          fprintf(f, " &");
          fprintf(f, "\n");
          indent(f, level);
        }

      switch(C -> term[i] -> tag)
	{
	case COREF:
	  assert(C->term[i]->coidx < coref->n);
	  fprintf(f, "#%s", coref -> coref[C -> term[i] -> coidx]);
          break;
	case TYPE:
          {
	    fprintf(f, "%s", types[C -> term[i] -> type]->printname);
            break;
          }
	case ATOM:
	  fprintf(f, "'%s", C -> term[i] -> value);
	  break;
	case STRING:
	  fprintf(f, "\"%s\"",  C -> term[i] -> value);
	  break;
	case FEAT_TERM:
	  complex = tdl_print_avm(f, level, C -> term[i] -> A, coref) || complex;
	  break;
	case LIST:
	case DIFF_LIST:
	  complex = tdl_print_list(f, level, C -> term[i] -> L, coref) || complex;
	  break;
	case TEMPL_PAR:
	case TEMPL_CALL:
	  fprintf(ferr, "internal error: unresolved template call/parameter\n");
	  exit(1);
	  break;
	default:
	  fprintf(ferr, "unknown term tag `%d'\n", C -> term[i] -> tag); 
	  break;
	}
    }

  return complex;
}

int tdl_print_avm(FILE *f, int level, struct avm *A, struct coref_table *coref)
{
  int complex = 0;

  if(A != NULL && A -> n > 0)
    {
      int j, l,  maxl;

      // find widest feature name
      maxl = 0;
      for (j = 0; j < A -> n; ++j)
        {
          if((l = strlen(A -> av[j] -> attr)) > maxl) maxl = l;
        }
      
      fprintf(f, "[ ");
      
      for(j = 0; j < A -> n; j++)
	{
          if(j != 0)
            {
              indent(f, level + 2); complex = 1;
            }

	  fprintf(f, "%s", A -> av[j] -> attr);
          l = strlen(A -> av[j] -> attr);

          indent(f, maxl - l + 1);

	  if(tdl_print_conjunction(f, level + 2 + maxl + 1, A -> av[j] -> val, coref))
            complex = 1;

          if(j == A -> n -1)
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

void tdl_print_constraint(FILE *f, struct type *t, const char *name)
{
  struct conjunction *c;

  fprintf(f, "%s :=", name);
  
  list_int *l = t->parents;
  while(l)
    {
      fprintf(f, " %s &", types[first(l)]->printname);
      l = rest(l);
    }

  fprintf(f, "\n");

  c = t-> constraint;
  
  indent(f, 2);
  tdl_print_conjunction(f, 2, c, t -> coref);

  if(t->status != NO_STATUS)
    {
      fprintf(f, ", status: %s", statustable.name(t->status).c_str());
    }
  
  fprintf(f, ".\n");
}

void write_pre_header(FILE *outf, char *outfname, char *fname, char *grammar_version)
{
  time_t t = time(NULL);
  
  fprintf(outf, ";;; `%s' -- generated from `%s' (%s) on %s\n",
	  outfname, fname, grammar_version, ctime(&t));
}

void write_pre(FILE *f)
{
  int i;
  struct type *t;

  fprintf(f, ":begin :type.\n\n");
  for(i = 0; i < types.number(); i++)
    {
      t = types[i];
      if(!t->tdl_instance && (t->constraint != NULL || t->parents != NULL))
	{
	  fprintf(f, ";; type definition from %s:%d\n", t->def->fname, t->def->linenr);
	  tdl_print_constraint(f, t, types[i]->printname);
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
	  fprintf(f, ";; instance definition from %s:%d\n", t->def->fname, t->def->linenr);
	  tdl_print_constraint(f, t, types[i]->printname);
	  fprintf(f, "\n");
	}
    }
  fprintf(f, ":end :instance.\n");
}
