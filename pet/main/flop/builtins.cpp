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

int create_grammar_info(char *name, char *grammar_version)
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
