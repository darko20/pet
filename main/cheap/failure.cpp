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

/* class representing a unification failure */

#include "pet-system.h"
#include "failure.h"
#include "types.h"

unification_failure::unification_failure()
  : _cyclic_paths()
{
  _type = SUCCESS; _path = 0; _s1 = _s2 = -1; _cost = -1;
}

unification_failure::unification_failure(const unification_failure &f)
  : _cyclic_paths()
{
  _type = f._type; _path = copy_list(f._path); _s1 = f._s1; _s2 = f._s2; _cost = f._cost;
  
  for(list<list_int *>::const_iterator iter = f._cyclic_paths.begin(); iter != f._cyclic_paths.end(); ++iter)
    _cyclic_paths.push_back(copy_list(*iter));
}

unification_failure::unification_failure(failure_type t, list_int *rev_path, int cost,
					 int s1, int s2, dag_node *cycle, dag_node *root)
  : _cyclic_paths()
{
  _type = t; _path = reverse(rev_path); _s1 = s1; _s2 = s2; _cost = cost;
  if(cycle && root)
    {
      _cyclic_paths = dag_paths(root, cycle);
    }
}

unification_failure::~unification_failure()
{
  free_list(_path);
  for(list<list_int *>::iterator iter = _cyclic_paths.begin(); iter != _cyclic_paths.end(); ++iter)
    free_list(*iter);
}
  
unification_failure &unification_failure::operator=(const unification_failure &f)
{
  if(_path) free_list(_path);
  for(list<list_int *>::iterator iter = _cyclic_paths.begin(); iter != _cyclic_paths.end(); ++iter)
    free_list(*iter);

  _type = f._type; _path = copy_list(f._path); _s1 = f._s1; _s2 = f._s2;

  for(list<list_int *>::const_iterator iter = f._cyclic_paths.begin(); iter != f._cyclic_paths.end(); ++iter)
    _cyclic_paths.push_back(copy_list(*iter));

  return *this;
}

void print_path(FILE *f, list_int *path)
{
  bool dot = false;

  if(!path) fprintf(f, "<empty path>");
  
  while(path)
    {
      fprintf(f, "%s%s", dot ? "." : "", attrname[first(path)]), dot = true;
      path = rest(path);
    }
}

void unification_failure::print(FILE *f) const
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
      fprintf(f, ": `%s' & `%s'", typenames[_s1], typenames[_s2]);
    }
  else if(_type == CONSTRAINT)
    {
      int meet = glb(_s1, _s2);

      fprintf(f, ": constraint `%s' introduced by `%s' & `%s'",
	      meet == -1 ? "bottom" : typenames[meet], typenames[_s1], typenames[_s2]);
    }
  else if(_type == CYCLE)
    {
      fprintf(f, ":");
      for(list<list_int *>::const_iterator iter = _cyclic_paths.begin(); iter != _cyclic_paths.end(); ++iter)
	{
	  fprintf(f, "\n  ");
	  ::print_path(f, *iter);
	}
    }
}

int compare(const unification_failure &a, const unification_failure &b)
{
  if(a._type < b._type)
    return -1;
  
  if(b._type < a._type)
    return 1;

  // types are equal - compare paths

  return compare(a._path, b._path);
}
