# conditional directories are always automatically added to DIST_SUBDIRS
# (but not if DIST_SUBDIRS is manually overwritten)
if ECLPREPROC
MAYBE_FSPP   = fspp
endif
SUBDIRS      = common flop $(MAYBE_FSPP) cheap

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

bindist: all
	$(am__remove_distdir)
	mkdir -p $(distdir)/bin
	mkdir -p $(distdir)/lib
	cp cheap/cheap $(distdir)/bin
	cp flop/flop $(distdir)/bin
	ldd cheap/cheap flop/flop | perl -ne ' print "cp $$2 $(distdir)/lib\n" if /(.+)\s+=>\s+(.+) \([0-9a-fx]+\)/ ' | sort -u | bash
	tar -czf $(distdir)_bin.tar.gz $(distdir)
	$(am__remove_distdir)

clean-local:
	-rm $(distdir)_bin.tar.gz

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

logon:
	strip flop/flop cheap/cheap
	/bin/cp flop/flop $$LOGONROOT/uio/bin/linux.x86.32/flop
	/bin/cp cheap/cheap $$LOGONROOT/uio/bin/linux.x86.32/cheap

endif

