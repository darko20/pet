#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

#include <cstring>

#include "logging.h"

class PrintBufferTest : public CppUnit::TestFixture {
public:
  CPPUNIT_TEST_SUITE( PrintBufferTest );
  CPPUNIT_TEST( testConstruction );
  CPPUNIT_TEST( testPrinting );
  CPPUNIT_TEST( testOverflow );
  CPPUNIT_TEST_SUITE_END();

  void testConstruction() {
    char buf[100];
    PrintfBuffer pb(buf, 100);
    CPPUNIT_ASSERT( !pb.isOverflowed() );
  }

  void testPrinting() {
    char buf[100];
    PrintfBuffer pb(buf, 100);
    pbprintf(&pb, "abc");
    pbprintf(&pb, " %s", "def");
    pbprintf(&pb, " %d", 42);
    CPPUNIT_ASSERT( !pb.isOverflowed() );
    CPPUNIT_ASSERT( strcmp("abc def 42", pb.getContents()) == 0 );
  }
  
  void testOverflow() {
    char buf1[6];
    buf1[5] = 'y';
    PrintfBuffer pb1(buf1, 5);

    //does not overflows the buffer
    pbprintf(&pb1, "1234");
    CPPUNIT_ASSERT( !pb1.isOverflowed() );
    CPPUNIT_ASSERT_EQUAL( '\0', buf1[4] );
    
    //this overflows
    pbprintf(&pb1, "x");
    CPPUNIT_ASSERT( pb1.isOverflowed() );
    CPPUNIT_ASSERT_EQUAL( '\0', buf1[4] );
    CPPUNIT_ASSERT_EQUAL( 'y',  buf1[5] );

    //try again to overflow the buffer
    pbprintf(&pb1, "xx");
    CPPUNIT_ASSERT( pb1.isOverflowed() );
    CPPUNIT_ASSERT_EQUAL( '\0', buf1[4] );
    CPPUNIT_ASSERT_EQUAL( 'y',  buf1[5] );

    char buf2[6];
    buf2[4] = '\0';
    buf2[5] = 'y';

    PrintfBuffer pb2(buf2, 5);
    pbprintf(&pb2, "12345");
    CPPUNIT_ASSERT( pb2.isOverflowed() );
    CPPUNIT_ASSERT_EQUAL( '\0', buf2[4] );
    CPPUNIT_ASSERT_EQUAL( 'y',  buf2[5] );
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION( PrintBufferTest );

int main(int argc, char *argv[]) {
  CppUnit::TextUi::TestRunner runner;
  CppUnit::TestFactoryRegistry &registry =
    CppUnit::TestFactoryRegistry::getRegistry();
  runner.addTest( registry.makeTest() );
  bool wasSuccessful = runner.run("", false);
  return wasSuccessful ? 0 : 1;  
}
