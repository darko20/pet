# -*- makefile -*-

FLOPCPPFLAGS = @FLOPCPPFLAGS@

include $(top_srcdir)/common/Makefile.common

bin_PROGRAMS = flop # boost

# -DNew=new removed
DEFS = -DDAG_SIMPLE -DWROBLEWSKI3 -DFLOP
AM_CPPFLAGS = -Wall -I$(top_srcdir)/common $(FLOPCPPFLAGS)
AM_CXXFLAGS = -O3

AM_LDFLAGS = -Wl,--defsym,__ctype_b=0,--defsym,__ctype_tolower=0
# Shared libraries are added to flop_LDADD via configure.ac

# In ../common (without headers)
nodist_flop_SOURCES = bitcode.cpp	\
	chunk-alloc.cpp			\
	dag-alloc.cpp			\
	dag-arced.cpp 			\
	dag-common.cpp			\
	dag-io.cpp			\
	dag-simple.cpp			\
	dumper.cpp 			\
	grammar-dump.cpp		\
	lex-io.cpp			\
	lex-tdl.cpp			\
	settings.cpp 			\
	types.cpp			\
	utility.cpp

flop_SOURCES = builtins.cpp	\
	corefs.cpp		\
	dag-tdl.cpp		\
	dump.cpp		\
	expand.cpp		\
	flop.cpp		\
	flop.h			\
	full-form.cpp		\
	hierarchy.cpp		\
	hierarchy.h		\
	options.cpp		\
	options.h		\
	parse-tdl.cpp		\
	partition.cpp		\
	partition.h		\
	print-chic.cpp		\
	print-tdl.cpp		\
	reduction.cpp		\
	reduction.h		\
	symtab.h		\
	template.cpp		\
	terms.cpp		\
	util.cpp


# boost libraries: -lm -lstdc++

# nodist_boost_SOURCES = 

# boost_SOURCES = hierarchy.h reduction.h boost.cpp reduction.cpp

EXTRA_DIST = Jamfile
