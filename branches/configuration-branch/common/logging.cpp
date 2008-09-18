#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cassert>

#include "logging.h"

#if HAVE_LIBLOG4CPP

#else // HAVE_LIBLOG4CPP
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
  logXML(NOTSET, 0);

std::string prio_names[] = {
  "fatal", "alert", "critical", "error",
  "warning", "notice", "info", "debug", ""
};

std::ostream &
Logger::print(const Category &cat, PriorityLevel prio) {
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
