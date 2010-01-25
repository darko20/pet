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

/** Root of all exceptions used in configuration system. */
class ConfigException : public tError {
public:
  ConfigException(std::string message);
};

/** @brief Exception thrown when string conversion not available. */
class ConversionException : public ConfigException {
public:
  ConversionException(std::string convertername) 
    : ConfigException(convertername) {};
  ConversionException(std::string key, std::string value, 
                      std::string convertername) 
    : ConfigException("`" + value +  "' can not be converted " +
                      "to fit the type of " + key
                      + " (" + convertername +")") {};
};

/** Thrown when the specified option is not registered. */
class NoSuchOptionException : public ConfigException {
public:
  NoSuchOptionException(std::string message) 
    : ConfigException("no such option: " + message) {};
};
 
/** Thrown when option is registered more than once. */
class DuplicateOptionException : public ConfigException {
public:
  DuplicateOptionException(std::string message)
    : ConfigException("duplicate option: " + message) {};
};

/** Thrown when the requested value type is not appropriate for the registered
    option. */
class WrongTypeException : public ConfigException {
public:
  WrongTypeException(std::string message)
    : ConfigException("Wrong type requested for option: " + message) {};
};


/** An abstract template class to convert things from and to string
    representations */
template<class T> class AbstractConverter {
public:
  virtual std::string toString(const T &val) = 0;
  virtual T fromString(const std::string &s) = 0;
};


/** Helper class for managing options with callback functions.
 * 
 * Methods of this class are called when value of an option is read or written.
 * An inheriting class is responsible for storing a value of an option.
 */
template<class T> class Callback {
public:
  virtual ~Callback() {}
  
  /** Set new value of the option to \a val */
  virtual void set(const T& val) = 0;

  /** Return the current value of the option. */
  virtual T& get() = 0;
};


/******************************************************************************
 * Implementation and internal classes of Config
 *****************************************************************************/
#include "configs-inner.h"

/******************************************************************************
 * Interface functions of Config
 *****************************************************************************/

/** Return \c true if an option with name \a key exists, \c false otherwise */
inline bool option_valid(const std::string &key) {
  return Config::get_instance()->hasOption(key);
}

/** Retrieves description of the option with name \a key
 */
inline const std::string& option_description(const std::string& key) {
  return Config::get_instance()->description(key);
}

/** Get the value of the option with name \a key and put it into \a place.
 *  \throw NoSuchOptionException wrong key
 *  \throw WrongTypeException the option \a key has a type different from \c T
 */
template<class T> void get_opt(const std::string& key, T& place) {
  place = Config::get_instance()->get<T>(key);
}

/** \brief Get the value of the option that with name \a key. The template
 *  parameter must be explicitely specified for this method, because the return
 *  value can not be used to select the correct template.
 *  \throw NoSuchOptionException wrong key
 *  \throw WrongTypeException the option \key has a type different from \c T
 *  This function has shortcuts for the most common simple types, like
 *  get_opt_bool, get_opt_int 
 */
template<class T> const  T& get_opt(const std::string& key) {
  return Config::get_instance()->get<T>(key);
}

/** Get the value of the option with name \a key and return it in a readable
 *  form, accepted by set_opt_from_string.
 *  \throw NoSuchOptionException wrong key
 *  \throw ConversionException the value for \a key can not be converted to a
 *                             string 
 */
inline std::string get_opt_as_string(const std::string& key) {
  return Config::get_instance()->get_as_string(key);
}

/* @{ */
inline const bool &get_opt_bool(const std::string& key) { return get_opt<bool>(key); }

inline const char &get_opt_char(const std::string& key) { return get_opt<char>(key); }

inline const int &get_opt_int(const std::string& key) { return get_opt<int>(key); }

inline const char * const &get_opt_charp(const std::string& key) { 
  return get_opt<const char *>(key);
}

inline const std::string &get_opt_string(const std::string& key) {
  return get_opt<std::string>(key);
}
/* @} */

/** Set the option that with name \a key to \a val
 *  \throw NoSuchOptionException wrong key
 *  \throw WrongTypeException the option \key requires a type different
 *                            from \c T
 */
template<class T> void 
set_opt(const std::string& key, const T& value, int prio = 0){
  Config::get_instance()->set<T>(key, value, prio);
}

/** Set the option that with name \a key to the (possibly translated) value
 *  \a val.
 *  \throw NoSuchOptionException wrong key
 *  \throw ConversionException value can not be converted to the appropriate
 *         type for the given option
 *  \todo maybe find a better name for the function
 */
inline void 
set_opt_from_string(const std::string& key, const std::string& value,
                    int prio = 0){
  Config::get_instance()->set_from_string(key, value, prio);
}

/** Register a managed option that is completely handled by the configuration
 *  system.
 *  These options are only referenced using the key, and should not be used
 *  directly in a time-critical environment.
 *  \todo Would be nice if we had another string for the module to get a
 *  sensible grouping in a GUI, for example.
 */
template<class T>
void managed_opt(const std::string &key, const std::string &docstring,
                 T initialValue) {
  Config::get_instance()->addOption<T>(key, docstring, initialValue);
}

template<class T>
void managed_opt(const std::string &key, const std::string &docstring,
                 T initialValue,
                 typename value<T>::fromfunc from,
                 typename value<T>::tofunc to) {
  Config::get_instance()->addOption<T>(key, docstring, initialValue, from, to);
}

template<class T>
void managed_opt(const std::string &key, const std::string &docstring,
                 T initialValue,
                 AbstractConverter<T> *conv) {
  Config::get_instance()->addOption<T>(key, docstring, initialValue, conv);
}

/** \brief Register a reference option, i.e., a variable whose value shall be
 *   changed directly through the configuration system.
 *  Use this if you want to change/get the value of the variable in the same
 *  moment where you set/query it.
 *  \todo Would be nice if we had another string for the module to get a
 *  sensible grouping in a GUI, for example.
 */
template<class T>
void reference_opt(const std::string key, const std::string docstring, 
                   T &initialValue) {
  Config::get_instance()->addReference<T>(key, docstring, initialValue);
}

/** Register an option that calls set/get functions instead of seeing the
 *  option directly.
 *  This is needed even if there are custom converters, since somebody might
 *  like to change a data member inside an object instead of the object itself.
 */
template<class T>
void callback_opt(const std::string key, const std::string docstring,
                  Callback<T> *cb) {
  Config::get_instance()->addCallback<T>(key, docstring, cb);
}

#endif //_CONFIGURATION_H

