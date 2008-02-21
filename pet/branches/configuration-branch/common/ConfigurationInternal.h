/* -*- Mode: C++ -*- */
#ifndef _CONFIG_INTERNAL_H
#define _CONFIG_INTERNAL_H

#include "logging.h"

/** @file ConfigInternal.h
 * @brief Internal header for configuration subsystem - do not read ;)
 * 
 * We need this file because we use templates and do not want to put everything
 * in the file with interface of subsystem.
 */


/**
 * @brief Converts values to/from string.
 *
 * It uses C++ stream operators << and >> to convert the argument.
 *
 * @todo Add error handling.
 */
template<class T>
class GenericConverter : public AbstractConverter<T> {
public:
  virtual ~GenericConverter() {}

  virtual std::string toString(const T& t) {
    std::ostringstream os;
    os << t;
    return os.str();
  }
  virtual T fromString(const std::string& s) {
    std::istringstream is(s);
    T t;
    is >> t;
    return t;
  }

  /**
   * Get the only instance of class. Implements singleton.
   * @return the only instance of the class.
   */
  static AbstractConverter<T>* getInstance() {
    if(_instance == NULL)
      _instance = new GenericConverter();
    
    return _instance;
  }
  
private:
  GenericConverter() {};
  static GenericConverter* _instance;
};

/**
 * @brief Converts boolean values to/from string.
 * @todo Add error handling.
 */
template<> class GenericConverter<bool> : public AbstractConverter<bool> {
public:
  virtual ~GenericConverter() {}

  virtual std::string toString(const bool& val) {
    return val ? "true" : "false";
  }
  virtual bool fromString(const std::string& s) {
    return (s == "true" || s =="yes");
  }

  /**
   * Get the only instance of class. Implements singleton.
   * @return the only instance of the class.
   */
  static AbstractConverter<bool>* getInstance() {
    if(_instance == NULL)
      _instance = new GenericConverter();
    
    return _instance;
  }
  
private:
  GenericConverter() {};
  static GenericConverter* _instance;
};

template<class T> GenericConverter<T>* GenericConverter<T>::_instance = NULL;


/** @brief Root class for managing one option. */
class IOption {
public:
  /**
   * @param description a description of this option
   */
  IOption(const std::string& description)
    : _greatestPriority(0), _description(description) {}

  virtual ~IOption() {}
  
  const std::string& getDescription() { return _description; }

  virtual void setString(const std::string &value, int priority) = 0;

  virtual std::string getString() = 0;

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
    if (priority >= _greatestPriority) {
      _greatestPriority = priority;
      return true;
    }
    return false;
  }

private:   
  int _greatestPriority; //greatest value used in set()
  const std::string _description;
};


/**
 * @brief Abstract class for managing one option of given type.
 */
template<class T> class Option : public IOption {
public:
  /**
   * @param description a description of this option
   */
  Option(const std::string& description)
    : IOption(description)
    , _converter(GenericConverter<T>::getInstance()) { }

  /**
   * @param description a description of this option
   */
  Option(const std::string& description, AbstractConverter<T> *converter)
    : IOption(description), _converter(converter) { }
  
  virtual ~Option() {}

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

  /** Returns value of the option. */
  virtual T& get() = 0;

  /**
   * Set a value of this option using string argument.
   * Uses custom converter, if one is specified.
   * @param s string containing new value of the option.
   * @param priority see #Option<T>::set
   */
  virtual void setString(const std::string& s, int priority = 1) {
    if (_converter == NULL) {
      LOG(logger, log4cxx::Level::WARN, 
          "no string converter for option %s", description().c_str());
    }
    this->set(_converter->fromString(s), priority);
  }

  /**
   * Get a value of the option as string.
   * Uses custom converter, if one is specified.
   * @return the value of the option as std::string
   */
  virtual std::string getString() {
    if (_converter == NULL) {
      LOG(logger, log4cxx::Level::WARN, 
          "no string converter for option %s", description().c_str());
    }
    return _converter->toString(this->get());
  }

private:
  AbstractConverter<T> *_converter;
};

/**
 * @brief  Concrete classes for managing options.
 * 
 * Object of this class stores the value of an option internally, as a field.
 */
template<class T>
class HandledOption : public Option<T> {
public:
  /** Create a new option managed completely by the Config system
   * @param description a description of this option
   * @param initial the initial value of this option
   */
  HandledOption(const std::string& description, const T& initial)
    : Option<T>(description), value_(initial) {}
  
  /** Create a new option managed completely by the Config system
   * @param description a description of this option
   * @param initial the initial value of this option
   * @param converter a custom string converter for this option
   */
  HandledOption(const std::string& description, const T& initial,
                AbstractConverter<T>* converter)
    : Option<T>(description, converter), value_(initial) {}
  
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
   * If object is created with this constructor, the value is stored in an
   * external place and only a reference is stored here.
   * @param description a description of this option
   * @param valueRef Reference to the variable that stores the value
   * @param converter a custom string converter for this option
   */
  ReferenceOption(const std::string& description, T& valueRef,
                  AbstractConverter<T>* converter)
    : Option<T>(description, converter), value_(valueRef) {}

  /**
   * If object is created with this constructor, the value is stored in an
   * external place and only a reference is stored here.
   * @param description a description of this option
   * @param valueRef Reference to the variable that stores the value
   */
  ReferenceOption(const std::string& description, T& valueRef)
    : Option<T>(description), value_(valueRef) {}

  virtual ~ReferenceOption() {}

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
  ReferenceOption() {}

  //we always acccess data using value_, but it can point to a object whose
  //memory is handled by the \c Option
  T& value_;
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
  CallbackOption(const std::string& description, Callback<T>* callback)
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

#endif // _CONFIG_INTERNAL_H
