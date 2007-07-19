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
	paths-test.cpp \
	../chart.cpp \
	../dag-tomabechi.cpp \
	../fs.cpp \
	../grammar.cpp \
	../item.cpp \
	../lexicon.cpp \
	../lexparser.cpp \
	../options.cpp \
	../parse.cpp \
	../paths.cpp \
	../postags.cpp \
	../sm.cpp \
	../task.cpp \
	../tsdb++.cpp \
	../unicode.cpp \
	$(top_srcdir)/common/bitcode.cpp \
	$(top_srcdir)/common/chunk-alloc.cpp \
	$(top_srcdir)/common/dag-alloc.cpp \
	$(top_srcdir)/common/dag-arced.cpp \
	$(top_srcdir)/common/dag-common.cpp \
	$(top_srcdir)/common/dag-io.cpp \
	$(top_srcdir)/common/dumper.cpp \
	$(top_srcdir)/common/grammar-dump.cpp \
	$(top_srcdir)/common/hash.cpp \
	$(top_srcdir)/common/lex-io.cpp \
	$(top_srcdir)/common/lex-tdl.cpp \
	$(top_srcdir)/common/mfile.c \
	$(top_srcdir)/common/settings.cpp \
	$(top_srcdir)/common/types.cpp \
	$(top_srcdir)/common/utility.cpp

endif

