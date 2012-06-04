##### http://autoconf-archive.cryp.to/ax_boost_regex.html
# ATTENTION: original file modified for PET by Peter Adolphs
#
# SYNOPSIS
#
#   AX_BOOST_REGEX([action-if-found], [action-if-not-found])
#
# DESCRIPTION
#
#   Test for Regex library from the Boost C++ libraries. The macro
#   requires a preceding call to AX_BOOST_BASE. Further documentation
#   is available at <http://randspringer.de/boost/index.html>.
#
#   This macro calls:
#
#     AC_SUBST(BOOST_REGEX_LIBS)
#
#   And sets:
#
#     HAVE_BOOST_REGEX
#
# LAST MODIFICATION
#
#   2010-01-28 by Peter Adolphs
#              - test alternative library names
#   2008-11-03 by Peter Adolphs
#              - simplified header and library checks using standard
#                autoconf macros
#   2008-02-25 by Peter Adolphs
#              - fixed broken indentation (tabs/spaces)
#              - added ACTION-IF-FOUND and ACTION-IF-NOT-FOUND
#              - notice instead of error when boost was not found
#              - changed help message
#
# COPYLEFT
#
#   Copyright (c) 2007 Thomas Porschberg <thomas@randspringer.de>
#   Copyright (c) 2007 Michael Tindal
#   Copyright (c) 2008 Peter Adolphs
#
#   Copying and distribution of this file, with or without
#   modification, are permitted in any medium without royalty provided
#   the copyright notice and this notice are preserved.

AC_DEFUN([AX_BOOST_REGEX],
[
  ax_boost_regex_default_libnames="boost_regex boost_regex-mt"
  AC_ARG_WITH([boost-regex],
    [ AS_HELP_STRING(
        [--with-boost-regex@<:@=ARG@:>@],
        [use standard Boost.Regex library (ARG=yes),
         use specific Boost.Regex library (ARG=<name>, e.g. ARG=boost_regex-gcc-mt-d-1_33_1),
         or disable it (ARG=no)
         @<:@ARG=yes@:>@ ]) ],
    [
      if test "$withval" = "no"; then
        ax_boost_regex_wanted="no"
      elif test "$withval" = "yes"; then
        ax_boost_regex_wanted="yes"
        ax_boost_regex_libnames="${ax_boost_regex_default_libnames}"
      else
        ax_boost_regex_wanted="yes"
        ax_boost_regex_libnames="$withval"
      fi
    ],
    [ax_boost_regex_wanted="yes"
     ax_boost_regex_libnames="${ax_boost_regex_default_libnames}"]
  )
  
  if test "x$ax_boost_regex_wanted" = "xyes"; then
    AC_REQUIRE([AX_BOOST_BASE])
    CPPFLAGS_SAVED="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
    export CPPFLAGS
    
    LDFLAGS_SAVED="$LDFLAGS"
    LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
    export LDFLAGS
    
    AC_LANG_PUSH([C++])
    AC_CHECK_HEADER([boost/regex.hpp],
                    [ax_boost_regex_headers=yes],
                    [ax_boost_regex_headers=no
                     AC_MSG_WARN([Could not find header files for Boost.Regex])])
    if test "x${ax_boost_regex_headers}" = "xyes"; then
      for ax_boost_regex_libname in ${ax_boost_regex_libnames} ; do
        AC_CHECK_LIB([$ax_boost_regex_libname], [main],
                     [ax_boost_regex_links=yes
                      BOOST_REGEX_LIBS=-l${ax_boost_regex_libname}
                      break
                     ],
                     [ax_boost_regex_links=no])
      done
      test $ax_boost_regex_links == "no" && AC_MSG_WARN([Could not link Boost.Regex])
    fi
    AC_LANG_POP([C++])
    
    if test "x$ax_boost_regex_headers" = "xyes" -a "x$ax_boost_regex_links" = "xyes"; then
      AC_DEFINE(HAVE_BOOST_REGEX,,[define if the Boost.Regex library is available])
      AC_SUBST([BOOST_REGEX_LIBS])
      # execute ACTION-IF-FOUND (if present):
      ifelse([$1], , :, [$1])
    else
      # execute ACTION-IF-NOT-FOUND (if present):
      ifelse([$2], , :, [$2])
    fi
    
    CPPFLAGS="$CPPFLAGS_SAVED"
    LDFLAGS="$LDFLAGS_SAVED"
  fi
])
