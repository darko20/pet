/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
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

  while(s != NULL)
    {
      if((s = strchr(s, (int) c)) != NULL)
        {
          s++; n++;
          if(s[0] == '\0') s = NULL;
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

  t -> def = NULL;

  t -> coref = NULL;
  t -> constraint = NULL;

  t -> bcode = NULL;
  t -> thedag = NULL;

  if(define)
    {
      t->tdl_instance = is_inst;
      t->id = types.add(name);
      types[t->id] = t;
      register_type(t -> id);
    }
  else
    t -> id = -1;

  t -> parents = 0;

  return t;
}
