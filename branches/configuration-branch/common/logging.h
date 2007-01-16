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
#  include <sstream>
#  ifdef WE_HAVE_XML
#    undef WE_HAVE_XML
#    define HAVE_XML
#  else
#    undef HAVE_XML
#  endif // WE_HAVE_XML
#else
#  ifndef LOG4CXX_DEBUG // despite lack of log4cxx if can be defined by user
#    define LOG4CXX_DEBUG(logger, msg) ;
#    define LOG4CXX_INFO(logger, msg) ;
#    define LOG4CXX_WARN(logger, msg) ;
#    define LOG4CXX_ERROR(logger, msg) ;
#    define LOG4CXX_FATAL(logger, msg) ;
#  endif // LOG4CXX_DEBUG
#endif // HAVE_LIBLOG4CXX

#endif // _LOGGING_H
