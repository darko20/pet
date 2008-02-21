#include "Configuration.h"

Config* Config::_instance = new Config();

GenericConverter<bool>* GenericConverter<bool>::_instance = NULL;

#if HAVE_LIBLOG4CXX
log4cxx::LoggerPtr Config::logger(log4cxx::Logger::getLogger("Config"));
#endif // HAVE_LIBLOG4CXX

ConfigException::ConfigException(std::string message)
  : tError(message) {}

bool Config::hasOption(const std::string& entry) {
  return (_instance->keyValue_.find(entry) != _instance->keyValue_.end());
}

bool Config::isValidName(const std::string& entry) const {
  return !entry.empty();
}

void Config::check(const std::string &name) {
  if(!isValidName(name)) {
    LOG(logger, log4cxx::Level::WARN,
        "isValidName(%s) == false", name.c_str());
    throw ConfigException(name + " is not a valid option name");
  }
  
  if(hasOption(name)) {
    LOG(logger, log4cxx::Level::WARN,
        "option \"%s\" already exists", name.c_str());
    throw ConfigException("option " + name + " already exists");
  }
}

void Config::add(const std::string &name, IOption* option) {
  check(name);
  keyValue_[name] = option;
}

IOption& Config::getIOption(const std::string& name) {
  KeyValueMap::iterator i = keyValue_.find(name);
  if(i == keyValue_.end())
    throw ConfigException("option " + name + " does not exist");
    
  return *(i->second);
}

const std::string& Config::getDescription(const std::string& name) {
  return _instance->getIOption(name).getDescription();
}

Config::~Config() {
  for(KeyValueMap::iterator i = keyValue_.begin(); i != keyValue_.end(); i++) {
    delete i->second;
  }
}

Config::Config() {}

void Config::setString(const std::string& entry, const std::string& value,
                       int prio) {
  try {
    IOption &o = _instance->getIOption(entry);
    o.setString(value, prio);
  }
  catch(std::bad_cast) {
    throw ConfigException("wrong type was used for option " + entry);
  }
}

std::string Config::getString(const std::string& entry) {
  try {
    IOption &o = _instance->getIOption(entry);
    return o.getString();
  }
  catch(std::bad_cast) {
    throw ConfigException("wrong type was used for option " + entry);
  }
}
