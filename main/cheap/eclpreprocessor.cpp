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
#include <fspp.h>   

#include "eclpreprocessor.h"
#include "cheap.h"

#include "unicode.h"
extern class EncodingConverter *Conv; // (bmw) why does this seem necessary?

#define PRE_EXT ".fsr"

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
#ifdef HAVE_ICU
  UnicodeString u_yyresult = preprocess(s.c_str(), _format);
  string yyresult = Conv->convert(u_yyresult);
  
  _stage_two.tokenize(yyresult, result);
#else
  throw tError("FSPP tokenizer not available (please compile cheap with Unicode support)");
#endif
}
