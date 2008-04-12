#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

#include <string>
#include <iostream>

#include "Configuration.h"

#if HAVE_LIBLOG4CXX
#  include <log4cxx/basicconfigurator.h>
const int logBufferSize = 65536;
char logBuffer[65536];
#endif // HAVE_LIBLOG4CXX

using namespace std;

//for testing callbacks
class MyCallbackTester : public Callback<int> {
public:
  int callsCounter;
  int lastValue;
  
  MyCallbackTester() : callsCounter(0) {};

  virtual ~MyCallbackTester(){}

  virtual void set(const int& i) {
    lastValue = i;
    callsCounter++;
  }

  virtual int& get() {
    return lastValue;
  }
};


struct MyCallbackGetter : public std::unary_function<void, int> {
  MyCallbackGetter(MyCallbackTester &m) : _m(m) {} 
  int operator()() {
    return _m.get();
  }
  MyCallbackTester& _m;
};

class StatusMap {
private:
  typedef std::set<std::string> StringSet;
  typedef std::map<std::string, StringSet > StatMap;
  StatMap statusMap_;

public:
  void add(const std::string &status, const std::string &typeName) {
    StatMap::iterator it = statusMap_.find(status);
    if (it == statusMap_.end()) {
      StringSet newSet;
      newSet.insert(typeName);
      statusMap_.insert(std::pair<std::string, StringSet>(status, newSet));
    } else {
      StringSet &oldSet = it->second;
      oldSet.insert(typeName);
    }
  }

  bool statusmember(const std::string &status, const std::string &typeName) {
    StatMap::iterator it = statusMap_.find(status);
    if (it == statusMap_.end()) 
      return false;
    StringSet::iterator it2 = (it->second).find(typeName);
    if (it2 == (it->second).end())
      return false;

    return true;
  }
};

// for testing initial values of options
class WithDefConstructor {
public:
  WithDefConstructor(string s = "abc") : s_(s) {};
  string s_;
};


//tests of Converters
class ConvertersTest : public CppUnit::TestFixture {
private:
  class CustomType {
  public:
    int x_, y_;
    CustomType(int x = 0, int y = 0) : x_(x), y_(y) {}
    
    friend ostream& operator<<(ostream& os, const CustomType& ct) {
      os << ct.x_ << ' ' << ct.y_;
      return os;
    }
    
    friend istream& operator>>(istream& is, CustomType& ct) {
      is >> ct.x_ >> ct.y_;
      return is;
    }
    
    bool operator==(const CustomType& ct) const {
      return (this->x_ == ct.x_) && (this->y_ == ct.y_);
    }
  };

public:

  CPPUNIT_TEST_SUITE( ConvertersTest );
  CPPUNIT_TEST( testConverterGetInstance );
  CPPUNIT_TEST( testConverterInt );
  CPPUNIT_TEST( testConverterCustom );
  CPPUNIT_TEST( testMapConverter );
  CPPUNIT_TEST_SUITE_END();

  void testConverterGetInstance() {
    AbstractConverter<int> *ci1, *ci2;
    AbstractConverter<float> *cf;
    
    ci1 = GenericConverter<int>::getInstance();
    ci2 = GenericConverter<int>::getInstance();
    cf  = GenericConverter<float>::getInstance();

    CPPUNIT_ASSERT( ci1 != 0 );
    CPPUNIT_ASSERT( ci1 == ci2 );
    CPPUNIT_ASSERT( cf != 0 );
    CPPUNIT_ASSERT( (void*)cf != (void*)ci1 );
  }

  void testConverterInt() {
    AbstractConverter<int> *ci;
    ci = GenericConverter<int>::getInstance();
    
    CPPUNIT_ASSERT_EQUAL( 42, ci->fromString("42") );
    CPPUNIT_ASSERT_EQUAL( string("45"), ci->toString(45) );
  }

  void testConverterCustom() {
    AbstractConverter<CustomType> *cct 
      = GenericConverter<CustomType>::getInstance();
    
    CPPUNIT_ASSERT_EQUAL( CustomType(10,5), cct->fromString("10 5") );
    CPPUNIT_ASSERT_EQUAL( string("7 19"), cct->toString(CustomType(7, 19)) );
  }

  void testMapConverter() {
    MapConverter<int>::Map m;
    m["one"]   = 1;
    m["two"]   = 2;
    m["three"] = 3;
    MapConverter<int> mc(m);
    
    CPPUNIT_ASSERT_EQUAL( 2, mc.fromString("two") );
    CPPUNIT_ASSERT_EQUAL( string("one"), mc.toString(1) );
    
    //CPPUNIT_ASSERT_THROW( mc.fromString("6"), ConvertionException );
    //CPPUNIT_ASSERT_THROW( mc.toString(5), ConvertionException );
  }
};


class ConfigurationTest : public CppUnit::TestFixture {
public:
  CPPUNIT_TEST_SUITE( ConfigurationTest );

  //CPPUNIT_TEST( testGetInstance );
  CPPUNIT_TEST( testConfigurationPresence );

  CPPUNIT_TEST( testConfigurationSimpleOwn );
  CPPUNIT_TEST( testConfigurationSimpleInt );
  CPPUNIT_TEST( testConfigurationSimpleFloat );
  CPPUNIT_TEST( testConfigurationSimpleCharPointer );

  CPPUNIT_TEST( testConfigurationComplexOwn );
  CPPUNIT_TEST( testConfigurationComplex );

  CPPUNIT_TEST( testEmptyName );
  CPPUNIT_TEST( testNonExistingEntry );
  CPPUNIT_TEST( testWrongType );
  CPPUNIT_TEST( testAlreadyExists );

  CPPUNIT_TEST( testCallback );
    
  CPPUNIT_TEST( testPriority );
  
  CPPUNIT_TEST( testDescription );
  CPPUNIT_TEST( testInitial );
  CPPUNIT_TEST( testConverter );
  //CPPUNIT_TEST( testConverterExceptions );
  CPPUNIT_TEST( testMapConverter );
  CPPUNIT_TEST (testOnStrings );
  
  CPPUNIT_TEST_SUITE_END();

  /*
  //checks if call to getInstance return the same object
  void testGetInstance() {
    Configuration& c1 = Configuration::getInstance();
    Configuration& c2 = Configuration::getInstance();
    CPPUNIT_ASSERT(&c1 == &c2);
    CPPUNIT_ASSERT(&c1 != 0);
  }
  */
  void testConfigurationPresence() {
    CPPUNIT_ASSERT_EQUAL(false, Config::hasOption("statusmember"));
    /*
    StatusMap statusmembers;
    reference_opt("statusmember", "special data structure test"
                  , statusmembers
                  , (AbstractConverter<StatusMap>*) NULL);
    */
    managed_opt("statusmember", "docstring", (bool) false);
    CPPUNIT_ASSERT_EQUAL(true, Config::hasOption("statusmember"));
  }

  void testConfigurationManaged() {
    managed_opt("int0", "Simple int test", (int) 0);

    int int0 = 0;
    reference_opt("int1", "", int0);
    int0 = 42;
    set_opt<int>("int1", 42);
    int& int1 = Config::get<int>("int1");
    CPPUNIT_ASSERT_EQUAL( 42, int1 );
    CPPUNIT_ASSERT( &int1 == &int0 );
    int1 = 43;
    CPPUNIT_ASSERT_EQUAL( 43, int0 );

    float float0 = 15.5f;
    reference_opt("MyFloat", "", float0);
    set_opt("MyFloat", 15.5f);
    float& myFloat = Config::get<float>("MyFloat");
    //comparing floats is generally not a good idea but here it should work
    CPPUNIT_ASSERT_EQUAL( 15.5f, myFloat );
    CPPUNIT_ASSERT( &float0 == &myFloat );

    myFloat = 16.7f;
    // Use the alternate form of get which does not need template parameters
    Config::get("MyFloat", myFloat);
    //comparing floats is generally not a good idea but here it should work
    CPPUNIT_ASSERT_EQUAL( 16.7f, myFloat );
    CPPUNIT_ASSERT( &float0 == &myFloat );

    // Be aware that this code may entail the copy of a float. If this was a
    // complex object, it would be a copy in any case.
    float myFloat2;
    Config::get("MyFloat", myFloat2);
    //comparing floats is generally not a good idea but here it should work
    CPPUNIT_ASSERT_EQUAL( 16.7f, myFloat2 );
    CPPUNIT_ASSERT( &float0 != &myFloat2 );
    myFloat2 = 0.0;

    Config::get("MyFloat", myFloat);
    CPPUNIT_ASSERT_EQUAL( 16.7f, myFloat );
  }

  void testConfigurationSimpleInt() {
    managed_opt("int2", "Simple test", (int) 0);
    set_opt("int2", 42);
    int int1 = Config::get<int>("int2");
    CPPUNIT_ASSERT_EQUAL( 42, int1 );
    Config::get("int2", int1);
    CPPUNIT_ASSERT_EQUAL( 42, int1 );
  }

  void testConfigurationSimpleFloat() {
    managed_opt<float>("MyFloat2", "float test", (double) 0.0);
    set_opt("MyFloat2", 15.5f);
    float myFloat = Config::get<float>("MyFloat2");
    //comparing floats is generally not a good idea but here it should work
    CPPUNIT_ASSERT_EQUAL( 15.5f, myFloat );
    Config::get("MyFloat2", myFloat);
    //comparing floats is generally not a good idea but here it should work
    CPPUNIT_ASSERT_EQUAL( 15.5f, myFloat );
  }

  void testConfigurationSimpleCharPointer() {
    managed_opt("MyCharP1", "float test", (char *) "initial");
    char *myString = Config::get<char *>("MyCharP1");
    //comparing floats is generally not a good idea but here it should work
    CPPUNIT_ASSERT( strcmp(myString,"initial") == 0 );
    Config::get("MyCharP1", myString);
    CPPUNIT_ASSERT( strcmp(myString,"initial") == 0 );
    { char foo[5];
      foo[0]='c'; foo[1] = 'h';foo[2] = 'a';foo[3] = 'r';foo[4] = '\0';
      set_opt<char *>("MyCharP1", foo);
      foo[0] = 'd';
    }
    CPPUNIT_ASSERT( strcmp(Config::get<char *>("MyCharP1") ,"char") == 0 );
    set_opt<char *>("MyCharP1", "other");
    CPPUNIT_ASSERT( strcmp(Config::get<char *>("MyCharP1") ,"other") == 0 );
  }

  void testConfigurationComplexOwn() {
    StatusMap statusmembers;
    reference_opt("statusmember1", "special type test", statusmembers,
                         (AbstractConverter<StatusMap>*) NULL);

    // This is true, but should not be exploited
    StatusMap &sm = Config::get< StatusMap >("statusmember1");
    CPPUNIT_ASSERT(&sm == &statusmembers);

    /*
    StatusMap* sm2 = Config::getPointer<StatusMap>("statusmember1");
    CPPUNIT_ASSERT(sm2 == &statusmembers);

    StatusMap* sm3 = NULL;

    Config::getPointer<StatusMap>("statusmember1", sm3);
    fprintf(stderr, "%x %x", sm3, &statusmembers);
    CPPUNIT_ASSERT(sm3 == &statusmembers);
    */
  }

  void testConfigurationComplex() {
    managed_opt("statusmember2", "special type handled test",
                      StatusMap(), (AbstractConverter<StatusMap>*) NULL);
    StatusMap &sm = Config::get<StatusMap>("statusmember2");
    sm.add("status1", "type1");
    StatusMap &sm2 = Config::get<StatusMap>("statusmember2");
    CPPUNIT_ASSERT(&sm == &sm2);
    CPPUNIT_ASSERT(sm2.statusmember("status1", "type1"));
  }

  void testEmptyName() {
    CPPUNIT_ASSERT_THROW( managed_opt<int>("", "desc", 0),
                          ConfigException );
    CPPUNIT_ASSERT_THROW( Config::get<int>(""),
                          ConfigException );
  }

  void testNonExistingEntry() {
    CPPUNIT_ASSERT_THROW( Config::get<int>("hello"),
                          ConfigException );
  }

  void testWrongType() {
    managed_opt<int>("int3", "exception test", 0);
    CPPUNIT_ASSERT_THROW( Config::get<double>("int3"),
                          ConfigException );
  }

  void testAlreadyExists() {
    managed_opt<int>("entry1", "exception test", 0);
    CPPUNIT_ASSERT_THROW( managed_opt<int>("entry1", "desc", 1),
                          ConfigException );

    int entry2;
    managed_opt<int>("entry2", "exception test", 0);
    CPPUNIT_ASSERT_THROW( reference_opt<int>("entry2", "exception test",
                                                    entry2),
                          ConfigException );
  }

  void testCallback() {
    MyCallbackTester cb;

    Config::addCallback(std::string("cb1"), "callback test", &cb);
    CPPUNIT_ASSERT_EQUAL( 0, cb.callsCounter );
    
    set_opt("cb1", 47);
    CPPUNIT_ASSERT_EQUAL( 1, cb.callsCounter );
    CPPUNIT_ASSERT_EQUAL( 47, cb.lastValue );
    
    set_opt("cb1", 49);
    CPPUNIT_ASSERT_EQUAL( 49, cb.lastValue );
  }
    
  void testPriority() {
    managed_opt<int>("prio1", "priority test", 0);
    set_opt("prio1", 58);
    CPPUNIT_ASSERT_EQUAL( 58, Config::get<int>("prio1"));
        
    set_opt("prio1", 59, 10);
    CPPUNIT_ASSERT_EQUAL( 59, Config::get<int>("prio1"));
        
    set_opt("prio1", 44, 9);
    CPPUNIT_ASSERT_EQUAL( 59, Config::get<int>("prio1"));
        
    set_opt("prio1", 44, 11);
    CPPUNIT_ASSERT_EQUAL( 44, Config::get<int>("prio1"));
  }

  // test if the descriptions of options are handled correctly
  void testDescription() {
    managed_opt<int>("opt_desc_H", "desc1", 0);
    CPPUNIT_ASSERT_EQUAL(std::string("desc1"),
                         Config::getDescription("opt_desc_H"));

    int i;
    reference_opt<int>("opt_desc_R", "desc2", i);
    CPPUNIT_ASSERT_EQUAL(std::string("desc2"),
                         Config::getDescription("opt_desc_R"));
    
    MyCallbackTester cb;
    Config::addCallback("opt_desc_C", "desc3", &cb);
    CPPUNIT_ASSERT_EQUAL(std::string("desc3"),
                         Config::getDescription("opt_desc_C"));
  }

  // test if the initial values of options are applied correctly
  void testInitial() {
    // --------------- Handled ---------------
    managed_opt<int>("initH1", "description", 3);
    CPPUNIT_ASSERT_EQUAL( 3, Config::get<int>("initH1") );

    // default constructors of primitive types
    // initialize variables to 0, 0.0, false, etc.
    managed_opt<int>("initH2", "desc", 0);
    CPPUNIT_ASSERT_EQUAL( 0, Config::get<int>("initH2") );
    
    WithDefConstructor foo;
    managed_opt("initH3", "desc", foo,
                      (AbstractConverter<WithDefConstructor>*) NULL);
    CPPUNIT_ASSERT_EQUAL( string("abc"),
                          Config::get<WithDefConstructor>("initH3").s_ );
    managed_opt("initH4", "descr", WithDefConstructor("xyz"),
                      (AbstractConverter<WithDefConstructor>*)NULL);
    CPPUNIT_ASSERT_EQUAL( string("xyz"),
                          Config::get<WithDefConstructor>("initH4").s_ );

    // --------------- Reference ---------------
    int i = 5;
    reference_opt<int>("initR1", "description", i);
    CPPUNIT_ASSERT_EQUAL( 5, i );
    CPPUNIT_ASSERT_EQUAL( 5, Config::get<int>("initR1") );
    
    double d = 0.0;
    reference_opt<double>("initR2", "doubletest", d);
    CPPUNIT_ASSERT_EQUAL( 0.0, d );
    CPPUNIT_ASSERT_EQUAL( 0.0, Config::get<double>("initR2") );

    WithDefConstructor wdc1("xyz"), wdc2;
    reference_opt<WithDefConstructor>("initR3", "descr", wdc1, NULL);
    CPPUNIT_ASSERT_EQUAL( string("xyz"), wdc1.s_ );
    reference_opt<WithDefConstructor>("initR4", "default constructor",
                                             wdc2, NULL);
    CPPUNIT_ASSERT_EQUAL( string("abc"), wdc2.s_ );
  }

  void testConverter() {
    // --------------- Handled -----------------
    managed_opt<int>("conv_1", "descr", 3);
    CPPUNIT_ASSERT_EQUAL( string("3"),
                          Config::getString("conv_1") );
    set_optString("conv_1", "47");
    CPPUNIT_ASSERT_EQUAL( 47,  Config::get<int>("conv_1") );
    
    // --------------- Reference ---------------
    int i = 3;
    reference_opt<int>("conv_2", "descr", i);
    CPPUNIT_ASSERT_EQUAL( string("3"), Config::getString("conv_2") );
    set_optString("conv_2", "47");
    CPPUNIT_ASSERT_EQUAL( 47,  i );
  }

  void testConverterExceptions() {
    managed_opt<int>("convEx_1", "descr", 3);
    CPPUNIT_ASSERT_THROW( Config::getString("convEx_1"),
                          NoConverterException );
  }

  void testMapConverter() {
    MapConverter<int>::Map m;
    m["one"]   = 1;
    m["two"]   = 2;
    m["three"] = 3;
    MapConverter<int> mc(m);
    
    // --------------- Handled -----------------
    managed_opt<int>("mconv_1", "descr", 3, &mc);
    CPPUNIT_ASSERT_EQUAL( string("three"),
                          Config::getString("mconv_1") );
    set_optString("mconv_1", "two");
    CPPUNIT_ASSERT_EQUAL( 2,  Config::get<int>("mconv_1") );
    
    set_opt("mconv_1", 42);
    CPPUNIT_ASSERT_THROW( Config::getString("mconv_1"),
                          ConfigException );

    // --------------- Reference ---------------
    int i = 1;
    reference_opt<int>("mconv_2", "descr", i, &mc);
    CPPUNIT_ASSERT_EQUAL( string("one"), Config::getString("mconv_2") );
    set_optString("mconv_2", "three");
    CPPUNIT_ASSERT_EQUAL( 3,  i );
    i = 42;
    CPPUNIT_ASSERT_THROW( Config::getString("mconv_2"), ConfigException );
  }

  void testOnStrings() {
    managed_opt<string>("onString_1", "", string(""));
    CPPUNIT_ASSERT_EQUAL( string(""),
                          Config::get<string>("onString_1") );
    set_opt("onString_1", (string) "ab");
    CPPUNIT_ASSERT_EQUAL( string("ab"),
                          Config::get<string>("onString_1") );
    Config::get<string>("onString_1") += "cde";
    CPPUNIT_ASSERT_EQUAL( string("abcde"),
    Config::get<string>("onString_1") );
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION( ConvertersTest );
CPPUNIT_TEST_SUITE_REGISTRATION( ConfigurationTest );

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
