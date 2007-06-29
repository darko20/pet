# -*- makefile -*-

# only build unit tests if cppunit is available (run ./configure --with-cppunit)
if CPPUNIT

# Development switches
DEFS += -DDYNAMIC_SYMBOLS
OPTIM=-O2

# These should be tested appropriately and appear in config.h
AM_CPPFLAGS = -I$(top_srcdir)/common -I$(top_srcdir)/cheap @CHEAPCPPFLAGS@
AM_CXXFLAGS = -g -Wall $(OPTIM) $(DEFS)
AM_LDFLAGS = @CHEAPLDFLAGS@
LIBS += @CHEAPLIBS@ @CPPUNIT_LIBS@

bin_PROGRAMS = tester

# Automake doesn't want to build something like paths.cpp ../paths.cpp
# because both files would be build in the same directory with the same
# filename.
tester_SOURCES = tester.cpp paths-test.cpp ../paths.cpp

endif

