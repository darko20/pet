/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* special and builtin types */

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

int new_bi_type(char *name)
{
  type *t = new_type(name, false);
  t->def = new_location("builtin", 0, 0);
  return t->id;
}

void initialize_builtins()
{
  BI_TOP = new_bi_type("*top*");
}

int create_grammar_info(char *name, char *grammar_version, const char *vocabulary)
{
  struct type *t = NULL;
  struct term *T;
	  
  t = new_type(name, false);
  t->def = new_location("virtual", 0, 0);

  t->constraint = new_conjunction();
	  
  T = new_avm_term();
  add_term(t->constraint, T);

  add_term(add_feature(T->A, "GRAMMAR_VERSION"),
	   new_string_term(grammar_version));
  add_term(add_feature(T->A, "GRAMMAR_VOCABULARY"),
	   new_string_term((char *) vocabulary));
  add_term(add_feature(T->A, "GRAMMAR_NTYPES"),
	   new_int_term(types.number()));
  add_term(add_feature(T->A, "GRAMMAR_NTEMPLATES"),
	   new_int_term(templates.number()));
  // add this last, and force this feature to be counted
  struct conjunction *nattributes = add_feature(T->A, "GRAMMAR_NATTRIBUTES");
  add_term(nattributes, new_int_term(attributes.number()));

  unify_wellformed = false;
  t->thedag = dagify_tdl_term(t->constraint, t->id, 0);
  unify_wellformed = true;

  subtype_constraint(t->id, BI_TOP);

  return t->id;
}
