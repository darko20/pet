/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 2002 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* main module (unit test driver) */

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/CompilerOutputter.h>

#include <string>

#include "grammar.h"
#include "settings.h"

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
  // initialize grammar:
  char* grmstr = "/home/peter/Eclipse/pet-icr/sample/tokmap-grammar/grammar.grm";
  std::string base = raw_name(grmstr);
  cheap_settings = new settings(base.c_str(), grmstr, "reading");
  Grammar = new tGrammar(grmstr);
  
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
