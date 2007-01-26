/* -*- Mode: C++ -*- */
#ifndef _CONFIGURATION_H
#define _CONFIGURATION_H

#include "pet-config.h"

#include <map>
#include <set>
#include <string>
#include <typeinfo>
#include <functional>

#include "ConfigurationInternal.h"

/** @file Configuration.h
 *   @brief System for managing various configuration options in a program.
 */

/** @brief Root of all exceptions used in configuration system. */
class ConfigurationException {};

/** @brief There is no option with given name. */
class NoSuchEntryException : public ConfigurationException {};

/** @brief Option has different type than one given. */
class WrongTypeException : public ConfigurationException {};

/** @brief System does not allow given option name.
 *  Eg. empty name can be forbidden.
 */
class WrongEntryNameException : public ConfigurationException {};

/** @brief Entry with given name already exists in configuration. */
class EntryAlreadyExistsException : public ConfigurationException {};

/**
 * @brief Helper class for managing options with callback functions.
 *
 * Methods of this class are called when value of an option is read or written.
 * An inheriting class is responsible for storing a value of an option.
 */
template<class T> class Callback {
public:
  virtual ~Callback() {}
  
  /**
   * Set new value of the option.
   * @param val New value.
   */
  virtual void set(const T& val) = 0;

  /**
   * Get value of the option.
   * @return Current value of the option.
   */
  virtual T& get() = 0;
};

/**
 * @brief Stores configuration entries.
 */
class Configuration {
public:
  ~Configuration();

  /**
   * Adds new configuration option. Value is handled internally
   * @param entry Name of the option.
   * @param description
   * @param initial initial value of the option.
   *
   * @exception WrongEntryNameException     Thrown if entry is not valid name,
   *                                        eg. it is empty.
   * @exception EntryAlreadyExistsException Thrown if entry already exists.
   */
  template<class T> static void 
  addOption(const std::string& entry, const std::string& description = "",
            T initial = T());

  /**
   * Adds new configuration option. Value is stored in given location.
   * @param entry Name of the option.
   * @param value Pointer to place where value is stored.
   * @param description
   * @param initial initial value of the option.
   *
   * @exception WrongEntryNameException     Thrown if entry is not valid name,
   *                                        eg. it is empty.
   * @exception EntryAlreadyExistsException Thrown if entry already exists.
   */
  template<class T> static void 
  addOption(const std::string& entry, T* value,
            const std::string& description = "", T initial = T());

  /**
   * Adds new callback configuration option. Only access functions are provided
   * @param entry       Name of the option.
   * @param callback    Function-object which is called every time value of
   *                    option is changed
   * @param description 
   *
   * @attention         The callback object must persist. It can not be
   *                    an object on the stack.
   *
   * @exception WrongEntryNameException     Thrown if entry is not valid name,
   *                                        eg. it is empty.
   * @exception EntryAlreadyExistsException Thrown if entry already exists.
   */
  template <class T> static void
  addCallback(const std::string& entry, Callback<T>* callback,
              const std::string& description = "");

  /**
   * Gets value of configuration option, if it exists.
   * @param entry Name of the option.
   *
   * @return reference of type T: the value.
   *
   * @exception WrongEntryNameException Thrown if entry is not valid name,
   *                                    eg. it is empty.
   * @exception NoSuchEntryException    Thrown if there is no option with
   *                                    given name.
   * @exception WrongTypeException      Thrown if option has different type
   *                                    than given (by the type of function).
   */
  template <class T> static T& get(const std::string& entry);
  
  /**
   * Works exactly like #get(const std::string& entry), but result is passed
   * as an "out" parameter. Throws the same exceptions.
   * @param entry
   * @param place Here goes the result.
   */
  template <class T> static void get(const std::string& entry, T& place);

  /**
   * Sets the value of an existing configuration option.
   * @param entry Name of the option.
   * @param value New value.
   * @param prio  Priority of current invocation. Works like #Option<T>::set.
   *
   * @exception WrongEntryNameException Thrown if entry is not valid name,
   *                                    eg. it is empty.
   * @exception NoSuchEntryException    Thrown if there is no option with
   *                                    given name.
   * @exception WrongTypeException      Thrown if option has different type
   *                                    than given (by the type of function).
   */
  template <class T> static void 
  set(const std::string& entry, const T& value, int prio = 1);

  /**
   * Checks if given name exists in configuration database.
   * @param entry Name of the parameter.
   * @return true iff entry is name of existing option.
   */
  static bool hasOption(const std::string& entry);

  /**
   * Retrieves description of an option.
   * @param entry Name of the option.
   * @return description of the option.
   *
   * @exception WrongEntryNameException Thrown if entry is not valid name,
   *                                    eg. it is empty.
   * @exception NoSuchEntryException    Thrown if there is no option with
   *                                    given name.
   */
  static const std::string& getDescription(const std::string& entry);

protected:
  /**
   * Checks if string can be used as a name for an configuration option.
   * @param entry String to be checked.
   * @return true iff entry can be name of an option.
   */
  bool isValidName(const std::string& entry) const;

#if HAVE_LIBLOG4CXX
  static log4cxx::LoggerPtr logger;
#endif // HAVE_LIBLOG4CXX

private:
  // Checks if the name can be used as a name of a new option.
  // throws WrongEntryNameException, EntryAlreadyExistsException
  void check(const std::string &entry);

  // Adds a new option to current configuration.
  void add(const std::string &entry, IOption* option);

  Configuration();

  static Configuration* instance_;
  typedef std::map<const std::string, IOption*> KeyValueMap;
  KeyValueMap keyValue_;
};

/******************************************************************************
 * Implementation of Configuration
 *****************************************************************************/

template<class T> void 
Configuration::addOption(const std::string& entry,
                         const std::string& description, T initial)
{
  instance_->check(entry);
  Option<T> *opt = new HandledOption<T>(description);
  opt->set(initial);
  instance_->add(entry, opt);
}

template<class T> void 
Configuration::addOption(const std::string& entry, T* value,
                         const std::string& description, T initial)
{
  instance_->check(entry);
  Option<T> *opt = new ReferenceOption<T>(value, description);
  opt->set(initial);
  instance_->add(entry, opt);
}

template <class T> void
Configuration::addCallback(const std::string& entry, Callback<T>* callback,
                           const std::string& description)
{
  instance_->check(entry);
  instance_->add(entry, new CallbackOption<T>(callback, description));
}

template<class T> 
T& Configuration::get(const std::string& entry) {
  LOG4CXX_DEBUG(logger, "getting entry " + entry);

  if(!instance_->isValidName(entry))
    throw WrongEntryNameException();
    
  KeyValueMap::iterator i = instance_->keyValue_.find(entry);

  if(i == instance_->keyValue_.end())
    throw NoSuchEntryException();
    
  IOption& io = *(i->second);

  try {
    Option<T>& o = dynamic_cast<Option<T>&>(io);
    return o.get();
  }
  catch(std::bad_cast) {
    throw WrongTypeException();
  }
}

template<class T> 
void Configuration::get(const std::string& entry, T& place) {
  place = get<T>(entry);
}

template<class T>
void Configuration::set(const std::string& entry, const T& value, int prio) {
#if HAVE_LIBLOG4CXX
  std::stringstream ss;
  ss << "setting entry " << entry << ", priority is " << prio;
  LOG4CXX_DEBUG(logger, ss.str());
#endif // HAVE_LIBLOG4CXX

  if(!instance_->isValidName(entry))
    throw WrongEntryNameException();
    
  KeyValueMap::iterator i = instance_->keyValue_.find(entry);

  if(i == instance_->keyValue_.end())
    throw NoSuchEntryException();
    
  IOption& io = *(i->second);

  try {
    Option<T>& o = dynamic_cast<Option<T>&>(io);
    o.set(value, prio);
  }
  catch(std::bad_cast) {
    throw WrongTypeException();
  }
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

// leave the set handling to the Configuration: creation, destruction, etc.
Configuration.addOption< std::set<std::string> >("<which_status>")

// I want the set in my hands, which means i can access it directly without
// searching for it by string. 
static std::set<std::string> which_status_set ;
Configuration.addOption("<which_status>", which_status_set)

// with default value only
Configuration.addOption("<some_flag>", false);

// place and default value: not needed. Supply default value beforehand
int foo;
Configuration.addOption("<int_opt>", &foo);

// -------------------- Access

int bar = Configuraton.get("<int_opt>");

assoc: very special convenience function for an assoc list data type
*/
