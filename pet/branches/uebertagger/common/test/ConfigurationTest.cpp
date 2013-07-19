#if 1
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

#include <iostream>

#include "configs.h"

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

/*
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
*/

class ConfigurationTest : public CppUnit::TestFixture {
public:
  CPPUNIT_TEST_SUITE( ConfigurationTest );

  //CPPUNIT_TEST( testGetInstance );
  CPPUNIT_TEST( testConfigurationPresenceDescription );

  CPPUNIT_TEST( testConfigurationManaged );
  CPPUNIT_TEST( testConfigurationReference );
  CPPUNIT_TEST( testConfigurationCharPointer );
  CPPUNIT_TEST( testConfigurationWrongType );

  CPPUNIT_TEST( testAlreadyExists );
  CPPUNIT_TEST( testCallback );
  CPPUNIT_TEST( testPriority );
  
  /*
  CPPUNIT_TEST( testConfigurationComplexOwn );
  CPPUNIT_TEST( testConfigurationComplex );

  CPPUNIT_TEST( testEmptyName );
  CPPUNIT_TEST( testNonExistingEntry );
  CPPUNIT_TEST( testWrongType );

  
  CPPUNIT_TEST( testDescription );
  CPPUNIT_TEST( testInitial );
  CPPUNIT_TEST( testConverter );
  CPPUNIT_TEST( testConverterExceptions );
  CPPUNIT_TEST( testMapConverter );
  CPPUNIT_TEST (testOnStrings );
  */
  CPPUNIT_TEST_SUITE_END();

  // test basic option presence and description functionality

  void testConfigurationPresenceDescription() {
    CPPUNIT_ASSERT(! option_valid("statusmembers"));
    int statusmembers;
    reference_opt("statusmembers", "The number of status members",
                  statusmembers);
    CPPUNIT_ASSERT( option_valid("statusmembers"));
    CPPUNIT_ASSERT( option_description("statusmembers") 
                    == (string)"The number of status members");
  }

  // test functionality for a managed option
  void testConfigurationManaged() {
    CPPUNIT_ASSERT(! option_valid("int0") );
    managed_opt("int0", "managed int option", (int) 1);
    CPPUNIT_ASSERT_EQUAL( 1, get_opt_int("int0") );
    set_opt("int0", (int) 42);
    CPPUNIT_ASSERT_EQUAL( 42, get_opt_int("int0") );
    int intVar;
    get_opt( "int0", intVar );
    CPPUNIT_ASSERT_EQUAL( 42, intVar );
    intVar = 43;
    CPPUNIT_ASSERT_EQUAL( 42, get_opt_int("int0") );
  }

  void testConfigurationWrongType() {
    float fl = 0.1;
    CPPUNIT_ASSERT( option_valid("int0") );
    CPPUNIT_ASSERT_THROW( get_opt("int0", fl), WrongTypeException );
    CPPUNIT_ASSERT_THROW( set_opt("int0", fl), WrongTypeException );
  }

  // test functionality for an "observed" option variable
  void testConfigurationReference() {
    CPPUNIT_ASSERT(! option_valid("int1") );
    int int1 = 0;
    reference_opt("int1", "reference int option", int1);
    set_opt("int1", 42);
    const int& intRef = get_opt_int("int1");
    CPPUNIT_ASSERT_EQUAL( 42, intRef );
    CPPUNIT_ASSERT_EQUAL( 42, get_opt_int("int1"));
    CPPUNIT_ASSERT( &int1 == &intRef );
    //intRef = 43;  // forbidden: good! :-)
    //CPPUNIT_ASSERT_EQUAL( 43, int1 );

    float float0 = 0.1f;
    reference_opt("MyFloat", "", float0);
    const float& myFloat = get_opt<float>("MyFloat");
    CPPUNIT_ASSERT_EQUAL( 0.1f, myFloat );
    set_opt("MyFloat", 15.6f);
    //comparing floats is generally not a good idea but here it should work
    CPPUNIT_ASSERT_EQUAL( 15.6f, myFloat );
    CPPUNIT_ASSERT( &float0 == &myFloat );

    // Be aware that the following code may entail the copy of a float. If this
    // was a complex object, it would be a copy in any case.
    float myFloat3;
    get_opt("MyFloat", myFloat3);
    //comparing floats is generally not a good idea but here it should work
    CPPUNIT_ASSERT_EQUAL( 15.6f, myFloat3 );
    CPPUNIT_ASSERT( &float0 != &myFloat3 );
    myFloat3 = 0.0;
    CPPUNIT_ASSERT_EQUAL( 15.6f, float0 );

    myFloat3 = get_opt<float>("MyFloat");
    //comparing floats is generally not a good idea but here it should work
    CPPUNIT_ASSERT_EQUAL( 15.6f, myFloat3 );
    CPPUNIT_ASSERT( &float0 != &myFloat3 );
    myFloat3 = 0.0;
    CPPUNIT_ASSERT_EQUAL( 15.6f, float0 );
  }
  
  void testConfigurationCharPointer() {
    managed_opt("charp0", "managed char pointer", (const char*) "badbadbad");
    CPPUNIT_ASSERT_EQUAL( strcmp("badbadbad", get_opt_charp("charp0") ), 0 );
    set_opt("charp0", (const char *)"goodgoodgood");
    CPPUNIT_ASSERT_EQUAL( strcmp("goodgoodgood", get_opt_charp("charp0") ), 0 );
    char * aString = new char[100];
    strcpy(aString, "A constant String");
    reference_opt("charp1", "managed char pointer", aString);
    CPPUNIT_ASSERT_EQUAL( strcmp("A constant String",
                                 get_opt<char*>("charp1") ), 0 );
    delete[] aString;
  }


  void testAlreadyExists() {
    managed_opt("entry1", "doc1", (int)0);
    CPPUNIT_ASSERT_THROW( managed_opt("entry1", "doc2", (int)1),
                          DuplicateOptionException );

    int entry2;
    reference_opt("entry2", "doc3", entry2);
    CPPUNIT_ASSERT_THROW( managed_opt("entry2", "doc4", (int)0),
                          DuplicateOptionException );
  }

  void testCallback() {
    MyCallbackTester *cbp = new MyCallbackTester();

    callback_opt<int>("cb1", "a callback option", cbp);
    // the callback is deleted by the callback option !!!!
    MyCallbackTester &cb = *cbp;

    CPPUNIT_ASSERT_EQUAL( 0, cb.callsCounter );
    
    set_opt("cb1", 47);
    CPPUNIT_ASSERT_EQUAL( 1, cb.callsCounter );
    CPPUNIT_ASSERT_EQUAL( 47, cb.lastValue );
    
    set_opt("cb1", 49);
    CPPUNIT_ASSERT_EQUAL( 49, cb.lastValue );
  }
    
  void testPriority() {
    managed_opt<int>("prio1", "doc11", 0);
    set_opt("prio1", 58);
    CPPUNIT_ASSERT_EQUAL( 58, get_opt_int("prio1"));
        
    set_opt("prio1", 59, 10);
    CPPUNIT_ASSERT_EQUAL( 59, get_opt_int("prio1"));
        
    set_opt("prio1", 44, 9);
    CPPUNIT_ASSERT_EQUAL( 59, get_opt_int("prio1"));
        
    set_opt("prio1", 44, 11);
    CPPUNIT_ASSERT_EQUAL( 44, get_opt_int("prio1"));
  }



  /*
  void testConfigurationComplexOwn() {
    StatusMap statusmembers;
    Configuration::addReference("statusmember1", &statusmembers);
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


  // test if the descriptions of options are handled correctly
  void testDescription() {
    Configuration::addOption<int>("opt_desc_H", "desc1");
    CPPUNIT_ASSERT_EQUAL(std::string("desc1"),
                         Configuration::getDescription("opt_desc_H"));

    int i;
    Configuration::addReference<int>("opt_desc_R", &i, "desc2");
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
    Configuration::addReference<int>("initR1", &i, "description", 5);
    CPPUNIT_ASSERT_EQUAL( 5, i );
    CPPUNIT_ASSERT_EQUAL( 5, Configuration::get<int>("initR1") );
    
    double d;
    Configuration::addReference<double>("initR2", &d);
    CPPUNIT_ASSERT_EQUAL( 0.0, d );
    CPPUNIT_ASSERT_EQUAL( 0.0, Configuration::get<double>("initR2") );

    WithDefConstructor wdc1, wdc2;
    Configuration::addReference<WithDefConstructor>("initR3", &wdc1, "descr",
                                                 WithDefConstructor("xyz"));
    CPPUNIT_ASSERT_EQUAL( string("xyz"), wdc1.s_ );
    Configuration::addReference<WithDefConstructor>("initR4", &wdc2);
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
    Configuration::addReference<int>("conv_2", &i, "descr", 3,
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
    Configuration::addReference<int>("mconv_2", &i, "descr", 1, &mc);
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
  */
};

// CPPUNIT_TEST_SUITE_REGISTRATION( ConvertersTest );
CPPUNIT_TEST_SUITE_REGISTRATION( ConfigurationTest );

int main(int argc, char *argv[]) {
#if HAVE_LIBLOG4CXX
  log4cxx::BasicConfigurator::configure();
  log4cxx::Logger::getRootLogger()->setLevel(log4cxx::Level::OFF);
#endif // HAVE_LIBLOG4CXX
  
  CppUnit::Test *suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();

  CppUnit::TextUi::TestRunner runner;
  runner.addTest( suite );

  runner.setOutputter( new CppUnit::CompilerOutputter( &runner.result(),
                                                       std::cerr ) );

  bool wasSuccessful = runner.run("", false);
  Config::finalize();
  return wasSuccessful ? 0 : 1;
}

#else

#include <iostream>
#include "configs.h"

int main(int argc, char *argv[]) {
  handled_value<int> val(0);
  handled_value<char*> cval("My Teststring");
  val.set(3);
  std::string foo = cval.get_as_string();
  std::cout << foo << " " <<  val.get() << std::endl;
  return 0;
}
#endif
