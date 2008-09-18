#include "config.h"

Config* Config::_instance = NULL;

//GenericConverter<bool>* GenericConverter<bool>::_instance = NULL;

ConfigException::ConfigException(std::string message)
  : tError(message) {}

template<> std::string default_tostring<const char *>(const char * const &val) {
  return ((std::string) val);
}

template<> const char * default_fromstring<const char *>(const std::string &s) {
  return s.c_str();
}

/*
void Config::add(const std::string &name, IOption* option) {
  if(!isValidName(name)) {
    throw ConfigException("`" + name + "' is not a valid option name");
  }
  
  if(hasOption(name)) { throw DuplicateOptionException(name); }
  keyValue_[name] = option;
}

IOption& Config::getIOption(const std::string& name) {
  KeyValueMap::iterator i = keyValue_.find(name);
  if(i == keyValue_.end())
    throw NoSuchOptionException(name);
    
  return *(i->second);
}
*/


/*
void Config::setString(const std::string& entry, const std::string& value,
                       int prio) {
  try {
    getIOption(entry).setString(value, prio);
  }
  catch(std::bad_cast) {
    throw WrongTypeException(entry);
  }
}

std::string Config::getString(const std::string& entry) {
  try {
    return getIOption(entry).getString();
  }
  catch(std::bad_cast) {
    throw WrongTypeException(entry);
  }
}
*/
