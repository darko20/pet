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

/* Interface to the LKB MRS module */

#ifndef _PETMRS_H_
#define _PETMRS_H_

#include "ecl.h"

//
// This module defines the C side of the interface required for the
// LKB MRS package. The functions here are called from the
// pet-interface.lisp module in the LKB MRS package.
//

extern int
pet_type_to_code(cl_object type_name);

extern cl_object
pet_code_to_type(int code);

extern int
pet_feature_to_code(cl_object feature_name);

extern cl_object
pet_code_to_feature(int code);


extern int
pet_fs_deref(int fs);

extern int
pet_fs_cyclic_p(int fs);

extern int
pet_fs_valid_p(int fs);

extern int
pet_fs_path_value(int fs, cl_object vector_of_codes);


extern cl_object
pet_fs_arcs(int fs);

extern int
pet_fs_type(int fs);


extern int
pet_subtype_p(int typ1, int type2);

extern int
pet_glb(int type1, int type2);

extern int
pet_type_valid_p(int type);

#endif
