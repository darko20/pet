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

/* Bridge from C to C++ into PET for the ECL lisp integration */

#include "pet-system.h"
#include "types.h"
#include "fs.h"
#include "settings.h"
#include "utility.h"
#include "cheap.h"

#include "cppbridge.h"
#include "petecl.h"

//
//
// Functions callable from C that call into the PET (C++) core.
//
//

#define LISP_EXT ".lisp"

char *
ecl_cpp_find_file(const char *fileName, const char *basePath)
{
    // _fix_me_
    // This piece of uglyness (copied from settings::settings())
    // should be factored out and encapsulated.

    char *res = 0;

    if(basePath)
    {
        char *slash = strrchr((char *) basePath, '/');
        char *prefix = (char *) malloc(strlen(basePath) + 1 + 1);
        if(slash)
	{
            strncpy(prefix, basePath, slash - basePath + 1);
            prefix[slash - basePath + 1] = '\0';
	}
        else
	{
            strcpy(prefix, "");
	}

        char *fname = (char *) malloc(strlen(prefix) + 1
                                      + strlen(fileName) + 1);
        strcpy(fname, prefix);
        strcat(fname, fileName);
      
        res = find_file(fname, LISP_EXT, false);

        free(prefix);
    }
    else
    {
        res = (char *) malloc(strlen(fileName) + 1);
        strcpy(res, fileName);
    }
  
    return res;
}

void
ecl_cpp_load_files(char *setting, char *basename)
{
    struct setting *set;
    if((set = cheap_settings->lookup(setting)) != 0)
    {
        for(int i = 0; i < set->n; i++)
        {
            char *fname = ecl_cpp_find_file(set->values[i], basename);
            if(fname)
            {
                ecl_load_lispfile(fname);
                free(fname);
            }
        }             
    }

}

int
pet_cpp_ntypes()
{
    return ntypes;
}

int
pet_cpp_lookup_type_code(const char *s)
{
    return lookup_type(s);
}

char *
pet_cpp_lookup_type_name(int t)
{
    return typenames[t];
}

int
pet_cpp_nattrs()
{
    return nattrs;
}

int
pet_cpp_lookup_attr_code(const char *s)
{
    return lookup_attr(s);
}

char *
pet_cpp_lookup_attr_name(int a)
{
    return attrname[a];
}

map<int, dag_node *> handle_to_dag;
map<dag_node *, int> dag_to_handle;
int next_handle = 1;

int
get_handle(dag_node *d)
{
    map<dag_node *, int>::iterator it = dag_to_handle.find(d);
    if(it != dag_to_handle.end())
    {
        return it->second;
    }
    else
    {
        handle_to_dag[next_handle] = d;
        dag_to_handle[d] = next_handle;
        return next_handle++;
    }
}

dag_node *
get_dag(int handle)
{
    map<int, dag_node *>::iterator it = handle_to_dag.find(handle);
    if(it != handle_to_dag.end())
    {
        return it->second;
    }
    else
    {
        return 0;
    }
}

void
flush_handles()
{
    handle_to_dag.clear();
    dag_to_handle.clear();
    next_handle = 1;
}

int
pet_cpp_fs_valid_p(int fs)
{
    dag_node *d = get_dag(fs);
    if(d && d != FAIL)
        return 1;
    else
        return 0;
}

int
pet_cpp_fs_type(int fs)
{
    dag_node *d = get_dag(fs);
    if(d && d != FAIL)
        return dag_type(d);
    else
        return -1;
}

int
pet_cpp_fs_path_value(int fs, int *path)
{
    dag_node *d = get_dag(fs);
    if(!d || d == FAIL)
        return -1;
 
    list_int *p = 0;
    for(int i = 0; path && path[i] != -1; i++)
    {
        p = append(p, path[i]);
    }
     
    dag_node *v = dag_get_path_value_l(d, p);
    free_list(p);

    if(!v || v == FAIL)
        return -1;

    return get_handle(v);
}

void
pet_cpp_fs_arcs(int fs, int **res)
{
    *res = 0;

    dag_node *d = get_dag(fs);
    if(!d || d == FAIL)
        return;

    dag_arc *a;
    int n = 0;

    a = d->arcs;
    while(a != 0)
    {
        a = a->next;
        n++;
    }
    
    *res = (int *) malloc((n+1) * 2 * sizeof(int));

    n = 0;
    a = d->arcs;
    while(a != 0)
    {
        (*res)[n] = a->attr;
        (*res)[n+1] = get_handle(a->val);
        a = a->next;
        n+=2;
    }
    (*res)[n] = -1;
    (*res)[n+1] = -1;
}

int
pet_cpp_subtype_p(int t1, int t2)
{
    return subtype(t1, t2);
}

int
pet_cpp_glb(int t1, int t2)
{
    return glb(t1, t2);
}

int
pet_cpp_type_valid_p(int t)
{
    return t >= 0 && t < ntypes;
}

//
//
// Functions callable from C++ that call into ECL. 
//
//

extern "C" char *
ecl_extract_mrs(int fs, char *mode);

string
ecl_cpp_extract_mrs(dag_node *d, char *mode)
{
    d = dag_expand(d);
    char *res = ecl_extract_mrs(get_handle(d), mode);
    flush_handles();

    if(res)
        return string(res);
    else
        return string();
}
