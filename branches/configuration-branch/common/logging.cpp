#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cassert>

#include "logging.h"

#if HAVE_LOG4CPP
#include "utility.h"
#include "log4cpp/PropertyConfigurator.hh"
#include "log4cpp/OstreamAppender.hh"
#include "log4cpp/PatternLayout.hh"
#include "log4cpp/SimpleLayout.hh"

namespace log4cpp {
  class EmptyLayout : public Layout {
    virtual ~EmptyLayout() {}
    virtual std::string format(const LoggingEvent &event) {
      return event.message;
    }
  };
};

Category &root = log4cpp::Category::getRoot(),
  &logAppl = log4cpp::Category::getInstance(std::string("logAppl")),
  &logApplC = log4cpp::Category::getInstance(std::string("logApplC")),
  &logGenerics = log4cpp::Category::getInstance(std::string("logGenerics")),
  &logGrammar = log4cpp::Category::getInstance(std::string("logGrammar")),
  &logLexproc = log4cpp::Category::getInstance(std::string("logLexproc")),
  &logMorph = log4cpp::Category::getInstance(std::string("logMorph")),
  &logPack = log4cpp::Category::getInstance(std::string("logPack")),
  &logParse = log4cpp::Category::getInstance(std::string("logParse")),
  &logSM = log4cpp::Category::getInstance(std::string("logSM")),
  &logSemantic = log4cpp::Category::getInstance(std::string("logSemantic")),
  &logSyntax = log4cpp::Category::getInstance(std::string("logSyntax")),
  &logTsdb = log4cpp::Category::getInstance(std::string("logTsdb")),
  &logUnpack = log4cpp::Category::getInstance(std::string("logUnpack")),
  &logXML = log4cpp::Category::getInstance(std::string("logXML"));

void init_logging(const std::string &base_dir) {
  try {
    std::string initFileName 
      = find_set_file("logging", ".properties", base_dir);
    if (!initFileName.empty()) {
      log4cpp::PropertyConfigurator::configure(initFileName);
      return;
    }
  } catch(log4cpp::ConfigureFailure& f) {
  }
  // use default configuration

  // don't now hot to suppress the CR in this logger
  //nocrapp = new log4cpp::OstreamAppender("nocr", &std::cerr);
  //nocrapp->setLayout(new log4cpp::EmptyLayout());
  /*
  Category *cats[] = {
    &root, &logAppl, &logApplC, &logGenerics, &logGrammar, &logLexproc,
    &logMorph,
    &logPack, &logParse, &logSM, &logSemantic, &logSyntax, &logTsdb,
    &logUnpack, &logXML
  };
  */
  log4cpp::Appender *plainapp
    = new log4cpp::OstreamAppender("default", &std::cerr);
  plainapp->setLayout(new log4cpp::SimpleLayout());
  root.setAppender(plainapp); root.setPriority(WARN);

  Category *appls[] = { &logAppl, &logApplC, NULL };
  for (int i = 0; NULL != appls[i]; ++i) {
    appls[i]->setPriority(INFO); appls[i]->setAdditivity(false);
    log4cpp::Appender *emptyapp
      = new log4cpp::OstreamAppender("empty", &std::cerr);
    emptyapp->setLayout(new log4cpp::EmptyLayout());
    appls[i]->setAppender(emptyapp);
  }

  log4cpp::Appender *emptyapp
    = new log4cpp::OstreamAppender("empty", &std::cerr);
  emptyapp->setLayout(new log4cpp::EmptyLayout());
  logSyntax.setAdditivity(false); logSyntax.setAppender(emptyapp);
}

void shutdown_logging() {
  log4cpp::Category::shutdown();
}

#else // NOT HAVE_LOG4CPP ****************************************

#include <iostream>
#include <iomanip>

Logger::loggerendl Logger::_e;
 
Category   root(WARN, 1),
  logAppl(INFO, 2),
  logApplC(INFO, 3),
  logGenerics(NOTSET, 0),
  logGrammar(NOTSET, 0),
  logLexproc(NOTSET, 0),
  logMorph(NOTSET, 0),
  logPack(NOTSET, 0),
  logParse(NOTSET, 0),
  logSM(NOTSET, 0),
  logSemantic(NOTSET, 0),
  logSyntax(NOTSET, 2),
  logTsdb(NOTSET, 0),
  logUnpack(NOTSET, 0),
  logXML(NOTSET, 2);

std::string prio_names[] = {
  "fatal", "alert", "critical", "error",
  "warning", "notice", "info", "debug", ""
};

std::ostream &
Logger::print(const Category &cat, Priority prio) {
  // would be possible to create a new ostringstream here and decide in endl
  // where to print it (more flexibility in the output)
  switch (cat._printer) {
  case 1:
    return std::cerr << std::endl << std::setw(10) << clock() << " "
                     << prio_names[prio/100] << ": " ;
  case 2:
    return std::cerr << std::endl;
  case 3:
    return std::cerr;
  }
  return std::cerr;
}

void 
Logger::loggerendl::print(std::ostream &out) const {
  /*
  switch (_cat->_printer || root->_printer) {
  case 1:
    break;
  case 2:
    break;
  case 3:
    break;
  }
  */
}
#endif
