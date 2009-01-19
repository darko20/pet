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

/* class to represent bitvectors of fixed size */

#include "bitcode.h"
#include "list-int.h"
#include "dumper.h"
#include "errors.h"

bitcode::bitcode(int n)
{
  sz = n; 
  register int i = 1 + sz/SIZE_OF_WORD;

  V = new CODEWORD[i];
  stop = V + i;

  while (i--) V[i]=0;
}

bitcode::bitcode(const bitcode& b)
{
  sz = b.sz;
  register int n = 1 + sz/SIZE_OF_WORD;

  V = new CODEWORD[n];
  stop = V + n;

  while (n--) V[n] = b.V[n];
}

bitcode& bitcode::operator=(const bitcode& b)
{
  if (this == &b) return *this;
  register int n = 1 + b.sz/SIZE_OF_WORD;
  if(sz != b.sz)
    {
      delete[] V;
      sz = b.sz;
      V = new CODEWORD[n];
      stop = V + n;
    }

  for(CODEWORD *p = V, *q = b.V; p < end(); p++, q++) *p = *q;

  return *this;
}

#ifdef ZONING
void bitcode::find_relevant_parts()
{
  first_set = last_set = -1;
  for(CODEWORD *p = V; p < end() && last_set == -1; p++)
  {
    if(first_set == -1)
    {
      if(*p != 0)
        first_set = p - V;
    }
    else if(last_set == -1)
    {
       if(*p == 0)
         last_set = p - V;
    }
  }

  if(first_set == -1)
    first_set = last_set = 0; // no bit set
  if(last_set == -1)
    last_set = stop - V;
}
#endif

list_int *bitcode::get_elements()
{
  // collect the positions of all bits that are 1 into the result list
  CODEWORD *p, w;
  int i;
  list_int *l = 0;

  for(p = V, i = 0; p < end(); p++, i++)
    if(*p)
      {
        w = *p;

        for(int j = 0; j < SIZE_OF_WORD; j++)
          {
            if((w & (CODEWORD)1) == 1)
              l = cons(i*SIZE_OF_WORD + j, l);
            w >>= 1;
          }
      }

  return l;
}

void
subset_bidir(const bitcode&A, const bitcode &B, bool &a, bool &b)
{
    // postcondition: a == subset(A, B) && b == subset(B, A)

    assert(A.sz == B.sz);
    CODEWORD *cA, *cB;
    a = b = true;

    for(cA = A.V, cB = B.V; cA < A.end(); cA++, cB++)
    {
        CODEWORD join = *cA & *cB;
        if(join != *cA) a = false;
        if(join != *cB) b = false;

        if(a == false && b == false)
            return;
    }
}

bool intersect_empty(const bitcode &A, const bitcode &B, bitcode *C)
{
  CODEWORD *p, *q, *s;
  bool empty;

  for(p = A.V, q = B.V, s = C -> V, empty = true; p < A.end(); p++, q++, s++)
    if((*s = *p & *q) != 0) empty = false;

  return empty;
}

bool bitcode::subset(const bitcode &supposed_superset)
{
  assert(sz == supposed_superset.sz);
  CODEWORD *sub, *super;

  for(sub = V, super = supposed_superset.V; sub < end(); sub++, super++)
    if((*sub & *super) != *sub) return false;

  return true;
}

#ifdef NAIVE_BITCODE_DUMP

void bitcode::dump(dumper *f)
{
  short int s = 1 + sz/SIZE_OF_WORD;

  f->dump_short(s);

  for(CODEWORD *p = V; p < end(); p++)
    f->dump_int(*p);
}

void bitcode::undump(dumper *f)
{
  short int s;

  s = f->undump_short();

  if(s != (1 + sz/SIZE_OF_WORD))
  {
    fprintf(ferr, "mismatch %d vs %d\n", s, 1+sz/SIZE_OF_WORD);
  }

  for(CODEWORD *p = V; p < end(); p++)
    *p = f->undump_int();
}

#else

void bitcode::dump(dumper *f)
{
  int n = 1 + sz/SIZE_OF_WORD;
  int i = 0;

  while(i < n)
    {
      f->dump_int(V[i]);
      if(V[i] == 0)
        {
          int j = i + 1;

          while(j < n && V[j] == 0)
            j++;

          f->dump_short((short int) (j - i));

          i = j;
        }
      else
        i++;
    }

  f->dump_int(0);
  f->dump_short(0);
}

void bitcode::undump(dumper *f)
{
  int n = 1 + sz/SIZE_OF_WORD;
  int i = 0;

  while(i < n)
    {
      V[i] = f->undump_int();
      if(V[i] == 0)
        {
          int l = f->undump_short();

          while(--l > 0)
            {
              i++;
              if(i >= n)
                throw tError("invalid compressed bitcode (too long)");
              V[i] = 0;
            }
        }
      i++;
    }

  if( f->undump_int() != 0 || f->undump_short() != 0)
    throw tError("invalid compressed bitcode (no end marker)");
}

#endif

int Hash(const bitcode &C)
{
  
  for(CODEWORD *p = C.V; p < C.end(); p++)
    if(*p != 0)
      {
        for(int j = 0; j < C.SIZE_OF_WORD ; j++)
          {
            if(*p & (1 << j))
              return (p - C.V) * C.sz + j;
          }
      }

  //  CODEWORD *stop = C.V + C.sz/C.SIZE_OF_WORD + 1;
  //  for(CODEWORD *p = C.V; p < stop; p++)
  //    if(*p != 0) return *p;
  
  return 0;
}

std::ostream& operator<<(std::ostream& O, const bitcode& C)
{
  for(int i=0; i < C.sz; i++)
    {
      if(C.member(i)) O << i << " "; 
    }
  O << std::endl;
  return O;
}
