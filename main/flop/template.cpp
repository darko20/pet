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

/* expansion of TDL templates */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "options.h"
#include "flop.h"

struct coref_table *crefs; // topmost cref table
char *context_descr = 0; // context for warning/error messages

//
// current nesting depth of template calls
//

int templ_nest = 0;

//
// `display' (stack of parameter lists) for actual template parameters:
//  whenever a template call is made, first the default parameter list,
//  and then the actual parameter list are pushed onto the stack */
//

struct param_list *templ_env[TABLE_SIZE];

struct param *find_param(char *name, struct param_list *env)
{
  assert(env != 0);

  for(int j = 0; j < env -> n; j++)
    {
      assert(env -> param[j] != 0);
      
      if(strcasecmp(env -> param[j] -> name, name) == 0)
	return env -> param[j];
    }
  return 0;
}

struct param *lookup_param(char *name)
{
  struct param *p;
  int i;

  p = 0;
  i = templ_nest * 2 - 1;

  while(i >= 0 && (p == 0 || p -> value == 0))
    {
      p = find_param(name, templ_env[i]);
      i--;
    }

  return p;
}

//
// templ_stack
// stack of currently expanded templates

struct templ* templ_stack[TABLE_SIZE];

//
// templ_crefs
// used to make coreference tags in template expansion unique, by giving each
// node in the expansion tree a unique id
//

int templ_crefs[TABLE_SIZE]; // to make coreferences in template calls unique

void clear_templ_crefs()
{
  for(int i = 0; i < TABLE_SIZE; i++)
    templ_crefs[i] = 0;
}

// `expand' (make unique) coreference tag by prepending contents of templ_crefs
string expand_coref_tag(string tag)
{
  string exp(tag);

  for(int i = 0; i < templ_nest; i ++)
    exp += string("_%d", templ_crefs[i]);

  return exp;
}

//
// expansion of template calls and parameters
//

void expand_conjunction(struct conjunction *);

void expand_avm(struct avm *A)
{
  int j;

  for (j = 0; j < A -> n; ++j)
    {
      if(A -> av[j] -> attr[0] == '$')
	{
	  struct param *p;
	  p = lookup_param(A -> av[j] -> attr + 1);

	  if(p == 0 || p -> value == 0)
	    {
	      fprintf(ferr,
		      "error (in `%s'): template parameter `$%s' without "
		      "value.\n", context_descr, A->av[j]->attr);
	      exit(1);
	    }
	  else
	    {
	      if(p->value->n != 1 || p->value->term[0]->tag != TYPE)
		{
		  fprintf(ferr,
			  "error (in `%s'): template parameter `$%s' with "
			  "illegal value.\n", context_descr, A->av[j]->attr);
		  exit(1);
		}
	      else
		{
		  A->av[j]->attr = strdup(p->value->term[0]->value);
		  strtoupper(A->av[j]->attr);
		  if(attributes.id(A->av[j]->attr) == -1)
		    attributes.add(A->av[j]->attr);
		}
	    }
	}

      expand_conjunction(A -> av[j] -> val);
    }
}

void expand_list(struct tdl_list *L)
{
  int i;

  for(i = 0; i < L -> n; i++)
    {
      expand_conjunction(L -> list[i]);
    }
  
  if(L -> dottedpair)
    expand_conjunction(L -> rest);

}

void expand_term (struct term *T)
{
  switch(T -> tag)
    {
    case TYPE:
      break;
    case ATOM:
    case STRING:
      break;
    case COREF:
      if(templ_nest > 0)
	{
	  // we need to make up a new name, and enter it into the main coref table
	  assert(T->coidx < templ_stack[templ_nest-1]->coref->n);
	  string uniq = expand_coref_tag(templ_stack[templ_nest-1]->coref->coref[T->coidx]);
	  
	  if(verbosity > 10)
	    {
	      fprintf(fstatus, "expanding `%s': new name `%s' for tag(%d/%d) `%s'\n",
		      context_descr, uniq.c_str(), T->coidx + 1, templ_stack[templ_nest-1]->coref->n,
		      templ_stack[templ_nest-1]->coref->coref[T->coidx]);
	    }

	  int id = add_coref(crefs, strdup(uniq.c_str()));
	  T -> coidx = id;
	}
      break;
    case FEAT_TERM:
      expand_avm(T -> A);
      break;
    case LIST:
    case DIFF_LIST:
      expand_list(T -> L);
      break;
    case TEMPL_PAR:
    case TEMPL_CALL:
      assert(!"this cannot happen");
      break;
    default:
      fprintf(ferr, "unknown kind of term: %d\n", T -> tag);
      break;
    }
}

struct param *expand_param(struct param *P)
{
  struct param *C;
  C = (struct param *) salloc(sizeof(struct param));
  C->name = P->name; // we share this
  C->value = copy_conjunction(P->value);
  expand_conjunction(C->value);
  return C;
}

struct param_list *expand_params(struct param_list *P)
{
  struct param_list *C;
  C =  (struct param_list *) salloc(sizeof(struct param_list));
  C -> n = P -> n;
  C -> param = (struct param **) salloc(TABLE_SIZE * sizeof(struct param*));

  for(int i = 0; i < C -> n; i++)
    C->param[i] = expand_param(P->param[i]);

  return C;
}

void check_params(struct param_list *actual, struct param_list *def)
/* ensure that only defined template paramters can be assigned to */ 
{
  for(int i = 0; i < actual -> n; i++)
    {
      if(!find_param(actual->param[i]->name, def))
	{
	  fprintf(ferr, "warning: assignment to undefined parameter `%s' (in %s)\n", actual->param[i]->name, context_descr);
	}
    }
}

static int ntemplinstantiations = 0;

void expand_conjunction(struct conjunction *C)
{
  int i, j;

  if(C == 0) return;

  for(i = 0; i < C -> n; i++)
    {
      struct term *term;
      term = C->term[i];
      if(term == 0) 
	{
	  fprintf(ferr, "funny...\n");
	  continue;
	}
      if(term -> tag == TEMPL_CALL)
	{
	  int ti;
	  ti = templates.id(term -> value);

	  if(ti == -1)
	    {
	      fprintf(ferr, "error: call to undefined template `%s'\n", term -> value);
	      term -> tag = TYPE;
	      term -> type = 0;
	    }
	  else
	    {
	      struct conjunction *tC;
	
	      ntemplinstantiations++;

	      assert((templ_nest + 1) * 2 < TABLE_SIZE);

	      templates[ti] -> calls ++;
	      templ_stack[templ_nest] = templates[ti];
	      templ_crefs[templ_nest] ++;

	      check_params(term->params, templates[ti]->params);
	      templ_env[templ_nest*2] = expand_params(templates[ti] -> params);
	      templ_env[templ_nest*2 + 1] = expand_params(term -> params);

	      templ_nest++;

	      if((tC = copy_conjunction(templates[ti] -> constraint)))
		 expand_conjunction(tC);
	      
	      if(tC && tC -> n > 0)
		{
		  C -> term[i] = tC -> term[0];
		  for(j = 1; j < tC -> n; j++)
		    {
		      C -> term[C -> n++] = tC -> term[j];
		    }
		}
	      else
		{
		  fprintf(ferr, "warning: template call to `%s' expanding to nothing (?)\n", term -> value);
		  term -> tag = TYPE;
		  term -> type = 0;
		}
	      
	      templ_nest--;
	    }
	}
      else if(term -> tag == TEMPL_PAR)
	{
	  struct param *p;

	  p = lookup_param(term -> value);

	  if(p == 0 || p -> value == 0)
	    {
	      fprintf(ferr, "in `%s': template parameter `$%s' without value.\n", context_descr, term -> value);
	      term -> tag = TYPE;
	      term -> type = 0;
	      exit(1);
	    }
	  else
	    {
	      if(p->value -> n > 0)
		{
		  C -> term[i] = p->value -> term[0];
		  for(j = 1; j < p->value -> n; j++)
		    {
		      C -> term[C -> n++] = p->value -> term[j];
		    }
		}
	      else
		{
		  fprintf(ferr, "warning: template parameter `$%s' expanding to nothing (???)\n", term -> value);
		  term -> tag = TYPE;
		  term -> type = 0;
		}
	    }
	}
      else
	expand_term(C -> term[i]);
    }
}

//
// enter sorts `created' in expansion into type table
//

void check_sorts_conjunction(struct conjunction *);

void check_sorts_avm(struct avm *A)
{
  int j;

  for (j = 0; j < A -> n; ++j)
    check_sorts_conjunction(A -> av[j] -> val);
}

void check_sorts_list(struct tdl_list *L)
{
  int i;

  for(i = 0; i < L -> n; i++)
    check_sorts_conjunction(L -> list[i]);
  
  if(L -> dottedpair)
    check_sorts_conjunction(L -> rest);

}

void check_sorts_term (struct term *T)
{
  switch(T -> tag)
    {
    case TYPE:
      {
	int id = types.id(T->value);
	if(id == -1)
	  {
	    struct type *typ;
	    typ = new_type(T->value, false);
	    typ->def = templ_stack[0]->loc;
	    id = typ->id;
	    typ->implicit = 1;
	  }
	
	T->type = id;
      }
      break;
    case ATOM:
    case STRING:
      break;
    case COREF:
      break;
    case FEAT_TERM:
      check_sorts_avm(T -> A);
      break;
    case LIST:
    case DIFF_LIST:
      check_sorts_list(T -> L);
      break;
    case TEMPL_PAR:
    case TEMPL_CALL:
      assert(!"this cannot happen");
      break;
    default:
      fprintf(ferr, "unknown kind of term: %d\n", T -> tag);
      break;
    }
}

void check_sorts_conjunction(struct conjunction *C)
{
  int i;

  if(C == 0) return;

  for(i = 0; i < C -> n; i++)
    check_sorts_term(C -> term[i]);
}

void expand_templates()
{
  int i;

  fprintf(fstatus, "- expanding templates: ");

  for(i = 0; i < types.number(); i++)
    {
      context_descr = (char *) types.name(i).c_str();
      clear_templ_crefs();
      crefs = types[i]->coref;
      expand_conjunction(types[i]->constraint);
      check_sorts_conjunction(types[i]->constraint);
    }

  fprintf(fstatus, "%d template instantiations\n", ntemplinstantiations);
}
