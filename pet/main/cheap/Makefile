%.o: %.cpp
	$(COMPILE.cc) $< $(OUTPUT_OPTION)

RM = rm
OBJ = o
CXX = g++
CC = gcc
# CC = KCC --
# CXX = KCC

ITSDBPATH = /home/oe/src/lkb
INCLUDE = -I. -I../common  -I$(ITSDBPATH)/include -I$(ICUDIR)/include

ICUDIR = /usr/local
#ICUDIR = /lt

# for Unix/gcc
CPPFLAGS1 = -Wall -g -DHASH_MAP_AVAIL -DUSEMMAP -DYY $(INCLUDE) \
  -DTSDBAPI -DSOCKET_INTERFACE -DONLINEMORPH -DICU
# for CygWin
# CPPFLAGS1 = -Wall -g -O3 -DYY $(INCLUDE)

#
# choice here has to correspond to choice for DAGOBJ (see below):
#

# CPPFLAGS = -DDAG_SIMPLE -DWROBLEWSKI2 $(CPPFLAGS1)
CPPFLAGS = -DDAG_TOMABECHI $(CPPFLAGS1) -DQC_PATH_COMP # -DPACKING
# CPPFLAGS = -DDAG_FIXED $(CPPFLAGS1) -DNO_CAST
# CPPFLAGS = -DDAG_SYNTH1 $(CPPFLAGS1)


CFLAGS = $(CPPFLAGS)

# solaris
# LIBS = -Wl,-R,/proj/perform/uc/5lib -lm -lstdc++ -ldl -liberty
# linux dynamic
# LIBS = -lm -lstdc++ -ldl -liberty
# linux static
LIBS = -lm -lstdc++ -ldl -liberty
# cygwin
# LIBS = -lm -lstdc++ -liberty

# solaris
# ITSDBLIBS = $(ITSDBPATH)/lib/solaris/libitsdb.a $(ITSDBPATH)/lib/solaris/libpvm3.a -lsocket -lnsl -lkstat -lposix4
# linux dynamic
# ITSDBLIBS = -L$(ITSDBPATH)/lib/linux -litsdb -lpvm3 -liberty
# linux static
ITSDBLIBS = -L$(ITSDBPATH)/lib/linux $(ITSDBPATH)/lib/linux/libitsdb.a -lpvm3 -liberty
# cygwin (no itsdb available)
# ITSDBLIBS =

#ICULIBS = -L$(ICUDIR)/lib -licuuc -licui18n -lpthread -ldl
ICULIBS = -Wl,-R,$(ICUDIR)/lib -L$(ICUDIR)/lib -licuuc -licui18n -lpthread -ldl


VPATH = .:../common

#
# choice here has to correspond to choice for CPPFLAGS (see above):
#

# DAGOBJ = dag-io.$(OBJ) dag-common.$(OBJ) dag-arced.$(OBJ) dag-alloc.$(OBJ) dag-simple.$(OBJ)
DAGOBJ = dag-io.$(OBJ) dag-common.$(OBJ) dag-arced.$(OBJ) dag-alloc.$(OBJ) dag-tomabechi.$(OBJ) failure.$(OBJ) qc.$(OBJ)
# DAGOBJ = dag-common.$(OBJ) dag-fixed.$(OBJ)
# DAGOBJ = dag-io.$(OBJ) dag-common.$(OBJ) dag-arced.$(OBJ) dag-alloc.$(OBJ) dag-synth1.$(OBJ)

COMMONOBJS = bitcode.$(OBJ) chunk-alloc.$(OBJ) errors.$(OBJ) mfile.$(OBJ) \
	lex-io.$(OBJ) lex-tdl.$(OBJ) settings.$(OBJ) fs.$(OBJ) dumper.$(OBJ) grammar-dump.$(OBJ) grammar.$(OBJ) types.$(OBJ) tsdb++.$(OBJ) utility.$(OBJ) $(DAGOBJ)

PARSEOBJS = parse.$(OBJ) item.$(OBJ) task.$(OBJ) chart.$(OBJ) agenda.$(OBJ) tokenizer.$(OBJ) options.$(OBJ) postags.$(OBJ) inputtoken.$(OBJ) inputchart.$(OBJ) mrs.$(OBJ) yy.$(OBJ) morph.$(OBJ) unicode.$(OBJ) paths.$(OBJ) sm.$(OBJ) hash.$(OBJ)

#
# cheap
#

CHEAPOBJS = cheap.$(OBJ) $(COMMONOBJS) $(PARSEOBJS)
cheap: $(CHEAPOBJS)
	$(CXX) $(CFLAGS) -o $@ $^ $(LIBS) $(ITSDBLIBS) $(ICULIBS)

#
# mtest
#

MTESTOBJS = mtest.$(OBJ) chunk-alloc.$(OBJ)
mtest: $(MTESTOBJS)
	$(CXX) $(CFLAGS) -o $@ $^ $(LIBS)

#
# sanity
#

clean:
	$(RM) *.$(OBJ)

#
# dependencies
#

include .depend

# DO NOT DELETE

agenda.o: agenda.h task.h grammar.h ../common/utility.h fs.h sm.h item.h
agenda.o: /usr/include/limits.h /usr/include/features.h
agenda.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
agenda.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h options.h
agenda.o: inputtoken.h postags.h paths.h
chart.o: cheap.h options.h ../common/errors.h chart.h item.h
chart.o: /usr/include/limits.h /usr/include/features.h
chart.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
chart.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h fs.h
chart.o: grammar.h ../common/utility.h sm.h inputtoken.h postags.h paths.h
chart.o: tsdb++.h
cheap.o: cheap.h options.h ../common/errors.h parse.h tsdb++.h agenda.h
cheap.o: task.h grammar.h ../common/utility.h fs.h sm.h item.h
cheap.o: /usr/include/limits.h /usr/include/features.h
cheap.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
cheap.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
cheap.o: inputtoken.h postags.h paths.h chart.h inputchart.h chartpositions.h
dag-tomabechi.o: tsdb++.h options.h
dumpgram.o: grammar.h ../common/utility.h fs.h sm.h
failure.o: failure.h
fs.o: cheap.h options.h ../common/errors.h fs.h tsdb++.h
grammar.o: grammar.h ../common/utility.h fs.h sm.h cheap.h options.h
grammar.o: ../common/errors.h parse.h tsdb++.h
inputchart.o: options.h grammar.h ../common/utility.h fs.h sm.h parse.h
inputchart.o: tsdb++.h item.h /usr/include/limits.h /usr/include/features.h
inputchart.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
inputchart.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
inputchart.o: inputtoken.h postags.h paths.h tokenizer.h cheap.h
inputchart.o: ../common/errors.h inputchart.h chartpositions.h
inputtoken.o: cheap.h options.h ../common/errors.h parse.h tsdb++.h
inputtoken.o: inputchart.h postags.h chartpositions.h inputtoken.h grammar.h
inputtoken.o: ../common/utility.h fs.h sm.h paths.h item.h
inputtoken.o: /usr/include/limits.h /usr/include/features.h
inputtoken.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
inputtoken.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
item.o: cheap.h options.h ../common/errors.h ../common/utility.h item.h
item.o: /usr/include/limits.h /usr/include/features.h
item.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
item.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h fs.h
item.o: grammar.h sm.h inputtoken.h postags.h paths.h parse.h tsdb++.h
morph.o: cheap.h options.h ../common/errors.h unicode.h morph.h grammar.h
morph.o: ../common/utility.h fs.h sm.h
mrs.o: item.h /usr/include/limits.h /usr/include/features.h
mrs.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
mrs.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h fs.h
mrs.o: options.h grammar.h ../common/utility.h sm.h inputtoken.h postags.h
mrs.o: paths.h mrs.h cheap.h ../common/errors.h tsdb++.h
mtest.o: /usr/include/stdio.h /usr/include/features.h
mtest.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
mtest.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
mtest.o: /usr/include/bits/types.h /usr/include/libio.h
mtest.o: /usr/include/_G_config.h /usr/include/wchar.h
mtest.o: /usr/include/bits/wchar.h /usr/include/gconv.h
mtest.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
mtest.o: /usr/include/bits/stdio_lim.h /usr/include/stdlib.h
mtest.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
mtest.o: /usr/include/bits/confname.h
options.o: /usr/include/getopt.h /usr/include/ctype.h /usr/include/features.h
options.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
options.o: /usr/include/bits/types.h
options.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
options.o: /usr/include/endian.h /usr/include/bits/endian.h fs.h cheap.h
options.o: options.h ../common/errors.h ../common/utility.h
parse.o: cheap.h options.h ../common/errors.h parse.h tsdb++.h fs.h item.h
parse.o: /usr/include/limits.h /usr/include/features.h
parse.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
parse.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h grammar.h
parse.o: ../common/utility.h sm.h inputtoken.h postags.h paths.h chart.h
parse.o: inputchart.h chartpositions.h tokenizer.h agenda.h task.h
paths.o: paths.h
postags.o: cheap.h options.h ../common/errors.h item.h /usr/include/limits.h
postags.o: /usr/include/features.h /usr/include/sys/cdefs.h
postags.o: /usr/include/gnu/stubs.h
postags.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h fs.h
postags.o: grammar.h ../common/utility.h sm.h inputtoken.h postags.h paths.h
sm.o: sm.h options.h
task.o: task.h grammar.h ../common/utility.h fs.h sm.h item.h
task.o: /usr/include/limits.h /usr/include/features.h
task.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
task.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h options.h
task.o: inputtoken.h postags.h paths.h parse.h tsdb++.h chart.h agenda.h
tokenizer.o: cheap.h options.h ../common/errors.h grammar.h
tokenizer.o: ../common/utility.h fs.h sm.h parse.h tsdb++.h inputchart.h
tokenizer.o: postags.h chartpositions.h inputtoken.h paths.h tokenizer.h
tsdb++.o: cheap.h options.h ../common/errors.h parse.h tsdb++.h agenda.h
tsdb++.o: task.h grammar.h ../common/utility.h fs.h sm.h item.h
tsdb++.o: /usr/include/limits.h /usr/include/features.h
tsdb++.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
tsdb++.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
tsdb++.o: inputtoken.h postags.h paths.h chart.h inputchart.h
tsdb++.o: chartpositions.h tokenizer.h qc.h failure.h
unicode.o: ../common/errors.h unicode.h
yy.o: ../common/utility.h ../common/errors.h tokenizer.h cheap.h options.h
yy.o: parse.h tsdb++.h agenda.h task.h grammar.h fs.h sm.h item.h
yy.o: /usr/include/limits.h /usr/include/features.h /usr/include/sys/cdefs.h
yy.o: /usr/include/gnu/stubs.h
yy.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h inputtoken.h
yy.o: postags.h paths.h chart.h inputchart.h chartpositions.h yy.h
yy.o: /usr/include/stdio.h
yy.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
yy.o: /usr/include/bits/types.h /usr/include/libio.h /usr/include/_G_config.h
yy.o: /usr/include/wchar.h /usr/include/bits/wchar.h /usr/include/gconv.h
yy.o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
yy.o: /usr/include/bits/stdio_lim.h /usr/include/assert.h
