# -*- makefile -*-

# only build unit tests if cppunit is available (run ./configure --with-cppunit)
if CPPUNIT

AM_CPPFLAGS = -I$(top_srcdir)/common -I$(top_srcdir)/cheap @CHEAPCPPFLAGS@
AM_CXXFLAGS = -Wall
AM_LDFLAGS = @CHEAPLDFLAGS@
LIBS += @CHEAPLIBS@ @CPPUNIT_LIBS@

# types can be added to the type hierarchy at run-time ('dynamic types')
DEFS += -DDYNAMIC_SYMBOLS

# Choice of the unifier:
if TOMABECHI_UNIFIER
DEFS += -DDAG_TOMABECHI
endif

bin_PROGRAMS = tester

tester_SOURCES = tester.cpp \
	paths-test.cpp \
	types-test.cpp
tester_LDADD = ../libcheap.a
if ECLMRS
tester_LDADD += ../libmrs.a
endif

endif

