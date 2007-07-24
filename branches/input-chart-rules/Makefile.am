# conditional directories are always automatically added to DIST_SUBDIRS
# (but not if DIST_SUBDIRS is manually overwritten)
if ECLPREPROC
MAYBE_FSPP   = fspp
endif
SUBDIRS      = common flop cheap $(MAYBE_FSPP)

EXTRA_DIST   = BUGS CHANGELOG LICENSE README TODO \
               doxyconfig.cheap doxyconfig.flop

# tell aclocal where to find local and third-party M4 macros (aclocal >= 1.6)
# if configure.ac uses a macro not yet present in the project's m4 directory
# but finds one in the system directories, copy it to the project's m4 dir:
ACLOCAL_AMFLAGS = -I m4 --install

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
if DOCBUILDERS
doc: flopdoc cheapdoc

flopdoc:
	$(MKDIR_P) doc/flop
	$(DOXYGEN) doxyconfig.flop

cheapdoc:
	$(MKDIR_P) doc/cheap
	$(DOXYGEN) doxyconfig.cheap
endif

