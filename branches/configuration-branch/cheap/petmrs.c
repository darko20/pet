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

/* C bridge between Lisp functions and PET for the MRS code integration */

#include <stdlib.h>

#include "petecl.h"
#include "petmrs.h"
#include "mfile.h"
#include "cppbridge.h"

extern void initialize_eclmrs(cl_object cl_block);

//
// Interface functions called from Lisp.
//

int
pet_type_to_code(cl_object type_name) {
  char *s = ecl_decode_string(type_name);
  return pet_cpp_lookup_type_code(s);
}

cl_object
pet_code_to_type(int code) {
  if(! pet_cpp_type_valid_p(code))
    return Cnil;

  return make_string_copy(pet_cpp_lookup_type_name(code));
}

int
pet_feature_to_code(cl_object feature_name) {
  char *s = ecl_decode_string(feature_name);
  return pet_cpp_lookup_attr_code(s);
}

cl_object
pet_code_to_feature(int code) {
  if(code < 0 || code >= pet_cpp_nattrs())
    return Cnil;
  
  return make_string_copy(pet_cpp_lookup_attr_name(code));
}

int
pet_fs_deref(int fs) {
  return fs;
}

int
pet_fs_cyclic_p(int fs) {
  return 0;
}

int 
pet_fs_valid_p(int fs) {
  return pet_cpp_fs_valid_p(fs);
}

int
pet_fs_path_value(int fs, cl_object vector_of_codes) {
  int res;
  
  int *v = ecl_decode_vector_int(vector_of_codes);
  
  res = pet_cpp_fs_path_value(fs, v);
  
  free(v);
  
  return res;
}

cl_object
pet_fs_arcs(int fs) {
  cl_object res;
  struct MFILE *f = mopen();
  int *v;
  int i;
  
  pet_cpp_fs_arcs(fs, &v);
  
  if(v) {
    mprintf(f, "(");
    for(i = 0; v[i] != -1; i+=2) {
      mprintf(f, "(%d . %d)",
              v[i], v[i+1]);
    }
    mprintf(f, ")");
    free(v);
  }
  
  res = c_string_to_object(mstring(f));
  mclose(f);
  return res;
}

int
pet_fs_type(int fs) {
  return pet_cpp_fs_type(fs);
}

int
pet_subtype_p(int t1, int t2) {
  return pet_cpp_subtype_p(t1, t2);
}

int
pet_glb(int t1, int t2) {
  return pet_cpp_glb(t1, t2);
}

int
pet_type_valid_p(int t) {
  return pet_cpp_type_valid_p(t);
}

char *
ecl_extract_mrs(int fs, char *mode, int cfrom, int cto) {
  cl_object result;
  result = funcall(3,
                   c_string_to_object("mrs::fs-to-mrs"),
                   MAKE_FIXNUM(fs),
                   make_string_copy(mode));
  return ecl_decode_string(result);
}

//
// ECL initialization function. Boots the ECL engine,
// loads user-specified (interpreted) lisp files, and
// initializes the compiled packages (currently only MRS).
//
int
mrs_initialize(const char *grammar_file_name, const char *vpm) {
  // Load lisp initialization files as specified in settings.

  ecl_cpp_load_files("preload-lisp-files", grammar_file_name);
    
  // Initialize the individual packages

  read_VV(OBJNULL, initialize_eclmrs);

  // Load lisp initialization files as specified in settings.

  ecl_cpp_load_files("postload-lisp-files", grammar_file_name);

  // [bmw] uncomment line below when debugging embedded Lisp MRS code
  //funcall(1, c_string_to_object("break"));

  if(vpm && *vpm )
    funcall(3,
            c_string_to_object("mt:read-vpm"),
            make_string_copy(vpm),
            c_string_to_object(":semi"));

  return 0;
}
