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

# Automake doesn't want to build something like paths.cpp ../paths.cpp
# because both files would be build in the same directory with the same
# filename.
tester_SOURCES = tester.cpp \
	paths-test.cpp

# test the object files compiled before:
tester_LDADD = ../chart.o \
	../dag-tomabechi.o \
	../fs.o \
	../grammar.o \
	../item.o \
	../lexicon.o \
	../lexparser.o \
	../options.o \
	../parse.o \
	../paths.o \
	../postags.o \
	../sm.o \
	../task.o \
	../tsdb++.o \
	../unicode.o \
	../bitcode.o \
	../chunk-alloc.o \
	../dag-alloc.o \
	../dag-arced.o \
	../dag-common.o \
	../dag-io.o \
	../dumper.o \
	../grammar-dump.o \
	../hash.o \
	../lex-io.o \
	../lex-tdl.o \
	../mfile.o \
	../settings.o \
	../types.o \
	../utility.o

# add ecl and mrs code if found during configure (required by tsdb++.cpp):
if ECL
tester_LDADD += ../petecl.o
endif
if ECLMRS
tester_LDADD+=../cppbridge.o ../petmrs.o ../libmrs.a
endif

endif

