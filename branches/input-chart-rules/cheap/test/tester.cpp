/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 2002 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* main module (unit test driver) */

#include "pet-config.h"
#include "grammar.h"
#include "grammar-dump.h"
#include "settings.h"

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/CompilerOutputter.h>

#include <string>

using std::string;

// required global settings from cheap.cpp 
char * version_string = VERSION ;
FILE* ferr = stderr;
FILE* fstatus = stderr;
FILE* flog = NULL;
bool XMLServices = false;
tGrammar *Grammar;
settings *cheap_settings;

int
main(int argc, char **argv)
{
  // load grammar:
  if (argc != 2) {
    printf("usage: %s <grammar-name>\n", argv[0]);
    return 2;
  }
  string grampath = find_file(argv[1], GRAMMAR_EXT);
  if(grampath.empty()) {
    fprintf(ferr, "Grammar not found\n");
    return 3;
  }
  string gramname = raw_name(grampath.c_str());
  cheap_settings = new settings(gramname.c_str(), grampath.c_str(), "reading");
  Grammar = new tGrammar(grampath.c_str());
  fprintf(stderr, "\n");
  
  // create and setup unit testing objects:
  CppUnit::TestResultCollector result;
  CppUnit::CompilerOutputter outputter(&result, std::cerr);
  CppUnit::BriefTestProgressListener progress;
  CppUnit::TestResult controller;
  CppUnit::TestRunner runner;
  controller.addListener(&result);
  controller.addListener(&progress);      
  runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());
  
  // run tests:
  runner.run(controller);
  outputter.write();    
  
  return result.wasSuccessful() ? 0 : 1;
}
