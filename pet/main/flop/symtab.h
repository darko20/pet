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

/* class to represent symbol tables */

#include <string>

#include <LEDA/array.h>
#include <LEDA/h_array.h>

extern int Hash(const string &s);

template <class info> class symtab {

  static const int INITIAL_SIZE = 128;

  leda_array<string> names;
  leda_h_array<string, int> ids;

  leda_array<info> infos;
  int n;

 public:
  
  symtab() : names(INITIAL_SIZE), ids(-1), infos(INITIAL_SIZE), n(0)
    { }

  int add(const string& name)
    {
      if(n > names.high())
	{
	  names.resize(0,n + INITIAL_SIZE);
	  infos.resize(0,n + INITIAL_SIZE);
	}

      names[n] = name;
      ids[name] = n;

      return n++;
    }

  int id(const string &name)
    {
      return ids[name];
    }
  
  const string &name(int id)
    {
      return names[id];
    }

  info &operator[](int id)
    {
      return infos[id];
    }

  int number()
    { return n; }
  
};
