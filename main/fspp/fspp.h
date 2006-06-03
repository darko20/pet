#ifndef _FSPP_H_
#define _FSPP_H_

#include <string>

int
preprocessor_initialize(const char *preproc_pathname);

std::string
preprocess(const char *inputstring, const char *format);

//#include <ecl.h>
//extern "C" void initialize_eclpreprocessor(cl_object cblock);

#endif
