TEMPLATE = vcapp
CONFIG += console
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .
INCLUDEPATH += ..
INCLUDEPATH += ../common
INCLUDEPATH += c:/mingw/include/boost-1_43
#LIBS += -Lc:/mingw/lib

DEFINES += HAVE_CONFIG_H
DEFINES += DAG_SIMPLE
DEFINES += WROBLEWSKI3
DEFINES += FLOP
DEFINES += DYNAMIC_SYMBOLS

# Input
HEADERS += flop.h \
           hierarchy.h \
           options.h \
           partition.h \
           symtab.h \
           ../pet-config.h \
           ../common/bitcode.h \
           ../common/configs.h \
           ../common/dag-arced.h \
           ../common/dag.h \
           ../common/dumper.h \
           ../common/errors.h \
           ../common/grammar-dump.h \
           ../common/lex-io.h \
           ../common/lex-tdl.h \
           ../common/list-int.h \
           ../common/logging.h \
           ../common/settings.h \
           ../common/types.h \
           ../common/utility.h \
           ../common/version.h \

SOURCES += \
           corefs.cpp \
           dag-tdl.cpp \
           dump.cpp \
           expand.cpp \
           flop.cpp \
           full-form.cpp \
           hierarchy.cpp \
           options.cpp \
           parse-tdl.cpp \
           partition.cpp \
           print-chic.cpp \
           print-tdl.cpp \
           reduction.cpp \
           template.cpp \
           terms.cpp \
           util.cpp \
	   ../common/bitcode.cpp \
	   ../common/chunk-alloc.cpp \
	   ../common/configs.cpp \
	   ../common/dag-alloc.cpp \
	   ../common/dag-arced.cpp  \
	   ../common/dag-common.cpp \
	   ../common/dag-io.cpp \
	   ../common/dag-simple.cpp \
	   ../common/dumper.cpp  \
	   ../common/grammar-dump.cpp \
	   ../common/lex-io.cpp \
	   ../common/lex-tdl.cpp \
	   ../common/logging.cpp \
	   ../common/settings.cpp \
	   ../common/types.cpp \
	   ../common/utility.cpp

