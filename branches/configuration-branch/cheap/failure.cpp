/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2002 Ulrich Callmeier uc@coli.uni-sb.de
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

/* class representing a unification/subsumption failure */

#include "failure.h"
#include <iostream>

using std::list;

const char * const unification_failure::failure_names[] = {
  "success", "type clash", "cycle", "constraint clash", "coreference clash"
};

unification_failure::unification_failure()
  : _cyclic_paths()
{
    _type = SUCCESS;
    _path = 0;
    _s1 = _s2 = -1;
    _cost = -1;
}

unification_failure::unification_failure(const unification_failure &f)
    : _cyclic_paths()
{
    _type = f._type;
    _path = copy_list(f._path);
    _s1 = f._s1;
    _s2 = f._s2;
    _cost = f._cost;
  
    for(list<list_int *>::const_iterator iter = f._cyclic_paths.begin();
        iter != f._cyclic_paths.end(); ++iter)
        _cyclic_paths.push_back(copy_list(*iter));
}

unification_failure::unification_failure(failure_type t, list_int *rev_path,
                                         int cost, int s1, int s2,
                                         dag_node *cycle, dag_node *root)
    : _cyclic_paths()
{
    _type = t;
    _path = reverse(rev_path);
    _s1 = s1;
    _s2 = s2; _cost = cost;
#ifdef QC_PATH_COMP
    if(cycle && root)
    {
        _cyclic_paths = dag_paths(root, cycle);
    }
#endif
}

unification_failure::~unification_failure()
{
    free_list(_path);
    for(list<list_int *>::iterator iter = _cyclic_paths.begin();
        iter != _cyclic_paths.end(); ++iter)
        free_list(*iter);
}
  
unification_failure &
unification_failure::operator=(const unification_failure &f)
{
    if(_path)
        free_list(_path);
    
    for(list<list_int *>::iterator iter = _cyclic_paths.begin();
        iter != _cyclic_paths.end(); ++iter)
        free_list(*iter);

    _type = f._type;
    _path = copy_list(f._path);
    _s1 = f._s1;
    _s2 = f._s2;

    for(list<list_int *>::const_iterator iter = f._cyclic_paths.begin();
        iter != f._cyclic_paths.end(); ++iter)
        _cyclic_paths.push_back(copy_list(*iter));

    return *this;
}

void print_path(FILE *f, list_int *path) {
  bool dot = false;

  if(!path) fprintf(f, "<empty path>");
  
  while(path) {
    fprintf(f, "%s%s", dot ? "." : "", attrname[first(path)]), dot = true;
    path = rest(path);
  }
}

void print_path(std::ostream &out, list_int *path) {
  if(path == NULL) {
    out << "<empty path>" ;
    return;
  }

  out << attrname[first(path)];
  path = rest(path);
  while(path) {
    out << '.' << attrname[first(path)];
    path = rest(path);
  }
}

void
unification_failure::print(std::ostream &out) const {
  if ((_type >= SUCCESS) && (_type <= COREF)) {
    out << failure_names[_type];
  } else {
    out << "unknown failure";
  }

  if(_path == 0) {
    out << " at root level";
  } else {
    out << " under ";
    print_path(out);
  }

  switch(_type) {
  case CLASH:
    out << ": `" << type_name(_s1) << "' & `" << type_name(_s2) << "'";
    break;
  case CONSTRAINT: {
    int meet = glb(_s1, _s2);
    
    out << ": constraint `" << (meet == -1 ? "bottom" : type_name(meet))
        << "' introduced by `" 
        << type_name(_s1) << "' & `" << type_name(_s2) << "'";
    break;
  }
  case CYCLE:
    out << ":";
    for(list<list_int *>::const_iterator iter = _cyclic_paths.begin();
        iter != _cyclic_paths.end(); ++iter) {
      out << std::endl << "  ";
      ::print_path(out, *iter);
    }
    break;
  default: break;
  }
}
#ifdef USE_DEPRECATED
void
unification_failure::print(FILE *f) const
{
    switch(_type)
    {
    case SUCCESS:
        fprintf(f, "success");
        break;
    case CLASH:
        fprintf(f, "type clash");
        break;
    case CYCLE:
        fprintf(f, "cycle");
        break;
    case CONSTRAINT:
        fprintf(f, "constraint clash");
        break;
    case COREF:
        fprintf(f, "coreference clash");
        break;
    default:
        fprintf(f, "unknown failure");
        break;
    }

    if(_path == 0)
    {
        fprintf(f, " at root level");
    }
    else
    {
        fprintf(f, " under ");
        print_path(f);
    }

    if(_type == CLASH)
    {
        fprintf(f, ": `%s' & `%s'", type_name(_s1), type_name(_s2));
    }
    else if(_type == CONSTRAINT)
    {
        int meet = glb(_s1, _s2);
        
        fprintf(f, ": constraint `%s' introduced by `%s' & `%s'",
                meet == T_BOTTOM ? "bottom" : type_name(meet),
                type_name(_s1), type_name(_s2));
    }
    else if(_type == CYCLE)
    {
        fprintf(f, ":");
        for(list<list_int *>::const_iterator iter = _cyclic_paths.begin();
            iter != _cyclic_paths.end(); ++iter)
        {
            fprintf(f, "\n  ");
            ::print_path(f, *iter);
        }
    }
}
#endif
int
unification_failure::less_than(const unification_failure &b) const
{
    if(_type < b._type)
        return -1;
    
    if(b._type < _type)
        return 1;
    
    // types are equal - compare paths
    
    return compare(_path, b._path);
}
