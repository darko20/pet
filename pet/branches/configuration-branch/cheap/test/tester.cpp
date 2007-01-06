/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 2002 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* main module (unit test driver) */

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

int
main(int argc, char **argv)
{
    CppUnit::TextUi::TestRunner runner;
    CppUnit::TestFactoryRegistry &registry =
        CppUnit::TestFactoryRegistry::getRegistry();
    
    runner.addTest(registry.makeTest());
    bool wasSucessful = runner.run("", false);
    return wasSucessful;
}
