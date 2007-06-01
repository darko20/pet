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
#include <iostream>
#include <iomanip>   // format manipulation
#include <string>
//#include <ecl.h>   

#include "unicode.h"
#include "petecl.h"

using namespace std;

extern "C" void initialize_eclpreprocessor(cl_object cblock);

class EncodingConverter *Conv2;

// Initialization

int
preprocessor_initialize(const char *preproc_pathname) {
  read_VV(OBJNULL,initialize_eclpreprocessor);
  // fix_me: get rid of (ECL) WARNING messages
  funcall(2
          , c_string_to_object("preprocessor::x-read-preprocessor")
          , make_string_copy(preproc_pathname));
  Conv2 = new EncodingConverter("utf-8");
  return 0; 
}

/** Preprocess one string with the preprocessor.
 * Args:
 *  - inputstring
 *  - format: one of :smaf :saf :yy
 */
UnicodeString
preprocess(UnicodeString u_inputstring, const char *format) {
  const string inputstring2=Conv2->convert(u_inputstring);
  const char* inputstring=inputstring2.c_str();

  // cout << "INPUT: " << inputstring << endl;

  cl_object resobj
    = funcall(4
              , c_string_to_object("preprocessor:preprocess")
              , make_string_copy(inputstring)
              , c_string_to_object(":format")
              , c_string_to_object(format)
              );
  char *result = ecl_decode_string(resobj);
  
#ifdef DEBUG 
  if (result) {
    cout << "FSR result: <" << result << ">" << resobj->string.fillp << endl << endl;
  } else {
    cout << "FSR result: NULL" << endl << endl;
  }
#endif

  return (result != NULL) ? Conv2->convert(std::string(result)) : Conv2->convert(std::string());
}
