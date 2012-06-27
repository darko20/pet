# -*- makefile -*-
#
# We use the same Makefile, whether the MRS library is used standalone, or
# inside the PET tree. The appropriate configure script (depending on what is
# considered top-level) should set the right variable to include only the
# available binaries and source files.

# This is always used! 
AM_CXXFLAGS = -Wall
ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = mrstest mrsconvert edmcompare

mrstest_SOURCES = \
	mrstest.cpp \
	mrs.cpp mrs.h \
	mrs-printer.cpp mrs-printer.h \
	mrs-reader.cpp mrs-reader.h \
  eds.cpp eds.h 

edmcompare_SOURCES = \
	edmcompare.cpp \
	mrs.cpp mrs.h \
	mrs-printer.cpp mrs-printer.h \
	mrs-reader.cpp mrs-reader.h \
  eds.cpp eds.h 

mrsconvert_SOURCES = \
	mrsconvert.cpp \
	mrs.cpp mrs.h \
	mrs-printer.cpp mrs-printer.h \
	mrs-reader.cpp mrs-reader.h \
  eds.cpp eds.h 

if MRS_STANDALONE

mrstest_SOURCES += mrs-utility.h mrs-utility.cpp mrs-errors.h
edmcompare_SOURCES += mrs-utility.h mrs-utility.cpp mrs-errors.h
mrsconvert_SOURCES += mrs-utility.h mrs-utility.cpp mrs-errors.h

else
# we are compiling within PET

VPATH += $(top_srcdir)/common

mrstest_SOURCES += $(top_srcdir)/common/utility.h \
  $(top_srcdir)/common/utility.cpp $(top_srcdir)/common/errors.h
edmcompare_SOURCES += $(top_srcdir)/common/utility.h \
  $(top_srcdir)/common/utility.cpp $(top_srcdir)/common/errors.h
mrsconvert_SOURCES += $(top_srcdir)/common/utility.h \
  $(top_srcdir)/common/utility.cpp $(top_srcdir)/common/errors.h


# Add cheap-specific CPPFLAGS/CXXFLAGS/LDFLAGS/LIBS as determined by configure
CPPFLAGS += -I$(top_srcdir)/common -I$(top_srcdir)/fspp -I$(top_srcdir)/cheap 
CXXFLAGS += @CHEAPCPPFLAGS@
LDFLAGS += @CHEAPLDFLAGS@
LIBS += @CHEAPLIBS@
DEFS += -DDYNAMIC_SYMBOLS

if XML
mrstest_SOURCES += \
	$(top_srcdir)/cheap/xmlparser.cpp $(top_srcdir)/cheap/xmlparser.h \
	$(top_srcdir)/cheap/unicode.cpp $(top_srcdir)/cheap/unicode.h \
	$(top_srcdir)/common/logging.cpp $(top_srcdir)/common/logging.h \
	$(top_srcdir)/common/hash.cpp $(top_srcdir)/common/hash.h \
	$(top_srcdir)/common/settings.cpp $(top_srcdir)/common/settings.h \
	mrs-handler.cpp mrs-handler.h \
	mrs-states.cpp mrs-states.h
endif

if XML
edmcompare_SOURCES += \
	$(top_srcdir)/cheap/xmlparser.cpp $(top_srcdir)/cheap/xmlparser.h \
	$(top_srcdir)/cheap/unicode.cpp $(top_srcdir)/cheap/unicode.h \
	$(top_srcdir)/common/logging.cpp $(top_srcdir)/common/logging.h \
	$(top_srcdir)/common/hash.cpp $(top_srcdir)/common/hash.h \
	mrs-handler.cpp mrs-handler.h \
	mrs-states.cpp mrs-states.h
endif

if XML
mrsconvert_SOURCES += \
	$(top_srcdir)/cheap/xmlparser.cpp $(top_srcdir)/cheap/xmlparser.h \
	$(top_srcdir)/cheap/unicode.cpp $(top_srcdir)/cheap/unicode.h \
	$(top_srcdir)/common/logging.cpp $(top_srcdir)/common/logging.h \
	$(top_srcdir)/common/hash.cpp $(top_srcdir)/common/hash.h \
	mrs-handler.cpp mrs-handler.h \
	mrs-states.cpp mrs-states.h
endif

#	mrs-tfs.cpp mrs-tfs.h \
#	mrs-handler.cpp mrs-handler.h \
#	mrs-states.cpp mrs-states.h \
#	$(top_srcdir)/common/configs.cpp \
#	$(top_srcdir)/common/dag-alloc.cpp \
#	$(top_srcdir)/common/dag-arced.cpp \
#	$(top_srcdir)/common/dag-common.cpp \
#	$(top_srcdir)/common/dag-io.cpp \
#	$(top_srcdir)/common/dagprinter.cpp \
#	$(top_srcdir)/common/dumper.cpp \
#	$(top_srcdir)/common/hash.cpp \
#	$(top_srcdir)/common/lex-io.cpp \
#	$(top_srcdir)/common/lex-tdl.cpp \
#	$(top_srcdir)/common/logging.cpp \
#	$(top_srcdir)/common/mfile.c \
#	$(top_srcdir)/common/settings.cpp \
#	$(top_srcdir)/common/types.cpp 

endif 


profclean:
	rm -f *.gcov gmon.out *.bb *.bbg *.da

