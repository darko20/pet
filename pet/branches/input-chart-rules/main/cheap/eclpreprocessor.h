/* -*- Mode: C++ -*- */

#ifndef _ECLPREPROCESSOR_H
#define _ECLPREPROCESSOR_H

#include <string>
#include "yy-tokenizer.h"

/** Initialize the ecl based Lisp preprocessor engine. 
 * To get the preprocessor specifications file, look into the directory that is
 * specified by \a grammar_pathname for a file whose name is specified 
 * by the global \c "preprocessor" setting.
 * The \a format argument specifies the output format that is returned by the
 * function preprocess. At the moment, it can be either ":yy" or ":smaf"
 */
//int preprocessor_initialize(const char *grammar_pathname, const char *format);

/** Preprocess one string with the preprocessor.
 * The output format was fixed when the preprocessor engine was initialized.
 */
//std::string preprocess(const char *inputstring);

class tFSRTokenizer : public tTokenizer {
public:
  tFSRTokenizer(const char *grammar_path);

  virtual void tokenize(myString s, inp_list &result);

  virtual position_map position_mapping(){
    return _position_mapping;
  }

  virtual string description() {
    return "ecl/Lisp based tokenizer/preprocessor modules with FSR rules";
  }
private:
  tYYTokenizer _stage_two;
  position_map _position_mapping;

  char *_format;
  
};

#endif
