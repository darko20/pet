#ifndef _CONFIGURATION_INTERNAL_H
#define _CONFIGURATION_INTERNAL_H

#include "logging.h"

/** @file ConfigurationInternal.h
 * @brief Internal header for configuration subsystem - do not read ;)
 * 
 * We need this file because we use templates and do not want to put everything
 * in the file with interface of subsystem.
 */

/** @brief Root class for managing one option. */
class IOption {
public:
  /**
   * @param description a description of this option
   */
  IOption(const std::string& description)
    : greatestPriority_(0), description_(description) {}

  virtual ~IOption() {}
  
  const std::string& getDescription() {
    return description_;
  }

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
  const std::string& description_;
};

/**
 * @brief Abstract class for managing one option of given type.
 */
template<class T> class Option : public IOption {
public:
  /**
   * @param description a description of this option
   */
  Option(const std::string& description) : IOption(description) {}

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
  /**
   * @param description a description of this option
   */
  HandledOption(const std::string& description) : Option<T>(description) {}
  
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
   * @param description a description of this option
   */
  ReferenceOption(T* valueRef, const std::string& description)
    : Option<T>(description), value_(valueRef) {}

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

template<class T> class Callback;

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
   * @param description a description of this option
   */
  CallbackOption(Callback<T>* callback, const std::string& description)
    : Option<T>(description), callback_(*callback) {}

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

#endif // _CONFIGURATION_INTERNAL_H
