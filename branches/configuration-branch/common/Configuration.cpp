#include "Configuration.h"

Configuration* Configuration::instance_ = new Configuration();

#if HAVE_LIBLOG4CXX
  log4cxx::LoggerPtr Configuration::logger(
                       log4cxx::Logger::getLogger("Configuration"));
#endif // HAVE_LIBLOG4CXX

bool Configuration::hasOption(const std::string& entry) {
  return (instance_->keyValue_.find(entry) != instance_->keyValue_.end());
}

bool Configuration::isValidName(const std::string& entry) const {
    return !entry.empty();
}

void Configuration::check(const std::string &entry) {
  if(!isValidName(entry)) {
    LOG4CXX_WARN(logger, "isValidName(" + entry + ") == false");
    throw WrongEntryNameException();
  }
  
  if(hasOption(entry)) {
    LOG4CXX_WARN(logger, "entry \"" + entry + "\" already exists");
    throw EntryAlreadyExistsException();
  }
}

void Configuration::add(const std::string &entry, IOption* option) {
  keyValue_[entry] = option;
}

const std::string& Configuration::getDescription(const std::string& entry) {
  if(!instance_->isValidName(entry))
    throw WrongEntryNameException();
    
  KeyValueMap::iterator i = instance_->keyValue_.find(entry);

  if(i == instance_->keyValue_.end())
    throw NoSuchEntryException();
    
  return (*(i->second)).getDescription();
}

Configuration::~Configuration() {
  for(KeyValueMap::iterator i = keyValue_.begin(); i != keyValue_.end(); i++) {
    delete i->second;
  }
}

Configuration::Configuration() {}
