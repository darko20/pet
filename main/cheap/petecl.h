/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2003 Ulrich Callmeier uc@coli.uni-sb.de
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/** \file petecl.h
 * ECL integration.
 *
 * Since ECL (and ecl.h in particular) doesn't compile as C++ we
 * cannot use ECL datatypes or functions from C++ modules. On the
 * other hand, we cannot call C++ functions from C (there's nothing
 * like an extern "C++" directive), so our setup is a little involved.
 *
 * The modules petecl and petmrs are written in C. They use ECL
 * functions and datatypes, and have access to PET data and
 * functionality by means of functions contained in the cppbridge
 * module. cppbridge is written in C++ and exports a number of extern
 * "C" functions. It includes standard PET headers, and uses their
 * functionality to implement the extern "C" functions.
 */


#ifndef _PETECL_H_
#define _PETECL_H_

/**
 * \brief ECL initialization function. Boots the ECL engine, loads
 * user-specified (interpreted) lisp files, and initializes the compiled
 * packages (currently only MRS).
 */
extern "C" int
ecl_initialize(int argc, char **argv, char *grammar_file_name);

/** Load a lisp file with the given name using the ECL interpreter. */
extern "C" void
ecl_load_lispfile(char *s);

#endif
