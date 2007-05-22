SUBDIRS      = flop cheap
if ECL
SUBDIRS += fspp
endif
DIST_SUBDIRS = $(SUBDIRS) common
EXTRA_DIST   = BUGS CHANGELOG LICENSE README TODO \
	       doxyconfig.cheap doxyconfig.flop

# For ebrowse (Emacs)
BROWSE: 
	echo `find $(topsrc_dir) -name '*.h' -print ` \
	     `find $(topsrc_dir) \( -name '*.c' -o -name '*.cpp' \) -print` | \
	tr ' ' '\n'| ebrowse

sign: dist
	gpg --detach-sign --armor $(distdir).tar.gz
	md5sum $(distdir).tar.gz | gpg --clearsign > $(distdir).tar.gz.md5sum

# Dependencies: all source files of the distribution
# Should be done when the distribution is made
doc: flopdoc cheapdoc

flopdoc:
	mkdir -p doc/flop
	$(DOXYGEN) doxyconfig.flop

cheapdoc:
	mkdir -p doc/cheap
	$(DOXYGEN) doxyconfig.cheap

TESTS = ConfigurationTest loggingTest

check_PROGRAMS = ConfigurationTest loggingTest
ConfigurationTest_SOURCES = \
	common/ConfigurationTest.cpp\
	common/Configuration.cpp

loggingTest_SOURCES = \
	common/logging.cpp\
	common/loggingTest.cpp

noinst_HEADERS = common/Configuration.h
