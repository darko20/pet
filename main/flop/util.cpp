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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include <LEDA/h_array.h>

#include "flop.h"

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

leda_h_array<string, int> name_type;

int Hash(const string &s)
{
  int h = 0, i, l = s.length();
  
  for(i = 0; i < l; i++) h = h*65599 + s[i]; 

  return h;
}

int lookup(const string &s)
{
  if(name_type.defined(s))
    return name_type[s];
  else
    return -1;
}

void indent (FILE *f, int nr)
{
  fprintf (f, "%*s", nr, "");
}

struct type *new_type(const string &name, bool is_inst, bool define = true)
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
