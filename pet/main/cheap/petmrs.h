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

/** \file petmrs.h
 * Interface to the LKB MRS module.
 * This module defines the C side of the interface required for the
 * LKB MRS package. The functions here are called from the
 * pet-interface.lisp module in the LKB MRS package.
 */

#ifndef _PETMRS_H_
#define _PETMRS_H_

#include "ecl.h"

/** Get the type code for \a type_name */
extern int
pet_type_to_code(cl_object type_name);

/** Get the name for type \a code as cl_object */
extern cl_object
pet_code_to_type(int code);

/** Get the attribute id for the attribute with name \a feature_name */
extern int
pet_feature_to_code(cl_object feature_name);

/** Get the feature name for attribute \a code as cl_object */
extern cl_object
pet_code_to_feature(int code);

/** A dummy always returning \a fs */
extern int
pet_fs_deref(int fs);

/** A dummy always returning zero */
extern int
pet_fs_cyclic_p(int fs);

/** check if \a fs is valid (not \c FAIL) */
extern int
pet_fs_valid_p(int fs);

/** Get a handle to the subdag reached when following the path encoded in \a
 * vector_of_codes from the root dag \a fs, or -1, if this path does not exist.
 */
extern int
pet_fs_path_value(int fs, cl_object vector_of_codes);

/** Return the arcs list of the dag node \a fs as lisp list of pairs (attribute
 *  id . dag node handle).
 */
extern cl_object
pet_fs_arcs(int fs);

/** Return the type id of dag node \a fs, if it is valid, -1 otherwise */
extern int
pet_fs_type(int fs);

/** Return a non-zero value if \a type1 is a subtype of \a type2 */
extern int
pet_subtype_p(int type1, int type2);

/** Return the glb of \a type1 and \a type2, if it exists, -1 otherwise */
extern int
pet_glb(int type1, int type2);

/** Return a non-zero value if \a type is a valid type id */
extern int
pet_type_valid_p(int type);

#endif
