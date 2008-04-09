# -*- makefile -*-

# FLOPCPPFLAGS = @FLOPCPPFLAGS@

include $(top_srcdir)/common/Makefile.common

bin_PROGRAMS = ConfigurationTest loggingTest

# -DNew=new removed
DEFS = -DDAG_SIMPLE -DWROBLEWSKI3 -DFLOP
AM_CPPFLAGS = -Wall -I$(top_srcdir)/common $(FLOPCPPFLAGS)
AM_CXXFLAGS = -g

#AM_LDFLAGS = -Wl,--defsym,__ctype_b=0,--defsym,__ctype_tolower=0
# Shared libraries are added to flop_LDADD via configure.ac

check_PROGRAMS = ConfigurationTest loggingTest

ConfigurationTest_SOURCES = ConfigurationTest.cpp
nodist_ConfigurationTest_SOURCES = Configuration.cpp

loggingTest_SOURCES = loggingTest.cpp
nodist_loggingTest_SOURCES = logging.cpp
