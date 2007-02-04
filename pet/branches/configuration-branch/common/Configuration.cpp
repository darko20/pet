#include "Configuration.h"

Configuration* Configuration::instance_ = new Configuration();

#if HAVE_LIBLOG4CXX
  log4cxx::LoggerPtr Configuration::logger(
                       log4cxx::Logger::getLogger("Configuration"));
#endif // HAVE_LIBLOG4CXX

ConfigurationException::ConfigurationException(std::string message)
  : tError(message) {}


NoSuchEntryException::NoSuchEntryException(std::string message)
  : ConfigurationException(message) {}

WrongTypeException::WrongTypeException(std::string message)
  : ConfigurationException(message) {}

WrongEntryNameException::WrongEntryNameException(std::string message)
  : ConfigurationException(message) {}

EntryAlreadyExistsException::EntryAlreadyExistsException(std::string message)
  : ConfigurationException(message) {}

ConvertionException::ConvertionException(std::string message)
  : ConfigurationException(message) {}

NoConverterException::NoConverterException(std::string message)
  : ConfigurationException(message) {}

bool Configuration::hasOption(const std::string& entry) {
  return (instance_->keyValue_.find(entry) != instance_->keyValue_.end());
}

bool Configuration::isValidName(const std::string& entry) const {
    return !entry.empty();
}

void Configuration::check(const std::string &entry) {
  if(!isValidName(entry)) {
    LOG4CXX_WARN(logger, "isValidName(" + entry + ") == false");
    throw WrongEntryNameException(entry + " is not a valid entry name");
  }
  
  if(hasOption(entry)) {
    LOG4CXX_WARN(logger, "entry \"" + entry + "\" already exists");
    throw EntryAlreadyExistsException("entry " + entry + " already exists");
  }
}

void Configuration::add(const std::string &entry, IOption* option) {
  keyValue_[entry] = option;
}

const std::string& Configuration::getDescription(const std::string& entry) {
  if(!instance_->isValidName(entry))
    throw WrongEntryNameException(entry + " is not a valid entry name");
    
  KeyValueMap::iterator i = instance_->keyValue_.find(entry);

  if(i == instance_->keyValue_.end())
    throw NoSuchEntryException("entry " + entry + " does not exist");
    
  return (*(i->second)).getDescription();
}

IOption& Configuration::getIOption(const std::string& entry) {
if(!instance_->isValidName(entry))
    throw WrongEntryNameException(entry + " is not a valid entry name");
    
  KeyValueMap::iterator i = instance_->keyValue_.find(entry);

  if(i == instance_->keyValue_.end())
    throw NoSuchEntryException("entry " + entry + " does not exist");
    
  return *(i->second);
}

Configuration::~Configuration() {
  for(KeyValueMap::iterator i = keyValue_.begin(); i != keyValue_.end(); i++) {
    delete i->second;
  }
}

Configuration::Configuration() {}
