/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2003 Ulrich Callmeier uc@coli.uni-sb.de
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

/* ECL integration */

#include "pet-config.h"
#include "petecl.h"
#include "stdlib.h"

//
// Helper functions.
//

char *
ecl_decode_string(cl_object x) {
  char *result = 0;
  switch (type_of(x)) {
#ifdef HAVE_STRUCT_ECL_STRING
    case t_string:
      result = (char *) x->string.self;
      result[x->string.fillp] = '\0';
      break;
#endif
#ifdef HAVE_STRUCT_ECL_BASE_STRING
    case t_base_string:
      result = (char *) x->base_string.self;
      result[x->base_string.fillp] = '\0';
      break;
#endif
    default: ;
  }
  return result;
}

int *
ecl_decode_vector_int(cl_object x) {
  int i;
  int *v = 0;
  cl_object y;

  if(type_of(x) != t_vector)
    return v;
    
  v = malloc(sizeof(int) * (x->vector.fillp + 1));
  v[x->vector.fillp] = -1; // end marker

  for(i = 0; i < x->vector.fillp; i++) {
    y = aref1(x, i);
    if(type_of(y) != t_fixnum)
      v[i] = -1;
    else
      v[i] = fix(y);
  }

  return v; // caller has to free v
}


/** Evaluate the given string \a form in the lisp listener
 * \pre \a form must be a valid s-expression
 * \returns the object returned by the evaluation
 */
cl_object
ecl_eval_sexpr(char *form) {
  cl_object obj
    = funcall(2
              , c_string_to_object("read-from-string")
              , make_simple_string(form));
  return cl_safe_eval(obj, Cnil, NULL);
}


// ECL initialization function. Boots the ECL engine
int
ecl_initialize(int argc, char **argv) {
  cl_boot(argc, argv);
  return 0;
}

//
// Load a lisp file with the given name using the ECL interpreter.
//
void
ecl_load_lispfile(const char *fname) {
  funcall(4,
          c_string_to_object("load"),
          make_string_copy(fname),
          c_string_to_object(":verbose"),
          Cnil);
}

