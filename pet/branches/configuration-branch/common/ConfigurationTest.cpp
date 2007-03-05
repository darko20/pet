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
    Converter<int> *ci1, *ci2;
    Converter<float> *cf;
    
    ci1 = Converter<int>::getInstance();
    ci2 = Converter<int>::getInstance();
    cf  = Converter<float>::getInstance();

    CPPUNIT_ASSERT( ci1 != 0 );
    CPPUNIT_ASSERT( ci1 == ci2 );
    CPPUNIT_ASSERT( cf != 0 );
    CPPUNIT_ASSERT( (void*)cf != (void*)ci1 );
  }

  void testConverterInt() {
    Converter<int> *ci;
    ci = Converter<int>::getInstance();
    
    CPPUNIT_ASSERT_EQUAL( 42, ci->fromString("42") );
    CPPUNIT_ASSERT_EQUAL( string("45"), ci->toString(45) );
  }

  void testConverterCustom() {
    Converter<CustomType> *cct = Converter<CustomType>::getInstance();
    
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
    
    CPPUNIT_ASSERT_THROW( mc.fromString("6"), ConvertionException );
    CPPUNIT_ASSERT_THROW( mc.toString(5), ConvertionException );
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
  CPPUNIT_TEST( testConverterExceptions );
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
    CPPUNIT_ASSERT_EQUAL(false, Configuration::hasOption("statusmember"));
    StatusMap statusmembers;
    Configuration::addOption("statusmember", &statusmembers);
    CPPUNIT_ASSERT_EQUAL(true, Configuration::hasOption("statusmember"));
  }

  void testConfigurationSimpleOwn() {
    int int0 = 0;
    Configuration::addOption("int1", &int0);
    int0 = 42;
    Configuration::set<int>("int1", 42);
    int& int1 = Configuration::get<int>("int1");
    CPPUNIT_ASSERT_EQUAL( 42, int1 );
    CPPUNIT_ASSERT( &int1 == &int0 );
    int1 = 43;
    CPPUNIT_ASSERT_EQUAL( 43, int0 );

    float float0 = 15.5f;
    Configuration::addOption("MyFloat", &float0);
    Configuration::set("MyFloat", 15.5f);
    float& myFloat = Configuration::get<float>("MyFloat");
    //comparing floats is generally not a good idea but here it should work
    CPPUNIT_ASSERT_EQUAL( 15.5f, myFloat );
    CPPUNIT_ASSERT( &float0 == &myFloat );

    myFloat = 16.7f;
    // Use the alternate form of get which does not need template parameters
    Configuration::get("MyFloat", myFloat);
    //comparing floats is generally not a good idea but here it should work
    CPPUNIT_ASSERT_EQUAL( 16.7f, myFloat );
    CPPUNIT_ASSERT( &float0 == &myFloat );

    // Be aware that this code may entail the copy of a float. If this was a
    // complex object, it would be a copy in any case.
    float myFloat2;
    Configuration::get("MyFloat", myFloat2);
    //comparing floats is generally not a good idea but here it should work
    CPPUNIT_ASSERT_EQUAL( 16.7f, myFloat2 );
    CPPUNIT_ASSERT( &float0 != &myFloat2 );
    myFloat2 = 0.0;

    Configuration::get("MyFloat", myFloat);
    CPPUNIT_ASSERT_EQUAL( 16.7f, myFloat );
  }

  void testConfigurationSimpleInt() {
    Configuration::addOption<int>("int2");
    Configuration::set("int2", 42);
    int int1 = Configuration::get<int>("int2");
    CPPUNIT_ASSERT_EQUAL( 42, int1 );
  }

  void testConfigurationSimpleFloat() {
    Configuration::addOption<float>("MyFloat2");
    Configuration::set("MyFloat2", 15.5f);
    float myFloat = Configuration::get<float>("MyFloat2");
    //comparing floats is generally not a good idea but here it should work
    CPPUNIT_ASSERT_EQUAL( 15.5f, myFloat );
  }

  void testConfigurationComplexOwn() {
    StatusMap statusmembers;
    Configuration::addOption("statusmember1", &statusmembers);
    StatusMap &sm 
      = Configuration::get< StatusMap >("statusmember1");
    CPPUNIT_ASSERT(&sm == &statusmembers);
  }

  void testConfigurationComplex() {
    Configuration::addOption< StatusMap >("statusmember2");
    StatusMap &sm = Configuration::get<StatusMap>("statusmember2");
    sm.add("status1", "type1");
    StatusMap &sm2 = Configuration::get<StatusMap>("statusmember2");
    CPPUNIT_ASSERT(&sm == &sm2);
    CPPUNIT_ASSERT(sm2.statusmember("status1", "type1"));
  }

  void testEmptyName() {
    CPPUNIT_ASSERT_THROW( Configuration::addOption<int>(""),
                          WrongEntryNameException );
    CPPUNIT_ASSERT_THROW( Configuration::get<int>(""),
                          WrongEntryNameException );
  }

  void testNonExistingEntry() {
    CPPUNIT_ASSERT_THROW( Configuration::get<int>("hello"),
                          NoSuchEntryException );
  }

  void testWrongType() {
    Configuration::addOption<int>("int3");
    CPPUNIT_ASSERT_THROW( Configuration::get<double>("int3"),
                          WrongTypeException );
  }

  void testAlreadyExists() {
    Configuration::addOption<int>("entry1");
    CPPUNIT_ASSERT_THROW( Configuration::addOption<int>("entry1"),
                          EntryAlreadyExistsException );

    int entry2;
    Configuration::addOption<int>("entry2");
    CPPUNIT_ASSERT_THROW( Configuration::addOption<int>("entry2", &entry2),
                          EntryAlreadyExistsException );
  }

  void testCallback() {
    MyCallbackTester cb;

    Configuration::addCallback(std::string("cb1"), &cb);
    CPPUNIT_ASSERT_EQUAL( 0, cb.callsCounter );
    
    Configuration::set("cb1", 47);
    CPPUNIT_ASSERT_EQUAL( 1, cb.callsCounter );
    CPPUNIT_ASSERT_EQUAL( 47, cb.lastValue );
    
    Configuration::set("cb1", 49);
    CPPUNIT_ASSERT_EQUAL( 49, cb.lastValue );
  }
    
  void testPriority() {
    Configuration::addOption<int>("prio1");
    Configuration::set("prio1", 58);
    CPPUNIT_ASSERT_EQUAL( 58, Configuration::get<int>("prio1"));
        
    Configuration::set("prio1", 59, 10);
    CPPUNIT_ASSERT_EQUAL( 59, Configuration::get<int>("prio1"));
        
    Configuration::set("prio1", 44, 9);
    CPPUNIT_ASSERT_EQUAL( 59, Configuration::get<int>("prio1"));
        
    Configuration::set("prio1", 44, 11);
    CPPUNIT_ASSERT_EQUAL( 44, Configuration::get<int>("prio1"));
  }

  // test if the descriptions of options are handled correctly
  void testDescription() {
    Configuration::addOption<int>("opt_desc_H", "desc1");
    CPPUNIT_ASSERT_EQUAL(std::string("desc1"),
                         Configuration::getDescription("opt_desc_H"));

    int i;
    Configuration::addOption<int>("opt_desc_R", &i, "desc2");
    CPPUNIT_ASSERT_EQUAL(std::string("desc2"),
                         Configuration::getDescription("opt_desc_R"));
    
    MyCallbackTester cb;
    Configuration::addCallback("opt_desc_C", &cb, "desc3");
    CPPUNIT_ASSERT_EQUAL(std::string("desc2"),
                         Configuration::getDescription("opt_desc_C"));
  }

  // test if the initial values of options are applied correctly
  void testInitial() {
    // --------------- Handled ---------------
    Configuration::addOption<int>("initH1", "description", 3);
    CPPUNIT_ASSERT_EQUAL( 3, Configuration::get<int>("initH1") );

    // default constructors of primitive types
    // initialize variables to 0, 0.0, false, etc.
    Configuration::addOption<int>("initH2");
    CPPUNIT_ASSERT_EQUAL( 0, Configuration::get<int>("initH2") );
    
    Configuration::addOption<WithDefConstructor>("initH3");
    CPPUNIT_ASSERT_EQUAL( string("abc"),
                          Configuration::get<WithDefConstructor>("initH3").s_ );
    Configuration::addOption<WithDefConstructor>("initH4", "descr",
                                                 WithDefConstructor("xyz"));
    CPPUNIT_ASSERT_EQUAL( string("xyz"),
                          Configuration::get<WithDefConstructor>("initH4").s_ );

    // --------------- Reference ---------------
    int i;
    Configuration::addOption<int>("initR1", &i, "description", 5);
    CPPUNIT_ASSERT_EQUAL( 5, i );
    CPPUNIT_ASSERT_EQUAL( 5, Configuration::get<int>("initR1") );
    
    double d;
    Configuration::addOption<double>("initR2", &d);
    CPPUNIT_ASSERT_EQUAL( 0.0, d );
    CPPUNIT_ASSERT_EQUAL( 0.0, Configuration::get<double>("initR2") );

    WithDefConstructor wdc1, wdc2;
    Configuration::addOption<WithDefConstructor>("initR3", &wdc1, "descr",
                                                 WithDefConstructor("xyz"));
    CPPUNIT_ASSERT_EQUAL( string("xyz"), wdc1.s_ );
    Configuration::addOption<WithDefConstructor>("initR4", &wdc2);
    CPPUNIT_ASSERT_EQUAL( string("abc"), wdc2.s_ );
  }

  void testConverter() {
    // --------------- Handled -----------------
    Configuration::addOption<int>("conv_1", "descr", 3,
                                  Converter<int>::getInstance());
    CPPUNIT_ASSERT_EQUAL( string("3"),
                          Configuration::getString<int>("conv_1") );
    Configuration::setString<int>("conv_1", "47");
    CPPUNIT_ASSERT_EQUAL( 47,  Configuration::get<int>("conv_1") );
    
    // --------------- Reference ---------------
    int i;
    Configuration::addOption<int>("conv_2", &i, "descr", 3,
                                  Converter<int>::getInstance());
    CPPUNIT_ASSERT_EQUAL( string("3"),
                          Configuration::getString<int>("conv_2") );
    Configuration::setString<int>("conv_2", "47");
    CPPUNIT_ASSERT_EQUAL( 47,  i );
  }

  void testConverterExceptions() {
    Configuration::addOption<int>("convEx_1", "descr", 3);
    CPPUNIT_ASSERT_THROW( Configuration::getString<int>("convEx_1"),
                          NoConverterException );
  }

  void testMapConverter() {
    MapConverter<int>::Map m;
    m["one"]   = 1;
    m["two"]   = 2;
    m["three"] = 3;
    MapConverter<int> mc(m);
    
    // --------------- Handled -----------------
    Configuration::addOption<int>("mconv_1", "descr", 3, &mc);
    CPPUNIT_ASSERT_EQUAL( string("three"),
                          Configuration::getString<int>("mconv_1") );
    Configuration::setString<int>("mconv_1", "two");
    CPPUNIT_ASSERT_EQUAL( 2,  Configuration::get<int>("mconv_1") );
    
    Configuration::set("mconv_1", 42);
    CPPUNIT_ASSERT_THROW( Configuration::getString<int>("mconv_1"),
                          ConvertionException );

    // --------------- Reference ---------------
    int i;
    Configuration::addOption<int>("mconv_2", &i, "descr", 1, &mc);
    CPPUNIT_ASSERT_EQUAL( string("one"),
                          Configuration::getString<int>("mconv_2") );
    Configuration::setString<int>("mconv_2", "three");
    CPPUNIT_ASSERT_EQUAL( 3,  i );
    i = 42;
    CPPUNIT_ASSERT_THROW( Configuration::getString<int>("mconv_2"),
                          ConvertionException );
  }

  void testOnStrings() {
    Configuration::addOption<string>("onString_1", "", string(""));
    CPPUNIT_ASSERT_EQUAL( string(""),
                          Configuration::get<string>("onString_1") );
    Configuration::set<string>("onString_1", "ab");
    CPPUNIT_ASSERT_EQUAL( string("ab"),
                          Configuration::get<string>("onString_1") );
    Configuration::get<string>("onString_1") += "cde";
    CPPUNIT_ASSERT_EQUAL( string("abcde"),
    Configuration::get<string>("onString_1") );
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
