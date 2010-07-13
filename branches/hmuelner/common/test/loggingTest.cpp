#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

#include <cstring>
#include <unistd.h>

#include "logging.h"

#if HAVE_LIBLOG4CXX
const int logBufferSize = 65536;
char logBuffer[logBufferSize];
#endif // HAVE_LIBLOG4CXX

class PrintBufferTest : public CppUnit::TestFixture {
public:
  CPPUNIT_TEST_SUITE( PrintBufferTest );
  CPPUNIT_TEST( testPrinting );
  CPPUNIT_TEST( testOverflow );
  CPPUNIT_TEST_SUITE_END();

  void testPrinting() {
    PrintfBuffer pb;
    pbprintf(pb, "abc");
    pbprintf(pb, " %s", "def");
    pbprintf(pb, " %d", 42);
    CPPUNIT_ASSERT( strcmp("abc def 42", pb.getContents()) == 0 );
  }

  void testOverflow() {
    PrintfBuffer pb(5, 3);
    
    pbprintf(pb, "aaa");
    CPPUNIT_ASSERT( strcmp("aaa", pb.getContents()) == 0 );
    
    pbprintf(pb, "bbb");
    CPPUNIT_ASSERT( strcmp("aaabbb", pb.getContents()) == 0 );
    
    pbprintf(pb, "ccc");
    CPPUNIT_ASSERT( strcmp("aaabbbccc", pb.getContents()) == 0 );

    pbprintf(pb, "ddd");
    CPPUNIT_ASSERT( strcmp("aaabbbcccddd", pb.getContents()) == 0 );

    pbprintf(pb, "eee");
    CPPUNIT_ASSERT( strcmp("aaabbbcccdddeee", pb.getContents()) == 0 );

    pbprintf(pb, "fff");
    CPPUNIT_ASSERT( strcmp("aaabbbcccdddeeefff", pb.getContents()) == 0 );
    
    pbprintf(pb, "ggg");
    CPPUNIT_ASSERT( strcmp("aaabbbcccdddeeefffggg", pb.getContents()) == 0 );

    pbprintf(pb, "hhh");
    CPPUNIT_ASSERT( strcmp("aaabbbcccdddeeefffggghhh", pb.getContents()) == 0 );
  }
};

class StreamPrinterTest : public CppUnit::TestFixture {
public:
  CPPUNIT_TEST_SUITE( StreamPrinterTest );
  CPPUNIT_TEST( testBasic );
  CPPUNIT_TEST_SUITE_END();

  void testBasic() {
    //we don't use any operating system's support for temporary files
    const char *fileName = "test_StreamPrinter";

    // write to the file through StreamPrinter
    FILE *file = fopen(fileName, "w");
    CPPUNIT_ASSERT( file != NULL );
    StreamPrinter sp(file);
    pbprintf(sp, "abc");
    pbprintf(sp, " %s", "def");
    pbprintf(sp, " %d", 42);
    fclose(file);
    
    //read the file
    file = fopen(fileName, "r");
    CPPUNIT_ASSERT( file != NULL );
    char buf[100];
    int nRead;
    nRead = fread(buf, 1, 100, file);
    buf[nRead] = '\0';
    fclose(file);
    unlink(fileName);

    //compare contents of the file and what should be written
    CPPUNIT_ASSERT( strcmp("abc def 42", buf) == 0 );
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION( PrintBufferTest );
CPPUNIT_TEST_SUITE_REGISTRATION( StreamPrinterTest );

int main(int argc, char *argv[]) {
#if HAVE_LIBLOG4CXX
  log4cxx::BasicConfigurator::configure();
  log4cxx::Logger::getRootLogger()->setLevel(log4cxx::Level::OFF);
#endif // HAVE_LIBLOG4CXX
  CppUnit::TextUi::TestRunner runner;
  CppUnit::TestFactoryRegistry &registry =
    CppUnit::TestFactoryRegistry::getRegistry();
  runner.addTest( registry.makeTest() );
  bool wasSuccessful = runner.run("", false);
  return wasSuccessful ? 0 : 1;  
}
