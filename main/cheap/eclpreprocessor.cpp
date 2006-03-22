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

#include "petecl.h"
#include "eclpreprocessor.h"
//extern "C" void initialize_ecllispaddon(cl_object cblock);
extern "C" void initialize_eclpreprocessor(cl_object cblock);

#include "cheap.h"
#include "utility.h"

#define PRE_EXT ".fsr"

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
#ifdef HAVE_PREPROC
  read_VV(OBJNULL,initialize_eclpreprocessor);
#endif

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

tFSRTokenizer::tFSRTokenizer(const char *grammar_path) {
  char *preproc_filename = cheap_settings->value("preprocessor");
  if (preproc_filename == NULL)
    throw tError("No `preprocessor' setting found");
                 
  string preproc_pathname 
    = find_file(preproc_filename, PRE_EXT, grammar_path);
  if (preproc_pathname.empty()) 
    throw tError(string("Preprocessor spec file ")
                 + preproc_filename + "could not be found") ;

  preprocessor_initialize(preproc_pathname.c_str());

  _format = ":yy";
}

void tFSRTokenizer::tokenize(myString s, inp_list &result) {
  string yyresult = preprocess(s.c_str(), _format);
  
  _stage_two.tokenize(yyresult, result);
}
