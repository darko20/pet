######################################################################
# Automatically generated by qmake (2.01a) Wed 28. Apr 10:49:38 2010
######################################################################

TEMPLATE = vcapp
CONFIG += console
#QT += xml

TARGET = cheap
INCLUDEPATH += .
INCLUDEPATH += ..
INCLUDEPATH += ../common
DEFINES += DAG_TOMABECHI
DEFINES += HAVE_CONFIG_H
DEFINES += DYNAMIC_SYMBOLS
#DEFINES += HAVE_XML

# Input
HEADERS += \
           agenda.h \
           chart.h \
           cheap.h \
           cheaptimer.h \
#           cppbridge.h \
           dag-tomabechi.h \
           extdict.h \
           failure.h \
           fs.h \
           grammar.h \
           input-modules.h \
           item-printer.h \
           item.h \
           itsdb.h \
           lexicon.h \
           lexparser.h \
           lingo-tokenizer.h \
           morph-inner.h \
           morph.h \
#           mrs-handler.h \
#           mrs-states.h \
           mrs.h \
           options.h \
           parse.h \
           paths.h \
#           petecl.h \
#           petmrs.h \
#           pic-handler.h \
#           pic-states.h \
           pic-tokenizer.h \
           position-mapper.h \
           postags.h \
           qc.h \
           restrictor.h \
           sm.h \
#           smaf-tokenizer.h \
           task.h \
           tsdb++.h \
           vpm.h \
           yy-tokenizer.h \
           yy.h \
           ../pet-config.h \
           ../common/chunk-alloc.h \
           ../common/configs.h \
           ../common/dag.h \
           ../common/dag-alloc.h \
           ../common/dag-common.h \
           ../common/dagprinter.h \
           ../common/dumper.h \
           ../common/errors.h \
           ../common/grammar-dump.h \
           ../common/hash.h \
           ../common/hashing.h \
           ../common/lex-tdl.h \
           ../common/logging.h \
           ../common/logging.h \
           ../common/settings.h \
           ../common/types.h \
           ../common/utility.h \
           ../common/version.h \

SOURCES += chart.cpp \
           cheap.cpp \
#           cppbridge.cpp \
           dag-tomabechi.cpp \
           extdict.cpp \
           failure.cpp \
           fs.cpp \
           grammar.cpp \
           input-modules.cpp \
           item-printer.cpp \
           item.cpp \
           lexicon.cpp \
           lexparser.cpp \
           lingo-tokenizer.cpp \
           morph.cpp \
#           mrs-handler.cpp \
#           mrs-states.cpp \
           mrs.cpp \
           options.cpp \
           parse.cpp \
           paths.cpp \
#           pet.cpp \
#           petecl.c \
#           petmrs.c \
#           pic-handler.cpp \
#           pic-tokenizer.cpp \
           postags.cpp \
           qc.cpp \
           restrictor.cpp \
           sm.cpp \
#           smaf-tokenizer.cpp \
           task.cpp \
           tsdb++.cpp \
           vpm.cpp \
           yy-tokenizer.cpp \
           yy.cpp \
           ../common/bitcode.cpp \
           ../common/chunk-alloc.cpp \
           ../common/configs.cpp \
	   ../common/dag-alloc.cpp \
	   ../common/dag-arced.cpp \
	   ../common/dag-common.cpp \
	   ../common/dag-io.cpp \
	   ../common/dagprinter.cpp \
	   ../common/dumper.cpp \
	   ../common/grammar-dump.cpp \
	   ../common/hash.cpp \
           ../common/lex-io.cpp \
	   ../common/lex-tdl.cpp \
	   ../common/logging.cpp \
           ../common/mfile.cpp \
	   ../common/settings.cpp \
	   ../common/types.cpp \
	   ../common/utility.cpp \

