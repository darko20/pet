/** @file logging.h
 *  @brief PET's utilities for log4cxx.
 */

#ifndef _LOGGING_H
#define _LOGGING_H

#include <cstdarg>

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

//use only if logging is enabled
#  define LOG_ONLY(instr) instr

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

extern const int defaultPbSize;
extern char defaultPb[];

#else // HAVE_LIBLOG4CXX

#  define LOG_ONLY(instr)

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

/**
 * @brief Interface for a class processing printf calls.
 *
 * A Class that implements this interface can be used in function
 * pbprintf(). It allows a programmer to change legacy code to use
 * more flexible I/O. This interface was designed to switch from fprintf-based
 * logging to log4cxx.
 */
class IPrintfHandler {
public:
  /**
   * This method is called by pbprintf(). See code of pbprintf.
   */
  virtual int vprintf(char *fmt, va_list ap) = 0;
  
  virtual ~IPrintfHandler();
};

/** @brief A Class for intercepting fprintf.
 *
 * Subsequent calls to printf method write to a buffer that can
 * be accessed at the end. Buffer's size changes dynamically.
 */
class PrintfBuffer : public IPrintfHandler {
public:
  /**
   * Use without parameters (default values shouuld be ok for everything).
   */
  PrintfBuffer(int size = 1024, int chunkSize = 1024);

  virtual ~PrintfBuffer();

  /**
   * Gets a C-style string containing everything that has been printed
   * (by method vprintf) so far.  Returned pointer is valid until the object
   * is destructed.
   */
  virtual char* getContents();
  
  virtual int vprintf(char *fmt, va_list ap);

protected:
  char *buffer_;
  int size_;        // size of the buffer_
  int chunkSize_;   // buffer size will be increased by this number
                    // each time it is neccessary
  int nWritten_;    // number of characters written so far
                    // (including trailing '\0')
  
#if HAVE_LIBLOG4CXX
  static log4cxx::LoggerPtr logger;
#endif // HAVE_LIBLOG4CXX

private:
  PrintfBuffer(const PrintfBuffer &pb);
};

/**
 * Output is sent to a stdio's FILE*.
 */
class StreamPrinter : public PrintfBuffer {
public:
  StreamPrinter(FILE *file);
  virtual ~StreamPrinter();
  virtual int vprintf(char *fmt, va_list ap);
private:
  FILE *file_;
};

/**
 * @brief Substitutes fprintf, uses IPrintfHandler to process arguments.
 *
 * @param iph object that processes the rest of the arguments.
 */
int pbprintf(IPrintfHandler &iph, char *fmt, ...);

#endif // _LOGGING_H
