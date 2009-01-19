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

/** \file list-int.h
 * Minimum overhead lists of integers - to avoid overhead of list<int>.
 * Everything here is inlined.
 */

#ifndef _LIST_INT_H_
#define _LIST_INT_H_

#include <list>
#include <functional>

/** Minimum overhead single linked lists of integers */
typedef struct list_int {
  /** The first of this cons */
  int val;
  /** The rest of this cons */
  list_int *next;
} list_int;

/** create a new cons cell with value = \a v and successor \a n */
inline list_int *cons(int v, list_int *n)
{
  list_int *c;

  c = new list_int;
  c->next = n;
  c->val = v;
  
  return c;
}
  
/** Return the content of the cons cell \a *l */
inline int first(const list_int *l)
{
  return l->val;
}

/** Return the successor of the cons cell \a *l */
inline list_int *rest(const list_int *l)
{
  return l->next;
}

/**
 * Deletes the last cell of the list \a l and returns \a l itself or NULL
 * if the last cell was the only cell in \a l.
 * If the list is empty, NULL is returned.
 */
inline list_int * pop_last(list_int *l)
{
  if (!l)
    return NULL;
  list_int *curr = l, *prec = NULL;
  while (rest(curr)) {
    prec = curr;
    curr = rest(curr);
  }
  delete curr;
  if (prec)
    prec->next = NULL;
  else
    l = NULL;
  return l;
}

/**
 * Deletes the first cell of the list \a l and returns the rest of the list.
 * If the list is empty, NULL is returned.
 */
inline list_int *pop_first(list_int *l)
{
  list_int *res;

  if(!l) return 0;

  res = rest(l);
  delete l;
  return res;
}

/** Free all cons cells of \a l */
inline void free_list(list_int *l)
{
  while(l)
    l = pop_first(l);
}

/** Search \a l for a cons cell whose content is \a e.
 * \return \c true if the list contains \a e, \c false otherwise.
 */
inline bool contains(const list_int *l, int e)
{
  while(l)
    {
      if(first(l) == e)
        return true;
      
      l = rest(l);
    }
  return false;
}

/** Return the last cons cell of the list beginning at \a l */
inline list_int *last(list_int *l)
{
  if(!l) return 0;

  while(rest(l)) l = rest(l);
  return l;
}

/** This is rather nconc than append: add a new list element containing \a e to
 *  \a l and return \a l.
 */
inline list_int *append(list_int *l, int e)
{
  if(l == 0)
    return cons(e, l);

  last(l)->next = cons(e, 0);
  return l;
}

/** Return length of \a l. Complexity is \f$ {\cal O}(n) \f$ */
inline int length(const list_int *l)
{
  int n = 0;
  while(l) n++, l = rest(l);
  return n;
}

/** Return newly allocated reverse copy of \a l */
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

/** Reverse the list \a l without using new cons cells (destructive) */
inline list_int *nreverse(list_int *l)
{
  list_int *curr, *rev = l;
  if (rev) {
    l = rest(l);
    rev->next = 0;
  }

  while(l)
    { 
      curr = l;
      l = rest(l);
      curr->next = rev;
      rev = curr;
    }

  return rev;
}

/** Return newly allocated copy of \a l */
inline list_int *copy_list(const list_int *l)
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

/** Return newly allocated copy of \a li */
inline list_int *copy_list(const std::list<int> &li)
{
  list_int *res = 0;

  for(std::list<int>::const_reverse_iterator it = li.rbegin()
        ; it != li.rend(); ++it)
    {
      res = cons(*it, res);
    }

  return res;
}

/** Merge two sorted lists \a a and \a b and return the new (sorted) list
 *  consisting of the cons cells of the arguments (destructive).
 */
inline list_int *merge(list_int *a, list_int *b) {
  list_int head;
  list_int *curr = &head;
  while (a && b) {
    if (a->val <= b->val) {
      curr->next = a; a = a->next;
    } else {
      curr->next = b; b = b->next;
    }
    curr = curr->next;
  }
  curr->next = a ? a : b;
  return head.next;
}

/** Is \a a a prefix of \a b? */
inline bool
prefix(const list_int *a, const list_int *b)
{
    if(a == 0)
        return true;

    if(b == 0)
        return false;

    if(first(a) == first(b))
        return prefix(rest(a), rest(b));

    return false;
}

/** Lexicographic compare of \a a and \a b.
 *  \return \c 0, if \a a and \a b are equal, \c -1, if \a a < \a b,
 *          \c 1 if \a a > \a b.
 */
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

/** Binary predicate to perform lexicographic ordering of list_int* objects. */
class list_int_compare 
: public std::binary_function<list_int *, list_int *, bool>
{
 public:
  /** Comparison operator. */
  inline bool operator() (const list_int* x, const list_int* y) const
    {
      return compare(x, y) == -1;
    }
};

#endif
