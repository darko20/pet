# -*- makefile -*-

include $(top_srcdir)/common/Makefile.common

# Add flop-specific CPPFLAGS/CXXFLAGS/LDFLAGS/LIBS as determined by configure
CPPFLAGS += -I$(top_srcdir)/common @FLOPCPPFLAGS@
LDFLAGS += @FLOPLDFLAGS@
LIBS += @FLOPLIBS@
DEFS += -DDAG_SIMPLE -DWROBLEWSKI3 -DFLOP

# This is always used! <-> documentation
# TODO remove no-deprecated when switched to unordered_{set|map}
AM_CXXFLAGS = -Wall -Wno-deprecated

bin_PROGRAMS = flop

# The include-file dependencies are automatically computed by Automake
# (`Automatic dependency tracking'). Thus, the headers do not need to be listed
# for compilation, but they need to be found for making a distribution.
# Note that the headers in $(top_srcdir)/common are always included
# (cf. $(top_srcdir)/common/Makefile.am) and are therefore not listed here.
flop_SOURCES = corefs.cpp \
	dag-tdl.cpp \
	dump.cpp \
	expand.cpp \
	flop.cpp \
	flop.h \
	full-form.cpp \
	hierarchy.cpp \
	hierarchy.h \
	options.cpp \
	options.h \
	parse-tdl.cpp \
	partition.cpp \
	partition.h \
	print-chic.cpp \
	print-tdl.cpp \
	reduction.cpp \
	symtab.h \
	template.cpp \
	terms.cpp \
	util.cpp \
	$(top_srcdir)/common/bitcode.cpp \
	$(top_srcdir)/common/chunk-alloc.cpp \
	$(top_srcdir)/common/configs.cpp \
	$(top_srcdir)/common/dag-alloc.cpp \
	$(top_srcdir)/common/dag-arced.cpp  \
	$(top_srcdir)/common/dag-common.cpp \
	$(top_srcdir)/common/dag-io.cpp \
	$(top_srcdir)/common/dag-simple.cpp \
	$(top_srcdir)/common/dagprinter.cpp \
	$(top_srcdir)/common/dumper.cpp  \
	$(top_srcdir)/common/grammar-dump.cpp \
	$(top_srcdir)/common/lex-io.cpp \
	$(top_srcdir)/common/lex-tdl.cpp \
	$(top_srcdir)/common/logging.cpp \
	$(top_srcdir)/common/settings.cpp \
	$(top_srcdir)/common/types.cpp \
	$(top_srcdir)/common/utility.cpp

