/** -*- Mode: C++ -*-
 *  @file logging.h
 *  @brief PET's utilities for log4cxx.
 */

#ifndef _LOGGING_H
#define _LOGGING_H

#include "pet-config.h"

#if HAVE_LIBLOG4CPP
#else // HAVE_LIBLOG4CXX
#include <ostream>

extern class Category 
  logAppl, logApplC, logGenerics, logGrammar, logLexproc, logMorph, logPack,
  logParse, logSM, logSemantic, logSyntax, logTsdb, logUnpack, logXML, root;

typedef enum {EMERG  = 0, 
              FATAL  = 0,
              ALERT  = 100,
              CRIT   = 200,
              ERROR  = 300, 
              WARN   = 400,
              NOTICE = 500,
              INFO   = 600,
              DEBUG  = 700,
              NOTSET = 800
} PriorityLevel;

class Category {
  PriorityLevel _prio;
  int _printer;
public:
  Category(PriorityLevel prio, int printer) {
    set(prio, printer);
  }

  void set(PriorityLevel prio, int printer){
    _prio = prio != NOTSET ? prio : root._prio;
    _printer = (printer != 0) ? printer : root._printer;
  }
  
  friend class Logger;
};

class Logger {
public:
  class loggerendl {
    const Category *_cat;
    PriorityLevel _prio;
  public:
    const loggerendl &set(const Category &cat, PriorityLevel prio) {
      _cat = &cat; _prio = prio; return *this;
    }
    void print(std::ostream &out) const;
  };
  inline static bool enabled(const Category &cat, PriorityLevel prio) {
    return cat._prio >= prio;
  }
  static std::ostream &print(const Category &cat, PriorityLevel prio);
  inline static const Logger::loggerendl &
  endl(const Category &cat, PriorityLevel prio) { return _e.set(cat,prio); }
private:
  static Logger::loggerendl _e;
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

#endif // HAVE_LIBLOG4CPP
#endif // _LOGGING_H
