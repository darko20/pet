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

/* ECL integration */

#include <ecl.h>
#include "cppbridge.h"

// Include the individual packages

// Replace nonexistent "rmrs.h" with "mrs.h" ... 
// Eric Nichols <eric-n@is.naist.jp>, Jun. 18, 2005
//#include "rmrs.h"
#include "mrs.h"

//
// ECL initialization function. Boots the ECL engine,
// loads user-specified (interpreted) lisp files, and
// initializes the compiled packages (currently only MRS).
//
int
ecl_initialize(int argc, char **argv, char *grammar_file_name)
{
    cl_boot(argc, argv);

    // Load lisp initialization files as specified in settings.

    ecl_cpp_load_files("preload-lisp-files", grammar_file_name);
    
    // Initialize the individual packages

    initialize_mrs();

    // Load lisp initialization files as specified in settings.

    ecl_cpp_load_files("postload-lisp-files", grammar_file_name);

    return 0;
}

//
// Load a lisp file with the given name using the ECL interpreter.
//
void
ecl_load_lispfile(char *fname)
{
    funcall(4,
            c_string_to_object("load"),
            make_string_copy(fname),
            c_string_to_object(":verbose"),
            Cnil);
}

