/** -*- Mode: C++ -*-
 *  @file logging.h
 *  @brief PET's utilities for log4cxx.
 */

#ifndef _LOGGING_H
#define _LOGGING_H

#include "pet-config.h"

#ifdef HAVE_LOG4CPP
#include <log4cpp/Category.hh>
#include <log4cpp/Priority.hh>
using log4cpp::Category;

#define EMERG  log4cpp::Priority::EMERG
#define FATAL  log4cpp::Priority::FATAL
#define ALERT  log4cpp::Priority::ALERT
#define CRIT   log4cpp::Priority::CRIT
#define ERROR  log4cpp::Priority::ERROR
#define WARN   log4cpp::Priority::WARN
#define NOTICE log4cpp::Priority::NOTICE
#define INFO   log4cpp::Priority::INFO
#define DEBUG  log4cpp::Priority::DEBUG
#define NOTSET log4cpp::Priority::NOTSET

inline std::ostream &get_stringstream() { return *(new std::ostringstream()); }

#define LOG_ENABLED(__C, __P) (__C.isPriorityEnabled(__P))
#define LOG(__C, __P, __M) { if LOG_ENABLED(__C, __P) { \
        std::ostringstream ___o; ___o << __M; __C << __P << ___o.str(); } }

//inline std::ostringstream &operator<<(std::ostringstream &os, const std::string &s){
//  os << s.c_str(); return os;
//}

extern class log4cpp::Category
  &logAppl,
  &logApplC,
  &logChart,
  &logChartMapping,
  &logGenerics,
  &logGrammar,
  &logLexproc,
  &logMorph,
  &logMRS,
  &logPack,
  &logParse,
  &logSM,
  &logChartPruning,
  &logSemantic,
  &logSyntax,
  &logTsdb,
  &logUnpack,
  &logXML,
  &root;

void init_logging(const std::string &base_dir);
void shutdown_logging();

#else // HAVE_LOG4CPP
#include "errors.h"
#include <ostream>
#include <map>

typedef enum {EMERG  = 0, 
              FATAL  = 0,
              ALERT  = 100,
              CRIT   = 200,
              ERROR  = 300, 
              WARN   = 400,
              NOTICE = 500,
              INFO   = 600,
              DEBUG  = 700,
              NOTSET = 800,
              ILLEGAL = -1
} Priority;

extern class Category
  &logAppl,
  &logApplC,
  &logChart,
  &logChartMapping,
  &logGenerics,
  &logGrammar,
  &logLexproc,
  &logMorph,
  &logMRS,
  &logPack,
  &logParse,
  &logSM,
  &logChartPruning,
  &logSemantic,
  &logSyntax,
  &logTsdb,
  &logUnpack,
  &logXML,
  &root;

class Category {
  Priority _prio;
  int _printer;
  std::string _name;

public:
  Category() { }
  Category(const std::string &name, Priority prio, int printer) : _name(name) {
    set(prio, printer);
  }

  inline Priority prio() const {
    return (_prio != NOTSET) ? _prio : root._prio;
  }
  inline int printer() const {
    return (_printer != 0) ? _printer : root._printer;
  }
  inline const std::string &name() const { return _name; }

  void set(Priority prio, int printer = 0){
    _prio = prio;
    _printer = printer;
  }
  
  //friend class Logger;
};

class Logger {
public:
  class loggerendl {
    const Category *_cat;
    Priority _prio;
  public:
    const loggerendl &set(const Category &cat, Priority prio) {
      _cat = &cat; _prio = prio; return *this;
    }
    void print(std::ostream &out) const;
  };
  inline static bool enabled(const Category &cat, Priority prio) {
    return cat.prio() >= prio;
  }
  static std::ostream &print(const Category &cat, Priority prio);
  inline static const Logger::loggerendl &
  endl(const Category &cat, Priority prio) { return _e.set(cat,prio); }

  static Category &addCat(const std::string &name, Priority prio, int printer) {
    if (hasCat(name)) throw tError("category "+ name + " added twice");
    _cats.insert(std::pair<std::string, Category>
                 (name, Category(name, prio, printer)));
    return _cats[name];
  }
  static bool hasCat(const std::string name) {
    return (_cats.find(name) != _cats.end());
  }
  static Category &getCat(const std::string name) {
    std::map<std::string, Category>::iterator it = _cats.find(name);
    if (it != _cats.end()) return _cats[name];
    else throw tError("unknown logging category requested");
  }
private:
  static Logger::loggerendl _e;
  static std::map<std::string, Category> _cats;
};

inline std::ostream &operator<<(std::ostream &o, const Logger::loggerendl &e) {
  e.print(o); return o;
}

// wenigstens zwei (drei?) verschiedene Ausgabemethoden: wie compiler-errors
// (vor allem fuer flop), ganz nackig (fuer Ausgaben aus dem System) und wie
// richtige log-messages mit Zeitpunkt, priority und message

#define LOG_ENABLED(__C, __P) Logger::enabled(__C, __P)
#define LOG(__C, __P, __M)                                              \
  (LOG_ENABLED(__C, __P)                                                \
   ? (Logger::print(__C, __P) << __M << Logger::endl(__C, __P), 1) : 0)

void init_logging(const std::string &base_dir);
inline void shutdown_logging() {}
#endif // HAVE_LIBLOG4CPP

#endif // _LOGGING_H
