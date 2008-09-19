#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cassert>

#include "logging.h"

#if HAVE_LOG4CPP
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

#else // HAVE_LOG4CPP
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
