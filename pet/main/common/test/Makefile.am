# -*- makefile -*-

# FLOPCPPFLAGS = @FLOPCPPFLAGS@

#uncommented since this is dysfunctional right now.... (pead01)
#bin_PROGRAMS = ConfigurationTest loggingTest

# -DNew=new removed
AM_CPPFLAGS = -I$(top_srcdir)/common
AM_CXXFLAGS = -Wall -g
LIBS += @CPPUNIT_LIBS@

#AM_LDFLAGS = -Wl,--defsym,__ctype_b=0,--defsym,__ctype_tolower=0
# Shared libraries are added to flop_LDADD via configure.ac

check_PROGRAMS = ConfigurationTest loggingTest

ConfigurationTest_SOURCES = ConfigurationTest.cpp
nodist_ConfigurationTest_SOURCES = $(top_srcdir)/common/configs.cpp

loggingTest_SOURCES = loggingTest.cpp
nodist_loggingTest_SOURCES = $(top_srcdir)/common/logging.cpp
