#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cassert>

#include "logging.h"

/* ###################################################################### */
/* ###################################################################### */

#if HAVE_LOG4CPP
#include "utility.h"
#include "log4cpp/PropertyConfigurator.hh"
#include "log4cpp/OstreamAppender.hh"
#include "log4cpp/PatternLayout.hh"
#include "log4cpp/SimpleLayout.hh"
#include <log4cpp/LayoutsFactory.hh>
#include <log4cpp/FactoryParams.hh>

namespace log4cpp {
  class SimplestLayout : public Layout {
    virtual ~SimplestLayout() {}
    virtual std::string format(const LoggingEvent &event) {
      return event.message + '\n';
    }
  };
  class EmptyLayout : public Layout {
    virtual ~EmptyLayout() {}
    virtual std::string format(const LoggingEvent &event) {
      return event.message;
    }
  };

  std::auto_ptr<Layout> create_simplest_layout(const FactoryParams& params) {
    return std::auto_ptr<Layout>(new SimplestLayout);
  }
  std::auto_ptr<Layout> create_empty_layout(const FactoryParams& params) {
    return std::auto_ptr<Layout>(new EmptyLayout);
  }
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
  // nice idea, but the layouts are not registered in the PropertyConfigurator
  // anyway.
  log4cpp::LayoutsFactory::getInstance().
    registerCreator("simplest", log4cpp::create_simplest_layout);
  log4cpp::LayoutsFactory::getInstance().
    registerCreator("org.apache.log4j.EmptyLayout",
                    log4cpp::create_empty_layout);

  try {
    std::string initFileName 
      = find_set_file("logging", ".properties", base_dir);
    if (!initFileName.empty()) {
      log4cpp::PropertyConfigurator::configure(initFileName);
      return;
    }
  } catch(log4cpp::ConfigureFailure& f) {
    std::cerr << f.what() << std::endl;
  }

  // use the default configuration 
  root.setPriority(WARN);
  log4cpp::Appender *app
    = new log4cpp::OstreamAppender("default", &std::cerr);
  app->setLayout(new log4cpp::SimpleLayout());
  root.setAppender(app);

  logAppl.setPriority(INFO); logAppl.setAdditivity(false);
  app = new log4cpp::OstreamAppender("simplest", &std::cerr);
  app->setLayout(new log4cpp::SimplestLayout());
  logAppl.setAppender(app);

  logApplC.setPriority(INFO); logApplC.setAdditivity(false);
  app = new log4cpp::OstreamAppender("empty", &std::cerr);
  app->setLayout(new log4cpp::EmptyLayout());
  logApplC.setAppender(app);

  app = new log4cpp::OstreamAppender("emptySyn", &std::cerr);
  app->setLayout(new log4cpp::EmptyLayout());
  logSyntax.setAdditivity(false); logSyntax.setAppender(app);
}

void shutdown_logging() {
  log4cpp::Category::shutdown();
}

/* ###################################################################### */
#else /* NOT HAVE_LOG4CPP ############################################### */
/* ###################################################################### */

#include "utility.h"

#include <iostream>
#include <fstream>
#include <iomanip>

using namespace std;

map<string, Category> Logger::_cats;
Logger::loggerendl Logger::_e;
 
Category &root = Logger::addCat("rootCategory", WARN, 1),
  &logAppl = Logger::addCat("logAppl", INFO, 2),
  &logApplC = Logger::addCat("logApplC", INFO, 3),
  &logChart = Logger::addCat("logChart", NOTSET, 0),
  &logChartMapping = Logger::addCat("logChartMapping", NOTSET, 0),
  &logGenerics = Logger::addCat("logGenerics", NOTSET, 0),
  &logGrammar = Logger::addCat("logGrammar", NOTSET, 0),
  &logLexproc = Logger::addCat("logLexproc", NOTSET, 0),
  &logMorph = Logger::addCat("logMorph", NOTSET, 0),
  &logPack = Logger::addCat("logPack", NOTSET, 0),
  &logParse = Logger::addCat("logParse", NOTSET, 0),
  &logSM = Logger::addCat("logSM", NOTSET, 0),
  &logSemantic = Logger::addCat("logSemantic", NOTSET, 0),
  &logSyntax = Logger::addCat("logSyntax", NOTSET, 2),
  &logTsdb = Logger::addCat("logTsdb", NOTSET, 0),
  &logUnpack = Logger::addCat("logUnpack", NOTSET, 0),
  &logXML = Logger::addCat("logXML", NOTSET, 2);

std::string prio_names[] = {
  "FATAL", "ALERT", "CRIT", "ERROR",
  "WARN", "NOTICE", "INFO", "DEBUG", "NOTSET"
};

Priority prios[] = {
  FATAL, ALERT, CRIT, ERROR, WARN, NOTICE, INFO, DEBUG, NOTSET
};

std::string printer_names[] = {
  "simple", "simplest", "empty"
};

std::ostream &
Logger::print(const Category &cat, Priority prio) {
  // would be possible to create a new ostringstream here and decide in endl
  // where to print it (more flexibility in the output)
  switch (cat.printer()) {
  case 1:
    return std::cerr << std::setw(10) << clock() << " "
                     << prio_names[prio/100] << ": ";
  case 2:
    return std::cerr;
  case 3:
    return std::cerr;
  }
  return std::cerr;
}

void 
Logger::loggerendl::print(std::ostream &out) const {
  switch (_cat->printer()) {
  case 1: out << std::endl;
    break;
  case 2: out << std::endl;
    break;
  case 3:
    break;
  }
}

Priority getPrio(const std::string prio) {
  if (prio.empty()) return NOTSET;
  if (prio == "EMERG") return EMERG;
  for (unsigned int i = 0; i <= sizeof(prio_names) / sizeof(std::string); ++i) {
    if (prio == prio_names[i]) return prios[i];
  }
  return ILLEGAL;
}

int getPrinter(const std::string printer) {
  for (int i = 0 ; i <= 2; ++i) {
    if (printer == printer_names[i]) return i+1;
  }
  return -1;
}

class PropertyStream {
private:
  int _line;
  std::ifstream _in;

public:
  PropertyStream(std::string filename) : _line(1), _in(filename.c_str()) {}

  bool good() { return _in.good(); }

  int lineno() const { return _line; }

  char eatws() {
    char c;
    while(_in.good()) {
      _in.get(c);
      if (c == '\n') { ++_line; return c; }
      if (! isspace(c)) return c;
    }
    return '\0';
  }

  // read from stream until newline has been read or stream is at EOF 
  inline void eatline() {
    char c;
    while(_in.good()) {
      _in.get(c);
      if (c == '\n') { ++_line; return; }
    }
  }

  // eat all lines starting with a hash sign
  void eatcomment() {
    while(_in.good()) 
      if (_in.peek() == '#') eatline();
      else return;
  }

  bool extract(string &result, char delim) {
    char c;
    while(_in.good()) {
      _in.get(c);
      if (c == '\n') { ++_line; return true; }
      if (c == delim) return false;
      if (! isspace(c)) result.push_back(c);
    }
    return true;
  }
};

void init_logging(const std::string &base_dir) {
  std::string initFileName 
    = find_set_file("simplelog", ".properties", base_dir);
  if (!initFileName.empty()) {
    PropertyStream ps(initFileName);
    while (ps.good()) {
      ps.eatcomment();
      string cat_name;
      if (ps.extract(cat_name, '=')) {
        // end of line reached: no '=' 
        if (! cat_name.empty())
          // line no - 1 'cause the reader's already in the next line
          cerr << initFileName << ":" << ps.lineno() - 1
               << ": error: illegal line" << endl;
        continue;
      }
      if (! Logger::hasCat(cat_name)) {
        cerr << initFileName << ":" << ps.lineno() 
             << ": warning: unknown category: " << cat_name << endl;
        ps.eatline();
        continue;
      }
      string level, printer;
      if (! ps.extract(level, ',')) {
        if (! ps.extract(printer, '.')) ps.eatline();
      } else {
        if (level.empty())
          // line no - 1 'cause the reader's already in the next line
          cerr << initFileName << ":" << ps.lineno() - 1
               << ": error: illegal line" << endl;
      }
      Category &cat = Logger::getCat(cat_name);
      Priority prio = getPrio(level);
      if (prio == ILLEGAL) {
        cerr << initFileName << ":" << ps.lineno() 
             << ": warning: unknown priority: " << level << endl;
      }
      int pri = getPrinter(printer);
      if (pri == -1) {
        if (!printer.empty())
          cerr << initFileName << ":" << ps.lineno() 
               << ": warning: unknown printer: " << printer << endl;
        cat.set(prio);
      }
      else cat.set(prio, pri);
    }
    return;
  }
}
#endif
