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

/* minimum overhead lists of integers - to avoid overhead of list<int> */
/* everything here is inlined */

#ifndef _LIST_INT_H_
#define _LIST_INT_H_

typedef struct list_int {
  int val;
  list_int *next;
} list_int;

inline list_int *cons(int v, list_int *n)
{
  list_int *c;

  c = New list_int;
  c->next = n;
  c->val = v;
  
  return c;
}
  
inline int first(const list_int *l)
{
  return l->val;
}

inline list_int *rest(const list_int *l)
{
  return l->next;
}

inline list_int *pop_rest(list_int *l)
{
  list_int *res;

  if(!l) return 0;

  res = rest(l);
  delete l;
  return res;
}

inline void free_list(list_int *l)
{
  while(l)
    l = pop_rest(l);
}

inline bool contains(list_int *l, int e)
{
  while(l)
    {
      if(first(l) == e)
	return true;
      
      l = rest(l);
    }
  return false;
}

inline list_int *last(list_int *l)
{
  if(!l) return 0;

  while(rest(l)) l = rest(l);
  return l;
}

inline list_int *append(list_int *l, int e)
{
  if(l == 0)
    return cons(e, l);

  last(l)->next = cons(e, 0);
  return l;
}

inline int length(list_int *l)
{
  int n = 0;
  while(l) n++, l = rest(l);
  return n;
}

inline list_int *reverse(list_int *l)
{
  list_int *rev = 0;

  while(l)
    {
      rev = cons(first(l), rev);
      l = rest(l);
    }

  return rev;
}

inline list_int *copy_list(list_int *l)
{
  list_int *head = 0, **tail = &head;
  
  while(l)
    {
      *tail = cons(first(l), 0);
      tail = &((*tail) -> next);
      l = rest(l);
    }

  return head;
}

inline int compare(const list_int *a, const list_int *b)
{
  while(a && b)
    {
      if(first(a) < first(b))
	return -1;
      if(first(b) < first(a))
	return 1;
      
      a = rest(a); b = rest(b);
    }
  
  if(a == b) return 0;
  
  if(!a) return -1;
  if(!b) return 1;

  return 0;
}

class list_int_compare 
{
 public:
  inline bool operator() (const list_int* x, const list_int* y) const
    {
      return compare(x, y) == -1;
    }
};

#endif
