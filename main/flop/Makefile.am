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
nodist_flop_SOURCES = bitcode.cpp chunk-alloc.cpp dag-alloc.cpp		\
	dag-arced.cpp dag-common.cpp dag-io.cpp	dag-simple.cpp		\
	dumper.cpp grammar-dump.cpp lex-io.cpp lex-tdl.cpp		\
	settings.cpp types.cpp utility.cpp

flop_SOURCES = flop.h flop.cpp builtins.cpp terms.cpp full-form.cpp	\
	parse-tdl.cpp print-chic.cpp print-tdl.cpp template.cpp		\
	corefs.cpp util.cpp options.h options.cpp hierarchy.h		\
        hierarchy.cpp expand.cpp reduction.h reduction.cpp dag-tdl.cpp	\
        dump.cpp symtab.h partition.h partition.cpp hashing.h


# boost libraries: -lm -lstdc++

# nodist_boost_SOURCES = 

# boost_SOURCES = hierarchy.h reduction.h boost.cpp reduction.cpp

EXTRA_DIST = Jamfile
