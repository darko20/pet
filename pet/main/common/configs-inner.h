/* -*- Mode: C++ -*- */
#ifndef _CONFIG_INTERNAL_H
#define _CONFIG_INTERNAL_H

#include <boost/lexical_cast.hpp>
#include <cassert>
#include <iostream> 

/** \file ConfigInternal.h
 * \brief Internal header for configuration subsystem - do not read ;)
 * 
 * We need this file because we use templates and do not want to put everything
 * in the file with interface of subsystem.
 */

template<class T> std::string default_tostring(const T &val) {
  return boost::lexical_cast<std::string, T>(val);
}

template<class T> T default_fromstring(const std::string &s) {
  return boost::lexical_cast<T, std::string>(s);
}

template<> std::string default_tostring<const char *>(const char * const &val);

template<> const char * default_fromstring<const char *>(const std::string &s);

template<class T> class value {
public:
  typedef T (*fromfunc)(const std::string &s);
  typedef std::string (*tofunc)(const T &val);

  value(fromfunc from= default_fromstring<T>, tofunc to=default_tostring<T>) 
    : _fromstring(from), _tostring(to), _conv(NULL) { }

  // the destruction of value will destroy the converter object
  value(AbstractConverter<T> *conv) 
    : _fromstring(NULL), _tostring(NULL), _conv(conv) { }

  virtual ~value() {};

  /** Return the stored value */
  virtual const T& get() = 0;

  /** Set the value to \a val */
  virtual void set(const T& val) = 0;

  /** Returns value of the option as a string which is acceptable in
   *  \see set_from_string
   */
  virtual std::string get_as_string() {
    return (_conv == NULL) ? _tostring(get()) : _conv->toString(get());
  }

  /** Set the value of this option from a readable represenation.
   * \param s string containing new value of the option.
   * \param priority see #option<T>::set
   */
  virtual void set_from_string(const std::string& s) {
    set((_conv == NULL) ? _fromstring(s) : _conv->fromString(s));
  }

private: 
  fromfunc _fromstring;
  tofunc _tostring;
  AbstractConverter<T> *_conv;
};

/** Template classes for managed values.
 * 
 * Object of this class stores the value of an option internally, as a field.
 */
template<class T> class handled_value : public value<T> {
  typedef typename value<T>::fromfunc fromfunc;
  typedef typename value<T>::tofunc tofunc;

public:
  /** Create a new value object managed completely by the Config system
   * \param initial the initial value
   */
  handled_value(const T& initial) : _value(initial) {}
  
  handled_value(const T& initial, fromfunc f, tofunc t)
    : value<T>(f,t), _value(initial) {}
  
  handled_value(const T& initial, AbstractConverter<T> *c)
    : value<T>(c), _value(initial) {}
  
  virtual ~handled_value() {}

  /** Returns stored value */
  virtual const T& get() { return _value; }

  /** Set the value to \a val */
  virtual void set(const T& val) { _value = val; }
  
private:
  handled_value() {}

  T _value;
};

/** value objects of this class store a pointer to the location of the real
 *  variable
 */
template<class T> class reference_value : public value<T> {
public:
  /** The value is stored in an external place and only the pointer to this
   * place is stored here.
   * \param valueRef Reference to the variable that stores the value
   */
  reference_value(T& valueRef)
    : _value_ptr(&valueRef) {}

  virtual ~reference_value() {}

  /** Returns value of the option. */
  virtual const T& get() { return *_value_ptr; }

  /** Set value to \a val */
  virtual void set(const T& val) { *_value_ptr = val; }

private:
  reference_value() {}

  T* _value_ptr;
};


/** value stored via callback (set and get functions) of given type. */
template<class T>
class callback_value : public value<T> {
public:
  /**
   * If object is created with this constructor, sets the value via the
   * callback
   * \param callback Function-object which is called every time value of value
   *                 is changed
   * \param description a description of this value
   */
  callback_value(Callback<T> *callback) : _callback(callback) {}

  virtual ~callback_value() { delete _callback; }

  /** Returns the stored value. */
  virtual const T& get() { return _callback->get(); }

  /** Set value to \a val */
  virtual void set(const T &val) { _callback->set(val); }

private:
  Callback<T> *_callback;
};


/** \brief Stores configuration entries.
 */
class Config {

  class abstract_opt {
  public:
    abstract_opt(const std::string &desc) : _description(desc) {}
    virtual ~abstract_opt() {}
    
    const std::string& description() { return _description; }

    /** Returns value of the option as a string which is acceptable in
     *  \see set_from_string
     */
    virtual std::string get_as_string() = 0;

    /** Set the value of this option from a readable represenation.
     * \param s string containing new value of the option.
     * \param priority see #option<T>::set
     */
    virtual void set_from_string(const std::string& s, int priority = 1) = 0;

  protected:
    const std::string _description;
  };
      

  /** class for managing an option of the given type. */
  template<class T> class option : public abstract_opt {
  public:
    /**
     * \param description a description of this option
     */
    option(const std::string& description, value<T> *val)
      : abstract_opt(description), _max_priority(0), _value(val)
        //, _converter(NULL)
    { }

    /**
     * \param description a description of this option
     *
     option(const std::string& description, AbstractConverter<T> *converter)
     : _max_priority(0), _description(description), _converter(converter) { }
    */

    virtual ~option() { delete _value; }

    /** Returns value of the option. */
    const T& get() { return _value->get(); }

    /** Set value of the option.
     * \param value New value.
     * \param priority the assignment of the new value to the option only takes
     *       place if \a priority is equal or grater than the maximum priority of
     *       previous calls to set.
     */
    void set(const T& value, int priority = 1) {
      if (priority >= _max_priority) {
        _max_priority = priority;
        _value->set(value);
      }
    }

    /** Returns value of the option as a string which is acceptable in
     *  \see set_from_string
     */
    virtual std::string get_as_string() {
      return _value->get_as_string();
    }

    /** Set the value of this option from a readable represenation.
     * \param s string containing new value of the option.
     * \param priority see #option<T>::set
     */
    virtual void set_from_string(const std::string& s, int priority = 1) {
      if (priority >= _max_priority) {
        _max_priority = priority;
        _value->set_from_string(s);
      }
    }

  private:
    int _max_priority;               // max priority used in set()
    value<T> *_value;
  };


private:
  template <class T> option<T> &get_option_checked(const std::string& key) {

    KeyValueMap::iterator i = _keyValue.find(key);
    if (i == _keyValue.end())
      throw NoSuchOptionException(key);
    try {
      option<T> &result = dynamic_cast< option<T> &>( *i->second );
      return result;
    }
    catch(std::bad_cast) {
      std::cerr << key << std::endl;
      throw WrongTypeException(key);
    }
  }

  abstract_opt *get_option(const std::string& key) {
    KeyValueMap::iterator i = _keyValue.find(key);
    if (i == _keyValue.end())
      throw NoSuchOptionException(key);
    return i->second;
  }

  /** Checks if string can be used as a name for an configuration option.
   * @param key String to be checked.
   * @return true iff key can be name of an option.
   */
  inline bool isValidName(const std::string& key) const {
    return !key.empty();
  }

  // Adds a new option to current configuration.
  inline bool check(const std::string &key) {
    if(!isValidName(key)) {
      throw ConfigException("`" + key + "' is not a valid option name");
    }
    
    if(hasOption(key)) { throw DuplicateOptionException(key); }
    return true;
  }

  inline void add(const std::string &key, abstract_opt* opt) {
    _keyValue[key] = opt;
  }
                                                            
  Config() {}

public:
  ~Config() {
    for(KeyValueMap::iterator i = _keyValue.begin();
        i != _keyValue.end(); ++i) {
      delete i->second;
    }
  }

  /** Adds new option where the value is handled internally
   * \param key Name of the option.
   * \param description
   * \param initial initial value of the option.
   * \param converter converter used for conversion to/from
   *                  std::string of this option
   *
   * \throw ConfigException Thrown if option already exists or name is
   *                            empty
   */
  template<class T> void 
  addOption(const std::string& key, const std::string& description,
            const T &initial) {
    if(check(key))  // we don't create the option if the key is not OK
      add(key, new option<T>(description, new handled_value<T>(initial)));
  }

  template<class T> void 
  addOption(const std::string& key, const std::string& description,
            const T &initial, AbstractConverter<T> *converter) {
    add(key,
        new option<T>(description, new handled_value<T>(initial, converter)));
  }
  
  template<class T> void 
  addOption(const std::string& key, const std::string& description,
            const T &initial,
            typename value<T>::fromfunc from, typename value<T>::tofunc to) {
    if(check(key))  // we don't create the option if the key is not OK
      add(key, 
          new option<T>(description, new handled_value<T>(initial, from, to)));
  }
  
  /** Adds new configuration option. Value is stored in given location.
   * \param key Name of the option.
   * \param description
   * \param place Reference to the place where the value is stored.
   * \param converter converter used for conversion to/from
   *                  std::string of this option
   *
   * \throw ConfigException Thrown if option already exists or name is
   *                            empty.
   *
  template<class T> void 
  addReference(const std::string& key, const std::string& description,
               T &place, AbstractConverter<T> *converter) {
    add(key, new option<T>(description, new reference_value<T>(place)
                           converter));
  }
  */

  template<class T> void 
  addReference(const std::string& key, const std::string& description,
               T &place) {
    if(check(key))  // we don't create the option if the key is not OK
      add(key, new option<T>(description, new reference_value<T>(place)));
  }


  /** Adds new callback configuration option, which provides the accessors.
   * @param key       Name of the option.
   * @param callback    Function-object which is called every time value of
   *                    option is changed
   * @param description 
   *
   * @attention         The callback object must be a heap allocated object. It
   *                    is destroyed when the option is destroyed.
   * @exception ConfigException Thrown if option already exists or name is
   *                            empty.
   */
  template <class T> void
  addCallback(const std::string& key, const std::string& description,
              Callback<T> *callback) {
    if(check(key))  // we don't create the option if the key is not OK
      add(key, new option<T>(description, new callback_value<T>(callback)));
  }

  /** Gets value of configuration option, if it exists.
   * @param key Name of the option.
   *
   * @return reference of type T: the value.
   *
   * @exception ConfigException Thrown if option does not exist.
   *                            Also if the option's type is different from T.
   */
  template <class T> const T& get(const std::string& key) {
    return get_option_checked<T>(key).get();
  };
  
  /** Works exactly like #get(const std::string& key), but result is passed
   * as an "out" parameter. Throws the same exceptions.
   * @param key
   * @param place Here goes the result.
   * @exception ConfigException Thrown if option does not exist.
   *                            Also if the option's type is different from T.
   */
  template <class T> void get(const std::string& key, T& place) {
    place = this->get<T>(key);
  }

  /** Sets the value of an existing configuration option.
   * @param key Name of the option.
   * @param value New value.
   * @param prio  Priority of current invocation. Works like #option<T>::set.
   *
   * @exception ConfigException Thrown if option does not exist.
   *                            Also if the option's type is different from T.
   */
  template <class T> void 
  set(const std::string& key, const T& value, int prio = 1) {
    get_option_checked<T>(key).set(value, prio);
  }

  /** Get the value of an option as string.
   * Works like #get(const std::string& key), throws the same exceptions
   */
  std::string get_as_string(const std::string& key) {
    return get_option(key)->get_as_string();
  }

  /** Sets the value of an existing configuration option from a string.
   * Works like #set, throws the same exceptions.
   */
  void set_from_string(const std::string& key, const std::string& value,
                       int prio = 1) {
    try {
      get_option(key)->set_from_string(value, prio);
    }
    catch (boost::bad_lexical_cast b) {
      throw ConversionException(key, value, "def");
    }
    catch (ConversionException c) {
      // this comes from a custom converter, which should provide its name in
      // the message
      throw ConversionException(key, value, c.getMessage());
    }
  }

  /** Checks if given name exists in configuration database.
   * @param key Name of the parameter.
   * @return true iff key is name of existing option.
   */
  inline bool hasOption(const std::string& key) {
    return (_keyValue.find(key) != _keyValue.end());
  }

  /** Retrieves description of an option.
   * @param key Name of the option.
   * @return description of the option.
   *
   * @exception ConfigException Thrown if option does not exist.
   */
  inline const std::string& description(const std::string& key) {
    return get_option(key)->description();
  }

  /** Get the singleton instance, or create it if not yet available */
  inline static Config *get_instance() { 
    if (_instance) return _instance;
    return (_instance = new Config());
  }

  static void finalize() { delete _instance; }

private:

  static Config *_instance;
  typedef std::map<const std::string, abstract_opt*> KeyValueMap;
  KeyValueMap _keyValue;
};


#endif // _CONFIG_INTERNAL_H
  /* Don't access the values directly. Use reference options if you want to.

  ** Works exactly like #get(const std::string& key), but result is passed
   * as an "out pointer" parameter. Throws the same exceptions.
   * @param key
   * @param place Here goes the result.
   * @exception ConfigException Thrown if option does not exist.
   *                            Also if the option's type is different from T.
   *
  template<class T> void getPointer(const std::string& key, (T*)& place);

  ** Works exactly like #get(const std::string& key), but result is passed
   * as an "out pointer" parameter. Throws the same exceptions.
   * @param key
   * @param place Here goes the result.
   * @exception ConfigException Thrown if option does not exist.
   *                            Also if the option's type is different from T.
   *
  template<class T> T* getPointer(const std::string& key);
  */
