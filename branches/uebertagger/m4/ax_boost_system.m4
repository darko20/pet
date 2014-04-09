# ===========================================================================
#    http://www.gnu.org/software/autoconf-archive/ax_boost_system.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_BOOST_SYSTEM
#
# DESCRIPTION
#
#   Test for System library from the Boost C++ libraries. The macro
#   requires a preceding call to AX_BOOST_BASE. Further documentation is
#   available at <http://randspringer.de/boost/index.html>.
#
#   This macro calls:
#
#     AC_SUBST(BOOST_SYSTEM_LIB)
#
#   And sets:
#
#     HAVE_BOOST_SYSTEM
#
# LICENSE
#
#   Copyright (c) 2008 Thomas Porschberg <thomas@randspringer.de>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 20

AC_DEFUN([AX_BOOST_SYSTEM],
[
  ax_boost_system_default_libnames="boost_system boost_system-mt"
	AC_ARG_WITH([boost-system],
	AS_HELP_STRING([--with-boost-system@<:@=special-lib@:>@],
                   [use the System library from boost - it is possible to specify a certain library for the linker
                        e.g. --with-boost-system=boost_system-gcc-mt-d-1_33_1 ]),
        [
        if test "$withval" = "no"; then
			ax_boost_system_wanted="no"
        elif test "$withval" = "yes"; then
            ax_boost_system_wanted="yes"
            ax_boost_system_libnames="${ax_boost_system_default_libnames}"
        else
		    ax_boost_system_wanted="yes"
		    ax_boost_system_libnames="$withval"
		fi
        ],
        [ax_boost_system_wanted="yes"
         ax_boost_system_libnames="${ax_boost_system_default_libnames}"]
	)

	if test "x$ax_boost_system_wanted" = "xyes"; then
    AC_REQUIRE([AX_BOOST_BASE])
		CPPFLAGS_SAVED="$CPPFLAGS"
		CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
		export CPPFLAGS

		LDFLAGS_SAVED="$LDFLAGS"
		LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
		export LDFLAGS

        AC_LANG_PUSH([C++])
    AC_CHECK_HEADER([boost/system/error_code.hpp],
                    [ax_boost_system_headers=yes],
                    [ax_boost_system_headers=no
                     AC_MSG_WARN([Could not find header files for Boost.System])])
    if test "x${ax_boost_system_headers}" = "xyes"; then
      for ax_boost_system_libname in ${ax_boost_system_libnames} ; do
        AC_CHECK_LIB([$ax_boost_system_libname], [main],
                     [ax_boost_system_links=yes
                      BOOST_SYSTEM_LIBS=-l${ax_boost_system_libname}
                      break
                     ],
                     [ax_boost_system_links=no])
      done
      test $ax_boost_system_links == "no" && AC_MSG_WARN([Could not link Boost.System])
    fi
    AC_LANG_POP([C++])
    
    if test "x$ax_boost_system_headers" = "xyes" -a "x$ax_boost_system_links" = "xyes"; then
      AC_DEFINE(HAVE_BOOST_SYSTEM,,[define if the Boost.System library is available])
      AC_SUBST([BOOST_SYSTEM_LIBS])
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
  
