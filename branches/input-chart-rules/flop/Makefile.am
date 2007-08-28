# -*- makefile -*-

include $(top_srcdir)/common/Makefile.common

bin_PROGRAMS = flop

DEFS += -DDAG_SIMPLE -DWROBLEWSKI3 -DFLOP
AM_CPPFLAGS = -I$(top_srcdir)/common @FLOPCPPFLAGS@
AM_CXXFLAGS = -Wall
AM_LDFLAGS  = @FLOPLDFLAGS@

# The include-file dependencies are automatically computed by Automake
# (`Automatic dependency tracking'). Thus, the headers do not need to be listed
# for compilation, but they need to be found for making a distribution.
# Note that the headers in $(top_srcdir)/common are always included
# (cf. $(top_srcdir)/common/Makefile.am) and need not be listed here.
flop_SOURCES = corefs.cpp \
	dag-tdl.cpp \
	dump.cpp \
	expand.cpp \
	flop.cpp flop.h \
	full-form.cpp \
	hierarchy.cpp hierarchy.h \
	options.cpp options.h \
	parse-tdl.cpp \
	partition.cpp partition.h \
	print-chic.cpp \
	print-tdl.cpp \
	reduction.cpp \
	symtab.h \
	template.cpp \
	terms.cpp \
	util.cpp \
	$(top_srcdir)/common/bitcode.cpp $(top_srcdir)/common/bitcode.h \
	$(top_srcdir)/common/chunk-alloc.cpp $(top_srcdir)/common/chunk-alloc.h \
	$(top_srcdir)/common/dag-alloc.cpp $(top_srcdir)/common/dag-alloc.h \
	$(top_srcdir)/common/dag-arced.cpp $(top_srcdir)/common/dag-arced.h \
	$(top_srcdir)/common/dag-common.cpp $(top_srcdir)/common/dag-common.h \
	$(top_srcdir)/common/dag-io.cpp \
	$(top_srcdir)/common/dag-simple.cpp \
	$(top_srcdir)/common/dumper.cpp $(top_srcdir)/common/dumper.h \
	$(top_srcdir)/common/grammar-dump.cpp $(top_srcdir)/common/grammar-dump.h \
	$(top_srcdir)/common/lex-io.cpp $(top_srcdir)/common/lex-io.h \
	$(top_srcdir)/common/lex-tdl.cpp $(top_srcdir)/common/lex-tdl.h \
	$(top_srcdir)/common/settings.cpp $(top_srcdir)/common/settings.h \
	$(top_srcdir)/common/types.cpp $(top_srcdir)/common/types.h \
	$(top_srcdir)/common/utility.cpp $(top_srcdir)/common/utility.h

