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

int
main(int argc, char **argv)
{
  CppUnit::TestResultCollector result;
  CppUnit::CompilerOutputter outputter(&result, std::cerr);
  CppUnit::BriefTestProgressListener progress;
  CppUnit::TestResult controller;
  CppUnit::TestRunner runner;
  
  controller.addListener(&result);
  controller.addListener(&progress);      
  runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());
  runner.run(controller);
  outputter.write();    
  
  return result.wasSuccessful() ? 0 : 1;
}
