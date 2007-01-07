/* -*- Mode: C++ -*- */
#ifndef _CONFIGURATION_H
#define _CONFIGURATION_H

#include <map>
#include <set>
#include <string>
#include <typeinfo>
#include <functional>

#if HAVE_LIBLOG4CXX
#  include <log4cxx/logger.h>
#  include <log4cxx/basicconfigurator.h>
#  include <sstream>
#else
#  ifndef LOG4CXX_DEBUG // despite lack of log4cxx if can be defined by user
#    define LOG4CXX_DEBUG(logger, msg) ;
#    define LOG4CXX_INFO(logger, msg) ;
#    define LOG4CXX_WARN(logger, msg) ;
#    define LOG4CXX_ERROR(logger, msg) ;
#    define LOG4CXX_FATAL(logger, msg) ;
#  endif // LOG4CXX_DEBUG
#endif // HAVE_LIBLOG4CXX

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


/** @brief Root class for managing one option. */
class IOption {
public:
  IOption() : greatestPriority_(0) {}

  virtual ~IOption() {}

protected:
  /**
   * Check if the given priority is greatest or equal to the highest
   * priority that has already been used. If yes than record it as
   * a new highest priority. See #Option<T>::set for details.
   * @param priority priority to check
   * @return true if the given priority is greatest or equal to the highest
   *              priority that has already been used
   */
  inline bool checkAndSetPriority(int priority) {
    if (priority >= greatestPriority_) {
      greatestPriority_ = priority;
      return true;
    }
    return false;
  }

private:   
  int greatestPriority_; //greatest value used in set()
};

/**
 * @brief Abstract class for managing one option of given type.
 */
template<class T> class Option : public IOption {
public:
  virtual ~Option() {}

  /** Returns value of the option. */
  virtual T& get() = 0;

  /**
   * Set value of the option.
   * @param value New value.
   * @param priority If priority is equal or grater than the one used in 
   *       previous calls, then assignment takes place. Otherwise the call is
   *       ignored. Eg. you can use priority = 10 for configuration file, and
   *       20 for command line options, then command line always overrides
   *       configuration file, even if configuration file is processed later.
   */
  virtual void set(const T& value, int priority = 1) = 0;
};

/**
 * @brief  Concrete classes for managing options.
 * 
 * Object of this class stores the value of an option internally, as a field.
 */
template<class T>
class HandledOption : public Option<T> {
public:
  virtual ~HandledOption() {}

  /** Returns value of the option. */
  virtual T& get() { return value_; }

  /**
   * Set value of the option.
   * @param value New value.
   * @param priority See parent class for details.
   * @see Option::set
   */
  virtual void set(const T& value, int priority = 1) {
    if (IOption::checkAndSetPriority(priority))
      value_ = value;
  }
  
private:
  //we always acccess data using value_, but it can point to a object whose
  //memory is handled by the \c Option
  T value_;
};

/**
 * @brief Concrete classes for managing options.
 *
 * Object of this class stores a pointer to location of option's variable.
 */
template<class T>
class ReferenceOption : public Option<T> {
public:
  /**
   * If object is created with this constructor, then it uses external (via
   * reference) variable for value of the option.
   * @param valueRef Reference to variable which stores value of the option.
   */
  ReferenceOption(T* valueRef) : value_(valueRef) {}

  virtual ~ReferenceOption() {}

  /** Returns value of the option. */
  virtual T& get() { return *value_; }

  /**
   * Set value of the option.
   * @param value New value.
   * @param priority See parent class for details.
   * @see Option::set
   */
  virtual void set(const T& value, int priority = 1) {
    if (IOption::checkAndSetPriority(priority))
      *value_ = value;
  }

private:
  ReferenceOption() {}

  //we always acccess data using value_, but it can point to a object whose
  //memory is handled by the \c Option
  T* value_;
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
 * @brief Concrete classes for managing callback options of given type.
 *
 * An user-provided callback is responsible for storing a value of an option.
 */
template<class T>
class CallbackOption : public Option<T> {
public:
  /**
   * If object is created with this constructor, sets the value via the
   * callback
   * @param callback Function-object which is called every time value of option
   *                 is changed
   */
  CallbackOption(Callback<T>* callback) : callback_(*callback) {}

  virtual ~CallbackOption() {}

  /** Returns value of the option. */
  virtual T& get() { 
    return callback_.get();
  }

  /**
   * Set value of the option.
   * @param value New value.
   * @param priority See parent class for details.
   * @see Option::set
   */
  virtual void set(const T &value, int priority = 1) {
    if (IOption::checkAndSetPriority(priority))
      callback_.set(value);
  }

private:
  Callback<T>& callback_;
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
   *
   * @exception WrongEntryNameException     Thrown if entry is not valid name,
   *                                        eg. it is empty.
   * @exception EntryAlreadyExistsException Thrown if entry already exists.
   */
  template<class T> static void 
  addOption(const std::string& entry, const std::string& description = "");

  /**
   * Adds new configuration option. Value is stored in given location.
   * @param entry Name of the option.
   * @param value Pointer to place where value is stored.
   * @param description
   *
   * @exception WrongEntryNameException     Thrown if entry is not valid name,
   *                                        eg. it is empty.
   * @exception EntryAlreadyExistsException Thrown if entry already exists.
   */
  template<class T> static void 
  addOption(const std::string& entry, T* value,
            const std::string& description = "");

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
                         const std::string& description)
{
  instance_->check(entry);
  instance_->add(entry, new HandledOption<T>());
}

template<class T> void 
Configuration::addOption(const std::string& entry, T* value,
                         const std::string& description)
{
  instance_->check(entry);
  instance_->add(entry, new ReferenceOption<T>(value));
}

template <class T> void
Configuration::addCallback(const std::string& entry, Callback<T>* callback,
                           const std::string& description)
{
  instance_->check(entry);
  instance_->add(entry, new CallbackOption<T>(callback));
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
  LOG4CXX_DEBUG(logger, "setting entry " << entry << ", priority is " << prio);

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
