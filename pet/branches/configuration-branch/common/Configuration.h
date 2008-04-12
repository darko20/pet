/* -*- Mode: C++ -*- */
#ifndef _CONFIGURATION_H
#define _CONFIGURATION_H

#include "pet-config.h"

#include <map>
#include <set>
#include <string>
#include <typeinfo>
#include <functional>
#include <sstream>

#include "errors.h"

/** @file Config.h
 *   @brief System for managing various configuration options in a program.
 */

/** @brief Root of all exceptions used in configuration system. */
class ConfigException : public tError {
public:
  ConfigException(std::string message);
};

/** @brief Exception thrown when string conversion not available. */
class NoConverterException : public ConfigException {
public:
  NoConverterException(std::string message) : ConfigException(message) {};
};


/**
 * @brief Converts values to/from string.
 */
template<class T>
class AbstractConverter {
public:
  virtual ~AbstractConverter() {}

  /**
   * Convert to std::string.
   * @param  t  value to be converted.
   * @return    string representation of t.
   * @exception ConversionException it is impossible
   *                                to convert the given value to string
   */
  virtual std::string toString(const T& t) = 0;
  
  /**
   * Parse string to extract value of type T.
   * @param  s  string to be parsed.
   * @return    extracted value.
   * @exception ConvertionException s is not a string representation
   *                                of a value of type T  
   */
  virtual T fromString(const std::string& s) = 0;

};

// Implementations and internal classes
#include "ConfigurationInternal.h"

/**
 * @brief Converts values to/from string in way defined by user.
 *
 * This converter uses map provided by user, that contains strings and
 * respective values. This allows conversion like:
 * 1 <-> "one", 2 <-> "two", 3 <-> "too much".
 * Conversion is case sensitive.
 */
template<class T>
class MapConverter : public AbstractConverter<T> {
public:
  typedef std::map<std::string,T> Map;
  typedef std::map<T,std::string> RMap;

  /*
   * @param m mapping from strings to values of type T. It is vital that
   *          m is "1 to 1" mapping, i.e. it is not allowed to have the same
   *          value for two strings.
   */
  MapConverter(Map m) : str2T_(m) {
    for(typename Map::iterator
          i = str2T_.begin();
        i != str2T_.end();
        i++)
      t2Str_[i->second] = i->first;
  }
  
  virtual std::string toString(const T& t) {
    typename RMap::iterator i =
      t2Str_.find(t);
    if(i == t2Str_.end())
      throw ConfigException(std::string("cannot convert to string, type ")
                            + typeid(T).name());
    
    return i->second;
  }

  virtual T fromString(const std::string& s) {
    typename Map::iterator i =
      str2T_.find(s);
    if(i == str2T_.end())
      throw ConfigException(
        std::string("cannot convert from string (") + s + "), type"
        + typeid(T).name());
    
    return i->second;
  }
  
private:
  Map str2T_;
  RMap t2Str_;
};

/**
 * @brief Helper class for managing options with callback functions.
 *
 * Methods of this class are called when value of an option is read or written.
 * An inheriting class is responsible for storing a value of an option.
 */
template<class T> class Callback {
public:
  virtual ~Callback() {}
  
  /** Set new value of the option.
   * @param val New value.
   */
  virtual void set(const T& val) = 0;

  /** Get value of the option.
   * @return Current value of the option.
   */
  virtual T& get() = 0;
};

/** @brief Stores configuration entries.
 */
class Config {
public:
  ~Config();

  /** Adds new configuration option. Value is handled internally
   * @param entry Name of the option.
   * @param description
   * @param initial initial value of the option.
   * @param converter converter used for conversion to/from
   *                  std::string of this option
   *
   * @exception ConfigException Thrown if option already exists or name is
   *                            empty
   */
  template<class T> void 
  addOption(const std::string& entry, const std::string& description,
            const T &initial, AbstractConverter<T> *converter) {
    add(entry, new HandledOption<T>(description, initial, converter));
  }

  template<class T> void 
  addOption(const std::string& entry, const std::string& description,
            const T &initial) {
    add(entry, new HandledOption<T>(description, initial));
  }
  
  /** Adds new configuration option. Value is stored in given location.
   * @param entry Name of the option.
   * @param description
   * @param place Reference to the place where the value is stored.
   * @param converter converter used for conversion to/from
   *                  std::string of this option
   *
   * @exception ConfigException Thrown if option already exists or name is
   *                            empty.
   */
  template<class T> void 
  addReference(const std::string& entry, const std::string& description,
               T &place, AbstractConverter<T> *converter) {
    add(entry, new ReferenceOption<T>(description, place, converter));
  }

  template<class T> void 
  addReference(const std::string& entry, const std::string& description,
               T &place) {
    add(entry, new ReferenceOption<T>(description, place));
  }


  /** Adds new callback configuration option, which provides the accessors.
   * @param entry       Name of the option.
   * @param callback    Function-object which is called every time value of
   *                    option is changed
   * @param description 
   *
   * @attention         The callback object must persist. It can not be
   *                    an object on the stack.
   *
   * @exception ConfigException Thrown if option already exists or name is
   *                            empty.
   */
  template <class T> void
  addCallback(const std::string& entry, const std::string& description,
              Callback<T>* callback) {
    add(entry, new CallbackOption<T>(description, callback));
  }

  /** Gets value of configuration option, if it exists.
   * @param entry Name of the option.
   *
   * @return reference of type T: the value.
   *
   * @exception ConfigException Thrown if option does not exist.
   *                            Also if the option's type is different from T.
   */
  template <class T> T& get(const std::string& entry) {
    LOG(logger, log4cxx::Level::DEBUG, "getting option %s", entry.c_str());
    
    try {
      Option<T>& o = dynamic_cast<Option<T>&>( getIOption(entry) );
      return o.get();
    }
    catch(std::bad_cast) {
      throw ConfigException("wrong type was used for option " + entry);
    }
  };
  
  /** Works exactly like #get(const std::string& entry), but result is passed
   * as an "out" parameter. Throws the same exceptions.
   * @param entry
   * @param place Here goes the result.
   * @exception ConfigException Thrown if option does not exist.
   *                            Also if the option's type is different from T.
   */
  template <class T> void get(const std::string& entry, T& place);

  /* Don't access the values directly. Use reference options if you want to.

  ** Works exactly like #get(const std::string& entry), but result is passed
   * as an "out pointer" parameter. Throws the same exceptions.
   * @param entry
   * @param place Here goes the result.
   * @exception ConfigException Thrown if option does not exist.
   *                            Also if the option's type is different from T.
   *
  template<class T> void getPointer(const std::string& entry, (T*)& place);

  ** Works exactly like #get(const std::string& entry), but result is passed
   * as an "out pointer" parameter. Throws the same exceptions.
   * @param entry
   * @param place Here goes the result.
   * @exception ConfigException Thrown if option does not exist.
   *                            Also if the option's type is different from T.
   *
  template<class T> T* getPointer(const std::string& entry);
  */

  /** Sets the value of an existing configuration option.
   * @param entry Name of the option.
   * @param value New value.
   * @param prio  Priority of current invocation. Works like #Option<T>::set.
   *
   * @exception ConfigException Thrown if option does not exist.
   *                            Also if the option's type is different from T.
   */
  template <class T> void 
  set(const std::string& entry, const T& value, int prio = 1) {
#if HAVE_LIBLOG4CXX
    std::stringstream ss;
    ss << "setting option " << entry << ", priority is " << prio;
    LOG4CXX_DEBUG(logger, ss.str());
#endif // HAVE_LIBLOG4CXX
    
    try {
      Option<T>& o = dynamic_cast<Option<T>&>( getIOption(entry) );
      o.set(value, prio);
    }
    catch(std::bad_cast) {
      throw ConfigException("wrong type was used for option " + entry);
    }
  }

  /** Sets the value of an existing configuration option from a string.
   * Works like #set, throws the same exceptions.
   */
  void 
  setString(const std::string& entry, const std::string& value, int prio = 1);
  
  /** Get the value of an option as string.
   * Works like #get(const std::string& entry), throws the same exceptions
   */
  std::string
  getString(const std::string& entry);

  /** Checks if given name exists in configuration database.
   * @param entry Name of the parameter.
   * @return true iff entry is name of existing option.
   */
  bool hasOption(const std::string& entry);

  /** Retrieves description of an option.
   * @param entry Name of the option.
   * @return description of the option.
   *
   * @exception ConfigException Thrown if option does not exist.
   */
  const std::string& getDescription(const std::string& entry);

  /** Get the singleton instance, or create it if not yet available */
  inline static Config *get_instance() { 
    if (_instance) return _instance;
    return (_instance = new Config());
  }

protected:
  /** Checks if string can be used as a name for an configuration option.
   * @param entry String to be checked.
   * @return true iff entry can be name of an option.
   */
  bool isValidName(const std::string& entry) const;

  /** Get IOption for given entry. This is common part of a few functions
   * in Config.
   * @param entry name of the option.
   * @return IOption for given entry.
   *
   * @exception ConfigException Thrown if option does not exist.
   */
  IOption& getIOption(const std::string& entry);

#if HAVE_LIBLOG4CXX
  static log4cxx::LoggerPtr logger;
#endif // HAVE_LIBLOG4CXX

private:
  // Checks if the name can be used as a name of a new option.
  // throws WrongEntryNameException, EntryAlreadyExistsException
  void check(const std::string &entry);

  // Adds a new option to current configuration.
  void add(const std::string &entry, IOption* option);

  Config();

  static Config *_instance;
  typedef std::map<const std::string, IOption*> KeyValueMap;
  KeyValueMap keyValue_;
};

/******************************************************************************
 * Implementation of Config
 *****************************************************************************/

template<class T> void get_opt(const std::string& entry, T& place) {
  place = Config::get_instance()->get<T>(entry);
}

template<class T> T& get_opt(const std::string& entry) {
  return Config::get_instance()->get<T>(entry);
}

inline bool get_opt_bool(const std::string& entry) {
  return get_opt<bool>(entry);
}

inline int get_opt_int(const std::string& entry) {
  return get_opt<bool>(entry);
}

inline char * get_opt_charp(const std::string& entry) {
  return get_opt<char *>(entry);
}

inline std::string &get_opt_string(const std::string& entry) {
  return get_opt<std::string>(entry);
}

/* Don't access the values directly. Use reference options if you want to.
template<class T> 
void Config::getPointer(const std::string& entry, T* place) {
  place = &(get<T>(entry));
}

template<class T>
T * Config::getPointer(const std::string& entry) {
  return &(get<T>(entry));
}
*/

template<class T> void 
set_opt(const std::string& entry, const T& value, int prio = 1){
  Config::get_instance()->set<T>(entry, value, prio);
}

inline void 
set_optString(const std::string& entry, const std::string& value, int prio = 1){
  Config::get_instance()->setString(entry, value, prio);
}

/** Registration: don't forget the doc strings. This may require mentioning
 *  the template parameter explicitely. Check this.
 *  Would be nice if we had another string for the module to get a sensible
 *  grouping in a GUI, for example.
 */
template<class T>
void managed_opt(const std::string &key, const std::string &docstring,
                 T initialValue) {
  Config::get_instance()->addOption<T>(key, docstring, initialValue);
}

template<class T>
void reference_opt(const std::string key, const std::string docstring, 
                   T &initialValue) {
  Config::get_instance()->addReference<T>(key, docstring, initialValue);
}
// do we need that if we provide custom converters???
// yes, if we would like to change a data member inside an object instead of
// the object itself.
template<class T>
void callback_opt(const std::string key, const std::string docstring,
                  Callback<T> &cb) {
  Config::get_instance()->addCallback<T>(key, docstring, cb);
}

#endif //_CONFIGURATION_H

/* API wishlist */

/*
template <class T> T& getRequired(std::string optionName);
bool available(std::string optionName);
template <class T> T& get(std::string optionName);
*/

/* replacement of used fns
member/statusmember: set<string> valued option (with convenience fn?)

is type_status in the set described by which_status??
statusmember(string which_status, string type_status)

// -------------------- Definition

// leave the set handling to the Config: creation, destruction, etc.
Config.addOption< std::set<std::string> >("<which_status>")

// I want the set in my hands, which means i can access it directly without
// searching for it by string. 
static std::set<std::string> which_status_set ;
Config.addOption("<which_status>", which_status_set)

// with default value only
Config.addOption("<some_flag>", false);

// place and default value: not needed. Supply default value beforehand
int foo;
Config.addOption("<int_opt>", &foo);

// -------------------- Access

int bar = Configuraton.get("<int_opt>");

assoc: very special convenience function for an assoc list data type
*/
