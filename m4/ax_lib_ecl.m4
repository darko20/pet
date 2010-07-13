# SYNOPSIS
#
#   AX_LIB_ECL([action-if-found], [action-if-not-found])
#
# DESCRIPTION
#
#   Tests for the presence of the ECL (Embeddable Common Lisp) compiler,
#   headers, and library, using ecl-config.
#
#   If ECL is found, the following variables are defined as shell and
#   autoconf output variables:
#
#     ECL_CPPFLAGS
#     ECL_CPPFLAGS_SEARCHPATH
#     ECL_LDFLAGS
#     ECL_LDFLAGS_SEARCHPATH
#     ECL_LIBS
#
#   and the following preprocessor define is set:
#
#     HAVE_LIBECL
#
# LAST MODIFICATION
#
#   2008-02-26
#
# COPYLEFT
#
#   Copyright (c) 2008 Peter Adolphs
#
#   Copying and distribution of this file, with or without
#   modification, are permitted in any medium without royalty provided
#   the copyright notice and this notice are preserved.

AC_DEFUN([AX_LIB_ECL], [
  # define configure parameter
  unset ax_lib_ecl_path ax_lib_ecl_config ECL
  # Precious variables (values will be preserved by config.status --recheck)
  AC_ARG_WITH([ecl],
    [AC_HELP_STRING([--with-ecl@<:@=ARG@:>@],
      [use ECL from a standard location (ARG=yes),
       from the specified location or ecl-config binary (ARG=<path>),
       or disable it (ARG=no)
       @<:@ARG=yes@:>@ ])],
    [case "${withval}" in
       yes) ax_lib_ecl="yes" ;;
       no)  ax_lib_ecl="no"  ;;
       *)   ax_lib_ecl="yes"
            if test -d $withval ; then
              ax_lib_ecl_path="$withval/bin"
            elif test -x $withval ; then
              ax_lib_ecl_config=$withval
              ax_lib_ecl_path=$(dirname $(which $ax_lib_ecl_config))
            else
              AC_MSG_ERROR([bad value ${withval} for --with-ecl ])
            fi ;;
     esac],
    [ax_lib_ecl="yes" ])
  if test "x$ax_lib_ecl_path" = "x" ; then
    ax_lib_ecl_path=$PATH
  fi
  
  # check for ecl-config
  if test "x$ax_lib_ecl" = "xyes" ; then
    AC_PATH_PROG([ax_lib_ecl_config], [ecl-config], [""], [$ax_lib_ecl_path])
    if test "x$ax_lib_ecl_config" = "x" ; then
      ax_lib_ecl="no"
      AC_MSG_WARN([ECL requested but ecl-config not found.])
    else
      # ecl binary must be in the same path as ecl-config
      # (otherwise it could belong to a different ECL installation)
      ax_lib_ecl_path=$(dirname $(which $ax_lib_ecl_config))
    fi
  fi
  
  # check for ecl binary
  if test "x$ax_lib_ecl" = "xyes" ; then
    AC_PATH_PROG([ECL], [ecl], [""], [$ax_lib_ecl_path])
    if test "x$ECL" = "x" ; then
      ax_lib_ecl="no"
      AC_MSG_WARN([ECL requested but ecl binary not found.])
    fi
  fi
  
  # get various flags and settings (using ecl-config if present)
  # cf. Autoconf manual:
  # LDFLAGS: options as `-s' and `-L' affecting only the behavior of the linker
  # LIBS: `-l' options to pass to the linker
  unset ECL_CPPFLAGS ECL_CPPFLAGS_SEARCHPATH ECL_LDFLAGS ECL_LDFLAGS_SEARCHPATH
  unset ECL_LIBS
  if test "x$ax_lib_ecl" = "xyes" ; then
    ECL_CPPFLAGS=$($ax_lib_ecl_config --cflags |
                sed -e 's/ /\n/g; s/^-[[^IDU]].*//mg; s/^\n//mg; s/\n/ /mg;')
    ECL_CPPFLAGS_SEARCHPATH=$($ax_lib_ecl_config --cflags |
                sed -e 's/ /\n/g; s/^-[[^I]].*//mg; s/^\n//mg; s/\n/ /mg;')
    ECL_LDFLAGS=$($ax_lib_ecl_config --ldflags |
                sed -e 's/ /\n/g; s/^-l.*//mg; s/^\n//mg; s/\n/ /mg;')
    ECL_LDFLAGS_SEARCHPATH=$($ax_lib_ecl_config --ldflags |
                sed -e 's/ /\n/g; s/^-[[^L]].*//mg; s/^\n//mg; s/\n/ /mg;')
    ECL_LIBS=$($ax_lib_ecl_config --libs |
                sed -e 's/ /\n/g; s/^-[[^l]].*//mg; s/^\n//mg; s/\n/ /mg;')
  fi
  
  ax_lib_ecl_saved_CPPFLAGS=$CPPFLAGS
  ax_lib_ecl_saved_CFLAGS=$CFLAGS
  ax_lib_ecl_saved_LDFLAGS=$LDFLAGS
  ax_lib_ecl_saved_LIBS=$LIBS
  CPPFLAGS="$ECL_CPPFLAGS $CPPFLAGS"
  CFLAGS="$ECL_CFLAGS $CFLAGS"
  LDFLAGS="$ECL_LDFLAGS $LDFLAGS"
  LIBS="$ECL_LIBS $LIBS"
  
  # checking headers
  if test "x$ax_lib_ecl" = "xyes" ; then
    AC_REQUIRE([AC_PROG_CC])
    AC_LANG_PUSH([C])
    ax_lib_ecl="no"
    AC_CHECK_HEADERS([ecl/ecl.h ecl.h], [ax_lib_ecl="yes"])
  fi
  
  # checking library functionality
  if test "x$ax_lib_ecl" = "xyes" ; then
    AC_CHECK_LIB([ecl], [main], [], [ax_lib_ecl="no"])
    AC_LANG_POP([C])
  fi
  
  CPPFLAGS=$ax_lib_ecl_saved_CPPFLAGS
  CFLAGS=$ax_lib_ecl_saved_CFLAGS
  LDFLAGS=$ax_lib_ecl_saved_LDFLAGS
  LIBS=$ax_lib_ecl_saved_LIBS
  
  # final actions
  AC_SUBST([ECL_CPPFLAGS])
  AC_SUBST([ECL_CPPFLAGS_SEARCHPATH])
  AC_SUBST([ECL_LDFLAGS])
  AC_SUBST([ECL_LDFLAGS_SEARCHPATH])
  AC_SUBST([ECL_LIBS])
  if test "x$ax_lib_ecl" = "xyes"; then
    # execute action-if-found (if any)
    ifelse([$1], [], :, [$1])
  else
    # execute action-if-not-found (if any)
    ifelse([$2], [], :, [$2])
  fi
])

