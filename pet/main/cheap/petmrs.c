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

/* C bridge between Lisp functions and PET for the MRS code integration */

#include <stdlib.h>

#include "petmrs.h"
#include "mfile.h"
#include "cppbridge.h"

//
// Helper functions.
//

char *
ecl_decode_string(cl_object x)
{
    if (type_of(x) == t_string)
        return x->string.self;
    else
        return 0;
}

int *
ecl_decode_vector_int(cl_object x)
{
    int i;
    int *v = 0;
    cl_object y;

    if(type_of(x) != t_vector)
        return v;
    
    v = malloc(sizeof(int) * (x->vector.fillp + 1));
    v[x->vector.fillp] = -1; // end marker

    for(i = 0; i < x->vector.fillp; i++)
    {
        y = aref1(x, i);
        if(type_of(y) != t_fixnum)
            v[i] = -1;
        else
            v[i] = fix(y);
    }

    return v; // caller has to free v
}

//
// Interface functions called from Lisp.
//

int
pet_type_to_code(cl_object type_name)
{
    char *s = ecl_decode_string(type_name);
    return pet_cpp_lookup_type_code(s);
}

cl_object
pet_code_to_type(int code)
{
    if(! pet_cpp_type_valid_p(code))
        return Cnil;

    return make_string_copy(pet_cpp_lookup_type_name(code));
}

int
pet_feature_to_code(cl_object feature_name)
{
    char *s = ecl_decode_string(feature_name);
    return pet_cpp_lookup_attr_code(s);
}

cl_object
pet_code_to_feature(int code)
{
    if(code < 0 || code >= pet_cpp_nattrs())
        return Cnil;

    return make_string_copy(pet_cpp_lookup_attr_name(code));
}

int
pet_fs_deref(int fs)
{
    return fs;
}

int
pet_fs_cyclic_p(int fs)
{
    return 0;
}

int 
pet_fs_valid_p(int fs)
{
    return pet_cpp_fs_valid_p(fs);
}

int
pet_fs_path_value(int fs, cl_object vector_of_codes)
{
    int res;

    int *v = ecl_decode_vector_int(vector_of_codes);

    res = pet_cpp_fs_path_value(fs, v);

    free(v);

    return res;
}

cl_object
pet_fs_arcs(int fs)
{
    cl_object res;
    struct MFILE *f = mopen();
    int *v;
    int i;

    pet_cpp_fs_arcs(fs, &v);

    if(v)
    {
        mprintf(f, "(");
        for(i = 0; v[i] != -1; i+=2)
        {
            mprintf(f, "(%d . %d)",
                    v[i], v[i+1]);
        }
        mprintf(f, ")");
        free(v);
    }

    res = c_string_to_object(mstring(f));
    mclose(f);
    return res;
}

int
pet_fs_type(int fs)
{
    return pet_cpp_fs_type(fs);
}

int
pet_subtype_p(int t1, int t2)
{
    return pet_cpp_subtype_p(t1, t2);
}

int
pet_glb(int t1, int t2)
{
    return pet_cpp_glb(t1, t2);
}

int
pet_type_valid_p(int t)
{
    return pet_cpp_type_valid_p(t);
}

char *
ecl_extract_mrs(int fs, char *mode, int cfrom, int cto)
{
    cl_object result;
    result = funcall(3,
                     c_string_to_object("mrs::fs-to-mrs"),
                     MAKE_FIXNUM(fs),
                     make_string_copy(mode));
    return ecl_decode_string(result);
}
