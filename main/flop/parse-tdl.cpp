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

/* parser for files in TDL syntax */

/* straightforward hand implemented top down parser, builds simple internal
   representation that closely resembles the BNF */

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
#include "utility.h"
#include "options.h"
#include "lex-tdl.h"

int default_status = NO_STATUS; /* for section-wide default status values */

void tdl_start(int toplevel);

int template_mode = 0;
/* true when we are inside a template definition/call, so that we can support
   attribute names in template parameters. the idea is to delay inserting
   names into the type table until a the template is actually called */

void tdl_domainname()
{
  if(LA(0)->tag == T_COLON)
    {
      consume(1);
      if(LA(0)->tag != T_ID)
	{
	  syntax_error("expecting domain name", LA(0));
	}
      else consume(1);
    }
  else if(LA(0)->tag == T_QUOTE)
    {
      consume(1);
      if(LA(0)->tag != T_ID)
	{
	  syntax_error("expecting domain name", LA(0));
	}
      else consume(1);
    }
  else if(LA(0)->tag == T_STRING)
    {
      consume(1);
    }
  else
    {
      syntax_error("expecting domain name", LA(0));
    }
}

int tdl_opt_inst_status()
{
  char *statusname = NULL;
  
  if(LA(0)->tag == T_COLON && is_keyword(LA(1), "status"))
    {
      consume(2);
      statusname = match(T_ID, "status value", false);
    }

  if(statusname)
    {
      strtolower(statusname);
      if(statustable.id(statusname) == -1)
	statustable.add(statusname);
      return statustable.id(statusname);
    }
  else
    return NO_STATUS;
}

int tdl_option(bool readonly)
// returns value of last `status option, if any
{
  char *statusname = NULL;

  if(LA(0)->tag != T_KEYWORD)
    {
      syntax_error("option expected", LA(0));
    }
  else
    {
      if(is_keyword(LA(0), "status"))
	 {
	   
	   consume(1);
	   match(T_COLON, "`:' after `status'", true);

	   if(!readonly)
	     statusname = match(T_ID, "status value", readonly);
	   else
	     match(T_ID, "status value", readonly);
	 }
      else
	{
	  syntax_error("unknown option", LA(0));
	  consume(1);
	}
    }
  
  if(statusname)
    {
      strtolower(statusname);
      if(statustable.id(statusname) == -1)
	statustable.add(statusname);

      return statustable.id(statusname);
    }
  else
    return NO_STATUS;
}

void tdl_subtype_def(char *name, char *printname)
{
  int nr = 0;
  struct type *subt = NULL;
  bool readonly = false;

  while(LA(0)->tag == T_ISA)
    {
      char *super;
      
      consume(1);

      super = match(T_ID, "name of supertype", false);
      strtolower(super);
      
      if(super)
	{
	  // name ISA super

	  int sub, sup;
	  struct type *supt;

	  sub = types.id(name); sup = types.id(super);

	  if(!readonly)
	    {
	      if(sub == -1)
		{
		  subt = new_type(name, false);
		  subt->def = LA(0)->loc; LA(0)->loc = NULL;
		}
	      else
		{
		  subt = types[sub];
		  if(subt->implicit == false)
		    {
		      if(!allow_redefinitions)
			fprintf(ferr, "warning: redefinition of `%s' at %s:%d\n",
				types.name(sub).c_str(),
				LA(0)->loc->fname, LA(0)->loc->linenr);
		      
		      undo_subtype_constraints(subt->id);
		      
		      subt->constraint = NULL;
		    }
		}
	      subt->printname = printname; printname = 0;
	      subt->implicit = false;
	      
	      if(sup == -1)
		{
		  supt = new_type(super, false);
		  if(LA(0)->loc)
		    {
		      supt->def = LA(0)->loc; LA(0)->loc = NULL;
		    }
		  else
		    supt->def = subt->def;
		  supt->implicit = true;
		}
	      else
		{
		  supt = types[sup];
		}
	      subtype_constraint(subt->id, supt->id);
	      subt->parents = cons(supt->id, subt->parents);
	    }
        }
      nr ++; name = super;
    }
  
  if(nr < 1) syntax_error("at least one supertype expected", LA(0));
  
  while(LA(0)->tag == T_COMMA)
    {
      int status;
      consume(1);
      status = tdl_option(readonly);
      if(!readonly && status != NO_STATUS)
	{
	  if(nr > 1)
	    {
	      syntax_error("not sure about the semantics of this - ignoring", LA(0));
	    }
	  else
	    {
	      subt->status = status;
	      subt->defines_status = true;
	    }
	}
    }
}

struct param *tdl_templ_par(struct coref_table *, bool readonly);
void tdl_template_def(char *name);

char *tdl_attribute(struct coref_table *co, bool readonly)
{
  if(LA(0)->tag == T_DOLLAR)
    { 
      struct param *par;
      char *s;

      par = tdl_templ_par(co, readonly);

      if(par->value != NULL)
	{
	  syntax_error("unexpected assignment to template parameter within body", LA(0));
	}

      s = (char *) salloc(strlen(par->name) + 2);
      strcpy(s, "$");
      strcat(s, par->name);
      return s;
    }
  else
    {
      if(!readonly)
	{
	  char *s = NULL;
	  s = match(T_ID, "attribute", false);
	  strtoupper(s);
	  
	  if(attributes.id(s) == -1)
	    attributes.add(s);

	  return s;
	}
      else
	{
	  match(T_ID, "attribute", true);
	  return NULL;
	}
    }
}

struct conjunction *tdl_conjunction(struct coref_table *, bool readonly);

struct attr_val *tdl_attr_val(struct coref_table *co, bool readonly)
{
  struct attr_val *av = NULL, *av_inner = NULL, *av_outer = NULL;

  if(!readonly)
    {
      av = new_attr_val();
      av->attr = tdl_attribute(co, readonly);
      av_inner = av_outer = av;
    }
  else
    tdl_attribute(co, readonly);

  while(LA(0)->tag == T_DOT)
    {
      // build a "path" of attribute-value structs, av_inner is the attr_val
      // struct at the bottom, av_outer at the top of the path
      struct term *T = NULL;
      
      consume(1);

      if(!readonly)
	{
	  av_inner = new_attr_val();
	  av_outer->val = new_conjunction();
	  T = add_term(av_outer->val, new_avm_term());
	  T->A->n = 1;
	  T->A->av[0] = av_inner;
	  av_inner->attr = tdl_attribute(co, readonly);
	  av_outer = av_inner;
	}
      else
	tdl_attribute(co, readonly);
    }

  if(!readonly)
    av_inner->val = tdl_conjunction(co, readonly);
  else
    tdl_conjunction(co, readonly);

  return av;
}

struct avm *tdl_feature_term(struct coref_table *co, bool readonly)
{
  struct avm *A = NULL;

  match(T_LBRACKET, "`[' starting feature term", true);

  if(!readonly) A = new_avm();
  
  if(LA(0)->tag != T_RBRACKET)
    {
      if(!readonly)
	add_attr_val(A, tdl_attr_val(co, readonly));
      else
	tdl_attr_val(co, readonly);

      while(LA(0)->tag == T_COMMA)
	{
	  consume(1);
	  if(!readonly)
	    add_attr_val(A, tdl_attr_val(co, readonly));
	  else
	    tdl_attr_val(co, readonly);
	}
    }

  match(T_RBRACKET, "`]' at the end of feature term", true);

  return A;
}

struct tdl_list *tdl_diff_list(struct coref_table *co, bool readonly)
{
  struct tdl_list *L = NULL;

  if(!readonly)
    {
      L = new_list();
      L->difflist = 1;
    }

  match(T_LDIFF, "`<!' starting difference list", true);

  if(LA(0)->tag != T_RDIFF)
    {
      if(!readonly)
	add_conjunction(L, tdl_conjunction(co, readonly));
      else
	tdl_conjunction(co, readonly);

      while(LA(0)->tag == T_COMMA)
	{
	  consume(1);
	  if(!readonly)
	    add_conjunction(L, tdl_conjunction(co, readonly));
	  else
	    tdl_conjunction(co, readonly);
	}
    }

  match(T_RDIFF, "`!>' at the end of difference list", true);

  return L;
}

struct tdl_list *tdl_list(struct coref_table *co, bool readonly)
{
  struct tdl_list *L = NULL;
  int openlist = 0;

  match(T_LANGLE, "`<' starting list", true);

  if(!readonly) L = new_list();

  if(LA(0)->tag != T_RANGLE)
    {
      if(!readonly)
	add_conjunction(L, tdl_conjunction(co, readonly));
      else
	tdl_conjunction(co, readonly);
      
      while(LA(0)->tag == T_COMMA)
	{
	  consume(1);
	  
	  if(LA(0)->tag == T_DOT)
	    {
	      consume(1);
	      match(T_DOT, "`...' - got `.'", true);
	      match(T_DOT, "`...' - got `..'", true);
	      openlist = 1;
	    }
	  else
	    {
	      if(!readonly)
		add_conjunction(L, tdl_conjunction(co, readonly));
	      else
		tdl_conjunction(co, readonly);
	    }
	}
      
      if(!openlist)
	{
	  if(LA(0)->tag == T_DOT)
	    { // dotted pair at the end
	      consume(1);
	      if(!readonly)
		{
		  L->dottedpair = 1;
		  L->rest = tdl_conjunction(co, readonly);
		}
	      else
		tdl_conjunction(co, readonly);
	    }
	}

      if(!readonly) L->openlist = openlist;
    }

  match(T_RANGLE, "`>' at the end of list", true);

  return L;
}

int tdl_coref(struct coref_table *co, bool readonly)
{
  char *name = NULL;

  match(T_HASH, "`#'", true);

  if(!readonly)
    {
      name = match(T_ID, "coreference name", false);
    }
  else
    {
      match(T_ID, "coreference name", true);
    }

  if(!readonly && name)
    return add_coref(co, name);

  return -1;
}

struct param *tdl_templ_par(struct coref_table *co, bool readonly)
{
  struct param *p = NULL;

  match(T_DOLLAR, "`$' at start of templ_par", true);

  if(!readonly)
    {
      p = (struct param *) salloc(sizeof(struct param));
      p->value = NULL;
      p->name = match(T_ID, "templ-var", false);
    }
  else
    match(T_ID, "templ-var", true);

  if(LA(0)->tag == T_EQUALS)
    {
      consume(1);
      if(!readonly)
	p->value = tdl_conjunction(co, readonly);
      else
	tdl_conjunction(co, readonly);
    }
  
  return p;
}

struct param_list *tdl_templ_par_list(struct coref_table *co, bool readonly)
{
  struct param_list *pl = NULL;

  int sv = template_mode;
  template_mode = 1;

  if(!readonly)
    {
      pl = (struct param_list *) salloc(sizeof(struct param_list));
      pl->n = 0;
      pl->param = (struct param **) salloc(TABLE_SIZE * sizeof(struct param*));
    }

  match(T_LPAREN, "`(' starting template parameter list", true);

  if(LA(0)->tag != T_RPAREN)
    {
      if(!readonly)
	pl->param[pl->n++] = tdl_templ_par(co, readonly);
      else
	tdl_templ_par(co, readonly);

      while(LA(0)->tag == T_COMMA)
	{
	  consume(1);
	  
	  if(!readonly)
	    {
	      assert(pl->n < TABLE_SIZE);
	      pl->param[pl->n++] = tdl_templ_par(co, readonly);
	    }
	  else
	    tdl_templ_par(co, readonly);
	}
    }

  match(T_RPAREN, "`)' at the end of  template parameter list", true);

  template_mode = sv;

  return pl;
}

struct templ *tdl_templ_call(struct coref_table *co, bool readonly)
{
  struct templ *t = NULL;

  match(T_AT, "`@' starting template call", true);
  
  if(!readonly)
    {
      t = (struct templ *) salloc(sizeof(struct templ));
      t->name = match(T_ID, "template name", false);
      if(templates.id(t->name) == -1)
	{
	  fprintf(ferr, "warning: call to undefined template `%s' at %s:%d\n",
		  t->name, LA(0)->loc->fname, LA(0)->loc->linenr);
	}
      t->params = tdl_templ_par_list(co, readonly);
      t->constraint = NULL;
    }
  else
    {
      match(T_ID, "template name", false);
      tdl_templ_par_list(co, readonly);
    }

  
  return t;
}


struct term *tdl_term(struct coref_table *co, bool readonly)
{
  struct term *t = NULL;

  if(!readonly) t = new_term();

  if(LA(0)->tag == T_ID)
    { // type name

      if(!readonly)
	{
	  int id = -1;
	  struct type *typ = NULL;
	  
	  t->tag = TYPE;
	  t->value = LA(0)->text; LA(0)->text = NULL;
	  strtolower(t->value);
	  
	  id = types.id(t->value);
	  if(id == -1 && !template_mode)
	    {
	      typ = new_type(t->value, false);
	      typ->def = LA(0)->loc; LA(0)->loc = NULL;
	      id = typ->id;
	      typ->implicit = true;
	    }
	  
	  t->type = id;
	}

      consume(1);
    }
  else if(LA(0)->tag == T_QUOTE)
    { // atom
      consume(1);
      if(!readonly)
	{
	  t->tag = ATOM;
	  t->value = match(T_ID, "atom expected (term)", false);
	  char *printname = strdup(t->value);
	  strtolower(t->value);

	  string s = "'" + string(t->value);
	  if(types.id(s) == -1)
	    {
	      struct type *ty = new_type(s, false);
	      ty->def = LA(0)->loc; LA(0)->loc = NULL;
	      subtype_constraint(ty->id, BI_SYMBOL);
	      ty->status = ATOM_STATUS;
	      ty->printname = printname; printname = 0;
	    }

	  if(printname) free(printname);
	}
      else
	match(T_ID, "atom expected (term)", true);
    }
  else if(LA(0)->tag == T_STRING)
    { // atom
      if(!readonly)
	{
	  t->tag = STRING;
	  t->value = LA(0)->text; LA(0)->text = NULL;

	  string s = "\"" + string(t->value) + "\"";
      
	  if(types.id(s) == -1)
	    {
	      struct type *ty = new_type(s, false);
	      ty->def = LA(0)->loc; LA(0)->loc = NULL;
	      subtype_constraint(ty->id, BI_STRING);
	      ty->status = ATOM_STATUS;
	      ty->printname = t->value;
	    }
	}

      consume(1);
    }
  else if(LA(0)->tag == T_LBRACKET)
    {
      if(!readonly)
	{
	  t->tag = FEAT_TERM;
	  t->A = tdl_feature_term(co,readonly);
	}
      else
	tdl_feature_term(co,readonly);
    }
  else if(LA(0)->tag == T_LDIFF)
    {
      if(!readonly)
	{
	  t->tag = DIFF_LIST;
	  t->L = tdl_diff_list(co, readonly);
	}
      else
	tdl_diff_list(co, readonly);
    }
  else if(LA(0)->tag == T_LANGLE)
    {
      if(!readonly)
	{
	  t->tag = LIST;
	  t->L = tdl_list(co,readonly);
	}
      else
	tdl_list(co,readonly);
    }
  else if(LA(0)->tag == T_HASH)
    {
      if(!readonly)
	{
	  t->tag = COREF;
	  t->coidx = tdl_coref(co,readonly);
	}
      else
	tdl_coref(co, readonly);
    }
  else if(LA(0)->tag == T_AT)
    {
      if(!readonly)
	{
	  struct templ *temp;
	  t->tag = TEMPL_CALL;
	  temp = tdl_templ_call(co, readonly);
	  
	  t->value = temp->name;
	  t->params = temp->params;
	}
      else
	tdl_templ_call(co, readonly);
    }
  else if(LA(0)->tag == T_DOLLAR)
    {
      if(!readonly)
	{
	  struct param *par;
	  
	  par = tdl_templ_par(co, readonly);
	  if(par->value != NULL)
	    syntax_error("unexpected assignment to template parameter within body", LA(0));
	  
	  t->tag = TEMPL_PAR;
	  t->value = par->name;
	}
      else
	tdl_templ_par(co, readonly);
    }
  else
    {
      t = NULL;
      syntax_error("expecting term", LA(0));
    }

  return t;
}

struct conjunction *tdl_conjunction(struct coref_table *co, bool readonly)
{
  struct conjunction *con = NULL;

  if(!readonly)
    {
      con = new_conjunction();
      add_term(con, tdl_term(co, readonly));
    }
  else
    tdl_term(co, readonly);

  while(LA(0)->tag == T_AMPERSAND)
    {
      consume(1);
      if(!readonly)
	add_term(con, tdl_term(co, readonly));
      else
	tdl_term(co, readonly);
    }

  return con;
}

struct coref_table *new_coref_table()
{
  struct coref_table *t;

  t = (struct coref_table *) salloc(sizeof(struct coref_table));
  
  t->n = 0;
  t->coref = (char **) salloc(COREF_TABLE_SIZE * sizeof(char *));

  return t;
}

void tdl_opt_inflr(struct type *t)
{
  while(LA(0)->tag == T_INFLR)
    {
      if(verbosity > 9)
	fprintf(stderr, "inflr <%s>: `%s'\n",
		t == 0 ? "(global)" : types.name(t->id).c_str(),
		LA(0)->text);

      if(t == 0)
	global_inflrs = add_inflr(global_inflrs, LA(0)->text);
      else
	t->inflr = add_inflr(t->inflr, LA(0)->text);

      consume(1);
    }
}


void tdl_avm_def(char *name, char *printname, bool is_instance, bool readonly)
{
  int status = NO_STATUS;
  int t_id;
  struct type *t = NULL;

  if(!readonly)
    {
      t_id = types.id(name);
      
      if(t_id != -1)
	{
	  t = types[t_id];
	  
	  if(t->implicit == false)
	    {
	      if(!allow_redefinitions)
		fprintf(ferr, "warning: redefinition of `%s' at %s:%d\n",
			name, LA(0)->loc->fname, LA(0)->loc->linenr);
	      
	      undo_subtype_constraints(t->id);
	    }
	}
      else
	{
	  t = new_type(name, is_instance);
	}

      t->def = LA(0)->loc; LA(0)->loc = NULL;
      t->implicit = false;
      t->printname = printname; printname = 0;
    }
  
  match(T_ISEQ, "avm definition starting with `:='", true);

  tdl_opt_inflr(t);

  if(!readonly)
    {
      t->coref = new_coref_table();
      t->constraint = tdl_conjunction(t->coref, readonly);
    }
  else
    tdl_conjunction(NULL, readonly);

  // process TDL rule definition code, if present
  if (LA(0)->tag == T_ARROW)
    {
      consume(1);
      if (LA(0)->tag != T_LANGLE)
        syntax_error("rule definintion not followed by a list", LA(0));

      if (! readonly)
        {
          // read the list and build the ARGS attribute value pair
          struct attr_val *attrval = new_attr_val();
          attrval->val = new_conjunction();
          add_term(attrval->val, tdl_term(t->coref, readonly));
          char *s = (char *) malloc(strlen("ARGS"));
          attrval->attr = strcpy(s, "ARGS");
          struct avm *A = new_avm();
          add_attr_val(A, attrval);
          struct term *T = new_term();
	  T->tag = FEAT_TERM;
          T->A = A;
          add_term(t->constraint, T);
        }
      else
        tdl_term(NULL, readonly);
    }

  while(LA(0)->tag == T_COMMA)
    {
      consume(1);
      status = tdl_option(readonly);
    }
  
  if(status == NO_STATUS && is_instance)
    status = default_status;
  
  if(!readonly && t)
    {
      t->status = status;
      if(status != NO_STATUS) t->defines_status = 1;
    }
}

void tdl_type_def()
{
  char *name , *printname;
  
  if(LA(0)->tag == T_STRING)
  {
      name = match(T_STRING, "type name", false);
      string s = "\"" + string(name) + "\"";
      printname = name;
      name = strdup(s.c_str());
  }
  else
  {
      name = match(T_ID, "type name", false);
      printname = strdup(name);
      strtolower(name);
  }
  
  if(LA(0)->tag == T_ISEQ)
    {
      tdl_avm_def(name, printname, false, false);
    }
  else if(LA(0)->tag == T_ISA)
    {
      tdl_subtype_def(name, printname);
    }
  else
    {
      free(printname);
      syntax_error("avm or subtype definition expected", LA(0));
      recover(T_DOT);
    }

  match(T_DOT, "`.' at end of type definition", true);
}

void tdl_instance_def()
{
  char *name = 0, *printname = 0;
  bool readonly = false;
  
  if(LA(0)->tag == T_STRING)
  {  
      name = match(T_STRING, "instance name", false);
      string s = "\"" + string(name) + "\"";
      printname = name;
      name = strdup(s.c_str());
  }
  else
  {
      name = match(T_ID, "instance name", false);
      printname = strdup(name);
      strtolower(name);
  }

  char *iname = (char *) salloc(strlen(name) + 2);
  sprintf(iname, "$%s", name);

  tdl_avm_def(iname, printname, true, readonly);
  
  free(iname);

  match(T_DOT, "`.' at end of instance definition", true);
}

void tdl_template_def(char *name)
{
  struct templ *t;
  int old = -1;

  template_mode = 1;

  t = (struct templ *) salloc(sizeof(struct templ));

  t->name = name;
  t->calls = 0;

  if((old = templates.id(t->name)) >= 0)
    {
      if(!allow_redefinitions)
        fprintf(ferr, "warning: redefinition of template `%s' at %s:%d\n",
                t->name, LA(0)->loc->fname, LA(0)->loc->linenr);
      templates[old] = t;
    }
  else
    {
      templates[templates.add(t->name)] = t;
    }

  t->loc = LA(0)->loc; LA(0)->loc = NULL;
  t->params = tdl_templ_par_list(NULL, false);

  match(T_ISEQ, "`:=' in template definition", true);
  
  t->coref = new_coref_table();

  t->constraint = tdl_conjunction(t->coref, false);

  while(LA(0)->tag == T_COMMA)
    {
      consume(1);
      tdl_option(true);
    }

  template_mode = 0;
}

void block_not_closed(char *kind, char *fname, int lnr)
{
  char *msg;

  msg = (char *) salloc(strlen(kind) + strlen(fname) + 80);
  sprintf(msg, "missing `end' for %s block starting at %s:%d", kind, fname, lnr);
  syntax_error(msg, LA(0));
  free(msg);
}

void tdl_block()
{
  char *b_fname;
  int b_lnr;

  static int nesting = 0;

  b_fname = curr_fname();
  b_lnr = curr_line();

  nesting++;

  consume(1); // `begin`
  
  match(T_COLON, "`:'", true);
  
  if(LA(0)->tag != T_KEYWORD)
    {
      syntax_error("expecting a keyword to start block", LA(0));
      recover(T_KEYWORD); // futile attempt ...
    }

  if(is_keyword(LA(0), "control"))
    {
      consume(1); match(T_DOT, "`.'", true);

      syntax_error("tdl control block not implemented", LA(0));

      recover(T_DOT);
    }
  else if(is_keyword(LA(0), "declare"))
    {
      consume(1); match(T_DOT, "`.'", true);

      syntax_error("tdl declare block not implemented", LA(0));

      recover(T_DOT);
    }
  else if(is_keyword(LA(0), "domain"))
    {
      consume(1);
      
      tdl_domainname();
      match(T_DOT, "`.'", true);
      
      tdl_start(0);

      optional(T_COLON);
      
      if(!is_keyword(LA(0), "end"))
	{
	  block_not_closed("domain", b_fname, b_lnr);
	}
      else
	{
	  match_keyword("end");
	  match(T_COLON, "`:'", true);
	  match_keyword("domain");
	  tdl_domainname();
	  match(T_DOT, "`.'", true);
	}
    }
  else if (is_keyword(LA(0), "instance"))
    {
      consume(1);

      int status, saved_status;

      status = tdl_opt_inst_status();

      saved_status = default_status;
      if(status != NO_STATUS)
	default_status = status;

      match(T_DOT, "`.'", true);

      do
	if(LA(0)->tag == T_ID || LA(0)->tag == T_STRING)
	  {
	    tdl_instance_def();
	  }
	else if(is_keyword(LA(0), "end") ||
		(LA(0)->tag == T_COLON && is_keyword(LA(1), "end")))
	  {
	    break;
	  }
        else if(LA(0)->tag == T_INFLR)
	  {
	    tdl_opt_inflr(0);
	  }
	else if(LA(0)->tag == T_EOF)
	  {
	    break;
	  }
	else
	  {
	    tdl_start(0);
	  }
      while (1);

      optional(T_COLON);
      
      if(!is_keyword(LA(0), "end"))
	{
	  block_not_closed("instance", b_fname, b_lnr);
	}
      else
	{
	  match_keyword("end");
	  match(T_COLON, "`:'", true);
	  match_keyword("instance");
	  match(T_DOT, "`.'", true);
	}

      default_status = saved_status;
    }
  else if (is_keyword(LA(0), "lisp"))
    {
      consume(1); match(T_DOT, "`.'", true);

      lisp_mode = 1;

      while(LA(0)-> tag == T_LISP)
	{
	  consume(1);
	}
      
      lisp_mode = 0;

      optional(T_COLON);

      if(!is_keyword(LA(0), "end"))
	{
	  block_not_closed("lisp", b_fname, b_lnr);
	}
      else
	{
	  match_keyword("end");
	  match(T_COLON, "`:'", true);
	  match_keyword("lisp");
	  match(T_DOT, "`.'", true);
	}
    }
  else if (is_keyword(LA(0), "template"))
    {
      consume(1); match(T_DOT, "`.'", true);

      do
	if(LA(0)->tag == T_ID)
	  {
            char *name;
            name = match(T_ID, "template name", false);
	    tdl_template_def(name);
            match(T_DOT, "`.' at the end of template definition", true);
          }
	else if(is_keyword(LA(0), "end") ||
		(LA(0)->tag == T_COLON && is_keyword(LA(1), "end")))
	  {
	    break;
	  }
	else if (LA(0)->tag == T_EOF)
	  {
	    break;
	  }
	else
	  {
	    tdl_start(0);
	  }
      while (1);

      optional(T_COLON);
            if(!is_keyword(LA(0), "end"))
	{
	  block_not_closed("template", b_fname, b_lnr);
	}
      else
	{
	  match_keyword("end");
	  match(T_COLON, "`:'", true);
	  match_keyword("template");
	  match(T_DOT, "`.'", true);
	}
    }
  else if (is_keyword(LA(0), "type"))
    {
      consume(1); match(T_DOT, "`.'", true);

      do
	if(LA(0)->tag == T_ID || LA(0)->tag == T_STRING)
	  {
	    tdl_type_def();
	  }
	else if(is_keyword(LA(0), "end") ||
		(LA(0)->tag == T_COLON && is_keyword(LA(1), "end")))
	  {
	    break;
	  }
	else if (LA(0)->tag == T_EOF)
	  {
	    break;
	  }
        else if(LA(0)->tag == T_INFLR)
	  {
	    tdl_opt_inflr(0);
	  }
	else
	  {
	    tdl_start(0);
	  }
      while (1);

      optional(T_COLON);

      if(!is_keyword(LA(0), "end"))
	{
	  block_not_closed("type", b_fname, b_lnr);
	}
      else
	{
	  match_keyword("end");
	  match(T_COLON, "`:'", true);
	  match_keyword("type");
	  match(T_DOT, "`.'", true);
	}
    }
  else
    {
      syntax_error("unknown/illegal type of block", LA(0));
      recover(T_DOT);
    }

  nesting --;
}

void tdl_defdomain_option()
{
  match(T_COLON, "`:'", true);
  
  if(is_keyword(LA(0), "errorp"))
    {
      consume(1);
      
      match(T_ID, "`t' or `nil'", true);
    }
  else if (is_keyword(LA(0), "delete-package-p"))
    {
      consume(1);

      match(T_ID, "`t' or `nil'", true);
    }
  else
    {
      syntax_error("unknown defdomain option", LA(0));
    }
}

void tdl_statement()
{
  if(is_keyword(LA(0), "defdomain"))
    {
      consume(1);
      tdl_domainname();

      while(LA(0)->tag != T_DOT && LA(0)->tag != T_EOF)
	{
	  tdl_defdomain_option();
	}

      match(T_DOT, "`.'", true);

    }
  else if(is_keyword(LA(0), "deldomain"))
    {
      consume(1);
      tdl_domainname();

      while(LA(0)->tag != T_DOT && LA(0)->tag != T_EOF)
	{
	  tdl_defdomain_option();
	}

      match(T_DOT, "`.'", true);
    }
  else if(is_keyword(LA(0), "expand-all-instances"))
    {
      consume(1);

      match(T_DOT, "`.'", true);
    }
  else if(is_keyword(LA(0), "include"))
    {
      consume(1);

      if(LA(0)->tag != T_STRING)
	{
	  syntax_error("expecting name of file to include", LA(0));
	  recover(T_DOT);
	}
      else
	{
	  char *ofname, *fname;

	  ofname = LA(0)->text; LA(0)->text = NULL;
	  consume(1);

	  match(T_DOT, "`.'", true);

	  fname = find_file(ofname, TDL_EXT);
	  
	  if(!fname)
	    {
	      fprintf(ferr, "file `%s' not found. skipping...\n", ofname);
	    }
	  else
	    {
	      push_file(fname, "including");
	    }
	}
    }
  else if(is_keyword(LA(0), "leval"))
    {
      consume(1);

      lisp_mode = 1;

      if(LA(0)->tag != T_LISP)
	{
	  syntax_error("expecting LISP expression", LA(0));
	  recover(T_DOT);
	}
      else
	consume(1);

      lisp_mode = 0;
      match(T_DOT, "`.'", true);
    }
  else if(is_keyword(LA(0), "end!"))
    {
      consume(1);
      match(T_DOT, "`.'", true);
    }
  else
    {
      syntax_error("unknown type of statement", LA(0));
      recover(T_DOT);
    }
}

void tdl_start(int toplevel)
{
  int start = tokensdelivered;

  do
    {
      if(LA(0)->tag == T_EOF)
	{
	  if(toplevel)
	    {
	      consume(1);
	      return;
	    }
	  syntax_error("end of input, but not on toplevel", LA(0));
	  return;
	}
      else if(LA(0)->tag == T_KEYWORD)
	{
	  if(is_keyword(LA(0), "begin"))
	    tdl_block();
	  else if(!is_keyword(LA(0), "end"))
	    tdl_statement();
	}
      else if(LA(0)->tag == T_COLON)
	{
	  if(LA(1)->tag == T_KEYWORD)
	    {
	      if(is_keyword(LA(1), "begin"))
		{
		  consume(1);
		  tdl_block();
		}
	      else if(!is_keyword(LA(1), "end"))
		{
		  consume(1);
		  tdl_statement();
		}
	      else
		break;
	    }
	  else
	    {
	      syntax_error("expecting keyword after `:'", LA(0));
	      recover(T_DOT);
	      break;
	    }
	}
      else if(toplevel)
	{
	  syntax_error("expecting block or statement", LA(0));
	  recover(T_DOT);
	}
      else
	break;
    } while (1);

  if(tokensdelivered == start)
    {
      syntax_error("expecting nested block or statement",
		   LA(0));
      consume(1);
    }
}
