#ifndef _LOGGING_H
#define _LOGGING_H

#include "pet-config.h"

#if HAVE_LIBLOG4CXX
// log4cxx header files define HAVE_XML and thus break Pet build system,
// temporary solution below
#  ifdef HAVE_XML
#    define WE_HAVE_XML
#    undef HAVE_XML
#  endif // HAVE_XML
#  include <log4cxx/logger.h>
#  include <log4cxx/basicconfigurator.h>
#  include <log4cxx/propertyconfigurator.h>
#  include <sstream>
#  ifdef WE_HAVE_XML
#    undef WE_HAVE_XML
#    define HAVE_XML
#  else
#    undef HAVE_XML
#  endif // WE_HAVE_XML

// use this for levels WARN, INFO, DEBUG
#  define LOG(logger, level, format, ...)  {                            \
     if (logger->isEnabledFor(level)) {                                 \
       snprintf(logBuffer, logBufferSize, format, ##__VA_ARGS__);        \
       logger->forcedLog(level, logBuffer, LOG4CXX_LOCATION); }}

#  define LOG_ERROR(logger, format, ...) {                                \
    if (logger->isErrorEnabled()) {                                     \
       snprintf(logBuffer, logBufferSize, format, ##__VA_ARGS__);        \
       logger->forcedLog(::log4cxx::Level::ERROR, logBuffer, LOG4CXX_LOCATION); }}

#  define LOG_FATAL(logger, format, ...) {                                \
    if (logger->isErrorEnabled()) {                                     \
       snprintf(logBuffer, logBufferSize, format, ##__VA_ARGS__);        \
       logger->forcedLog(::log4cxx::Level::FATAL, logBuffer, LOG4CXX_LOCATION); }}

extern const int logBufferSize;
extern char logBuffer[];

extern ::log4cxx::LoggerPtr loggerUncategorized;
extern ::log4cxx::LoggerPtr loggerExpand;
extern ::log4cxx::LoggerPtr loggerFs;
extern ::log4cxx::LoggerPtr loggerGrammar;
extern ::log4cxx::LoggerPtr loggerHierarchy;
extern ::log4cxx::LoggerPtr loggerLexproc;
extern ::log4cxx::LoggerPtr loggerParse;
extern ::log4cxx::LoggerPtr loggerTsdb;

using log4cxx::Level;

#else // HAVE_LIBLOG4CXX

#  ifndef LOG // despite lack of log4cxx if can be defined by user
#    define LOG(logger, level, format, ...)
#  endif // LOG

#  ifndef LOG_ERROR
#    define LOG_ERROR(logger, format, ...)    \
       fprintf(stderr, format "\n", ##__VA_ARGS__)
#  endif // LOG_ERROR

#  ifndef LOG_FATAL
#    define LOG_FATAL(logger, format, ...)    \
       fprintf(stderr, format "\n", ##__VA_ARGS__)
#  endif // LOG_FATAL

#endif // HAVE_LIBLOG4CXX

#endif // _LOGGING_H
