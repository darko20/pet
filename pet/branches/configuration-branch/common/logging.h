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

/** @brief A Class for intercepting fprintf.
 *
 * Subsequent calls to printf method write to a buffer that can
 * be accessed at the end.
 */
class PrintfBuffer {
public:
  /**
   * @param buffer  an external buffer used for writing output from printf.
   * @param size    size of the buffer.
   */
  PrintfBuffer(char *buffer, int size = 65536);

  //make gcc not complain about non-virtual destructor
  virtual ~PrintfBuffer();

  /**
   * Gets a C-style string containing everything that has been printed
   * (by method printf) so far. (Provided that the buffer has not 
   * been overflowed).
   */
  virtual char* getContents();
  
  /**
   * Checks if the buffer is overflowed.
   * @return true iff the buffer is overflowed.
   */
  virtual bool isOverflowed();

  /**
   * Writes to a buffer. Works like stdio's vprintf.
   * @return result of internal call to vsnprintf() or -1 on error
   */
  virtual int vprintf(char *fmt, va_list ap);

protected:
  char *buffer_;
  const int size_;  //size of the buffer_
  int nWritten_;    //sum of positive results of vsnprintf,
                    //in case of no errors this is the number of character
                    //written so far
private:
  PrintfBuffer(const PrintfBuffer &pb);
};

/**
 * Works like PrintfBuffer but output is not captured, it is sent to
 * a stdio's FILE.
 */
class StreamPrinter : public PrintfBuffer {
public:
  StreamPrinter(FILE *file);
  virtual ~StreamPrinter();
  virtual char* getContents();
  virtual bool isOverflowed();
  virtual int vprintf(char *fmt, va_list ap);
private:
  FILE *file_;
};

/**
 * @brief Substitutes fprintf, uses PrintfBuffer to capture output.
 *
 * @param printfBuffer object that accumulates results.
 */
int pbprintf(PrintfBuffer *printfBuffer, char *fmt, ...);

#endif // _LOGGING_H
