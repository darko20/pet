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

/* C bridge between Lisp functions and PET for the preprocessor
   code integration */

#include <stdlib.h>
#include <string>
#include <ecl.h>  

extern "C" void initialize_eclpreprocessor(cl_object cblock);

char *
ecl_decode_string(cl_object x) {
  if (type_of(x) != t_string) return NULL;
  // make sure the string is correctly terminated
  char *result = (char *) x->string.self;
  result[x->string.fillp] = '\0';
  return result;
}

// Initialization

/** Initialize the ecl based Lisp preprocessor engine. 
 * To get the preprocessor specifications file, look into the directory that is
 * specified by \a grammar_pathname for a file whose name is specified 
 * by the global \c "preprocessor" setting.
 * The \a format argument specifies the output format that is returned by the
 * function preprocess. At the moment, it can be either ":yy" or ":smaf"
 */
int
preprocessor_initialize(const char *preproc_pathname) {
  read_VV(OBJNULL,initialize_eclpreprocessor);

  cl_object result =
    funcall(2
            , c_string_to_object("preprocessor::x-read-preprocessor")
            , make_string_copy(preproc_pathname));
  return 0;
}

/** Preprocess one string with the preprocessor.  The output format was fixed
 * when the preprocessor engine was initialized.
 */
std::string
preprocess(const char *inputstring, const char *format) {
  cl_object resobj
    = funcall(4
              , c_string_to_object("preprocessor:preprocess")
              , make_string_copy(inputstring)
              , c_string_to_object(":format")
              , c_string_to_object(format)
              );
  char *result = ecl_decode_string(resobj);
  /*
  if (result) {
    fprintf(ferr, "FSR result: <%s>%d", result, resobj->string.fillp );
  } else {
    fprintf(ferr, "FSR result: NULL");
  }
  */
  return (result != NULL) ? std::string(result) : std::string();
}
