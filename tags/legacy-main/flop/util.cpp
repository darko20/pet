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

/* utility functions */

#include "flop.h"
#include "hierarchy.h"
#include "lex-io.h"
#include "utility.h"

using namespace std;

int strcount(char *s, char c)
{
  int n = 0;

  assert(c != 0);

  while(s != 0)
    {
      if((s = strchr(s, (int) c)) != 0)
        {
          s++; n++;
          if(s[0] == '\0') s = 0;
        }
    }
  
  return n;
}

map<string, int> name_type;

int lookup(const string &s)
{
    map<string, int>::iterator it = name_type.find(s);
    if(it != name_type.end())
        return it->second;
    else
        return -1;
}

/** the constructor of the type struct */
struct type *new_type(const string &name, bool is_inst, bool define)
{
  struct type *t;
  t = new struct type;
  
  t -> status = NO_STATUS;
  t -> defines_status = 0;
  t -> status_giver = 0;

  t -> implicit = 0;

  t -> def = 0;

  t -> coref = 0;
  t -> constraint = 0;

  t -> bcode = 0;
  t -> thedag = 0;

  t -> printname = 0;

  if(define)
    {
      t->tdl_instance = is_inst;
      t->id = types.add(name);
      types[t->id] = t;
      register_type(t -> id);
    }
  else
    t -> id = -1;

  t -> inflr = 0;

  t -> parents = 0;

  return t;
}

/** Register a new builtin type with name \a name */
int
new_bi_type(const char *name)
{
    type *t = new_type(name, false);
    t->def = new_location("builtin", 0, 0);
    return t->id;
}

char *add_inflr(char *old, char *add)
{
  char *s = (char *) salloc((old == 0 ? 0 : strlen(old)) + strlen(add) + 1);
  if(old != 0)
    {
      strcpy(s, old);
      free(old);
    }
  else
    {
      strcpy(s, "");
    }
  strcat(s, add);
  return s;
}
