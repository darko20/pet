# conditional directories are always automatically added to DIST_SUBDIRS
# (but not if DIST_SUBDIRS is manually overwritten)
if ECLPREPROC
MAYBE_FSPP   = fspp
endif
SUBDIRS      = common flop cheap $(MAYBE_FSPP)

EXTRA_DIST   = BUGS CHANGELOG LICENSE README TODO \
               doxyconfig.cheap.in doxyconfig.flop.in \
               sample

# Tell aclocal where to find local and third-party M4 macros (aclocal >= 1.6)
ACLOCAL_AMFLAGS = -I m4

# For ebrowse (Emacs)
BROWSE: 
	echo `find $(topsrc_dir) -name '*.h' -print ` \
	     `find $(topsrc_dir) \( -name '*.c' -o -name '*.cpp' \) -print` | \
	tr ' ' '\n'| ebrowse

# Remove meta files that might end up in the distribution via EXTRA_DIST:
dist-hook:
	rm -rf $(find -type d -name .svn)
	rm -f $(find -type d -name "*.grm")

sign: dist
	gpg --detach-sign --armor $(distdir).tar.gz
	md5sum $(distdir).tar.gz | gpg --clearsign > $(distdir).tar.gz.md5sum

# Dependencies: all source files of the distribution
# Should be done when the distribution is made
if DOCBUILDERS
doc: flopdoc cheapdoc

flopdoc:
	mkdir -p doc/flop
	$(DOXYGEN) doxyconfig.flop

cheapdoc:
	mkdir -p doc/cheap
	$(DOXYGEN) doxyconfig.cheap
endif

