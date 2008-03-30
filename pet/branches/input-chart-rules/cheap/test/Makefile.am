# -*- makefile -*-

AM_CPPFLAGS = -I$(top_srcdir)/common -I$(top_srcdir)/cheap @CHEAPCPPFLAGS@
AM_CXXFLAGS = -Wall
AM_LDFLAGS  = @CHEAPLDFLAGS@
LIBS += @CHEAPLIBS@ @CPPUNIT_LIBS@
DEFS += -DDYNAMIC_SYMBOLS -DDAG_TOMABECHI

bin_PROGRAMS = tester

tester_SOURCES = tester.cpp \
	paths-test.cpp \
	types-test.cpp
tester_LDADD = ../libcheap.a
if ECLMRS
tester_LDADD += ../libmrs.a
endif

