# -*- makefile -*-

# only build unit tests if cppunit is available (run ./configure --with-cppunit)
if CPPUNIT

AM_CPPFLAGS = -I$(top_srcdir)/common -I$(top_srcdir)/cheap @CHEAPCPPFLAGS@
AM_CXXFLAGS = -Wall
AM_LDFLAGS  = @CHEAPLDFLAGS@
LIBS += @CHEAPLIBS@ @CPPUNIT_LIBS@ ../libcheap.a
if ECLMRS
LIBS += ../libmrs.a
endif
DEFS += -DDYNAMIC_SYMBOLS -DDAG_TOMABECHI

bin_PROGRAMS = tester

tester_SOURCES = tester.cpp \
	paths-test.cpp \
	types-test.cpp

endif

